# Root Key Deep Dive — Verification, Rotation & Revocation

> **Applies to:** ADU agent v1.4.0
> **Audience:** Agent maintainers, security engineers, and solution operators who need to understand or operate the root-key trust chain.
> **Source of truth:** Everything in this doc is verified against the `develop` branch source. Source-file references are included throughout.

This document explains, end-to-end, **how the ADU agent uses root keys to verify update manifests, how the root-key trust chain bootstraps, and how root keys can be rotated or revoked**. It is the companion deep-dive for the Security Model section of [architecture-overview.md](architecture-overview.md#security-model).

## Table of contents

- [1. Why root keys matter](#1-why-root-keys-matter)
- [2. What is a root-key package, and who creates it?](#2-what-is-a-root-key-package-and-who-creates-it)
- [3. The trust chain at a glance](#3-the-trust-chain-at-a-glance)
- [4. The two hardcoded root keys](#4-the-two-hardcoded-root-keys)
- [5. Anatomy of a root-key package](#5-anatomy-of-a-root-key-package)
- [6. How the agent uses the root key for content verification](#6-how-the-agent-uses-the-root-key-for-content-verification)
  - [6.1 The download trigger](#61-the-download-trigger)
  - [6.2 Successful download — storage and runtime use](#62-successful-download--storage-and-runtime-use)
  - [6.3 Failed download or validation](#63-failed-download-or-validation)
  - [6.4 The "package unchanged" path](#64-the-package-unchanged-path)
- [7. Root-key rotation and revocation](#7-root-key-rotation-and-revocation)
  - [7.1 Two independent revocation lists](#71-two-independent-revocation-lists)
  - [7.2 Revoking a signing key (intermediate)](#72-revoking-a-signing-key-intermediate)
  - [7.3 Revoking a root key (one of two)](#73-revoking-a-root-key-one-of-two)
  - [7.4 Introducing a new root key](#74-introducing-a-new-root-key)
  - [7.5 What if both hardcoded root keys are compromised at once?](#75-what-if-both-hardcoded-root-keys-are-compromised-at-once)
- [8. Failure-mode reference](#8-failure-mode-reference)
- [9. Operator runbook (cloud-side rotation)](#9-operator-runbook-cloud-side-rotation)
- [10. Observations and limitations](#10-observations-and-limitations)

---

## 1. Why root keys matter

The ADU agent receives every update instruction from Azure IoT Hub as a JSON object — the **update manifest** (carried as a stringified JSON in the `updateManifest` field) plus a signature in the `updateManifestSignature` field. The signature is a compact JWS whose payload is a small object containing the SHA-256 hash of the `updateManifest` string. The agent verifies the JWS, then re-computes the hash of the manifest string and compares it to the hash in the JWS payload (see [`workflow_utils.c`](../../src/utils/workflow_utils/src/workflow_utils.c) — `Json_ValidateManifestHash`). Only after both succeed is the manifest trusted.

The agent has no other way to authenticate the manifest. **If the agent trusted whatever the C2D payload claimed, anyone able to inject a desired-property update on a device twin could install arbitrary code.**

Root keys are the **anchors of trust** that break this circular problem. They are the only piece of cryptographic material baked into the agent binary at build time, and they are the only keys the agent trusts a priori. Every other key used in the verification pipeline (signing keys, intermediate keys) ultimately derives its trust from one of these embedded root keys.

Concretely, root keys answer one question: *"Should this update-manifest signature be trusted?"* Without them, the entire JWS chain in [`src/utils/jws_utils/src/jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) (`VerifyJWSWithSJWK` → `VerifySJWK` → `VerifyJWSWithKey`) has nothing to anchor against.

## 2. What is a root-key package, and who creates it?

A **root-key package** is a signed JSON document, published by the operator of the Device Update *service* (not by the device or its owner), that tells every agent in the fleet three things:

1. "Here is the current set of trusted root public keys."
2. "Here are root keys that have been revoked."
3. "Here are intermediate signing keys that have been revoked."

The agent fetches this document on **every** deployment (before any manifest processing — see [§6.1](#61-the-download-trigger)), validates it against the keys baked into its binary, and replaces its on-disk copy. It is the *only* mutable source of trust state the agent has; everything else (hardcoded keys, code) is fixed at build time.

### Who creates it?

The entity that operates the trust infrastructure for the agent fleet — i.e., whoever owns the private root keys and the package-signing pipeline. In practice:

- **For the standard Microsoft-hosted Azure IoT Hub Device Update service** (what almost all readers are running): **Microsoft** creates, signs, and publishes the packages. Microsoft holds the private root keys in HSMs and publishes the signed JSON documents under `*.b.nlu.dl.adu.microsoft.com`. The default URL hardcoded into the `rootkey_validator` tool ([`tools/rootkey_validator/main.cpp`](../../tools/rootkey_validator/main.cpp)) is:

  ```
  http://granite-iothub-aat-dui--granite-iothub-aat-du.b.nlu.dl.adu.microsoft.com
    /SouthCentralUS/rootkeypackages/rootkeypackage-2.json
  ```

- **For private or forked ADU deployments** (e.g., a customer who builds their own agent from this repo with their own root keys embedded): the operator that owns the build owns the package. They would replace the contents of [`src/utils/root_key_utils/src/root_key_list.c`](../../src/utils/root_key_utils/src/root_key_list.c) with their own keys and stand up their own signing and hosting pipeline.

**Customers consuming Azure ADU as a service (Contoso and similar) do *not* create root-key packages.** When a customer publishes an update via the Azure portal, CLI, or REST API, what they upload is the *update content* (e.g., a `.deb` file) and an *update import manifest* describing it. The Microsoft-operated service generates the on-the-wire update manifest, signs it with Microsoft-managed signing keys, and publishes it to the device twin. The root-key package sits one layer above that — it is the infrastructure that lets devices trust *Microsoft's* signing keys, and it is rotated only by Microsoft. A customer who discovers (say) a signing-key compromise would need to escalate to Microsoft; they cannot publish a revocation themselves.

The C2D message that arrives on the device contains a `rootKeyPackageUrl` field ([`update_content.h`](../../src/adu_types/inc/aduc/types/update_content.h)) that the service populates with the URL of the currently-published package. The agent has a build-time `ADUC_ROOTKEY_PKG_URL_OVERRIDE` ([`CMakeLists.txt`](../../CMakeLists.txt)) that can pin the URL for testing or air-gapped scenarios; if set, it wins over whatever the C2D message says.

### How is the package constructed?

The agent source does not contain package-generation tooling — that lives on the service side and is not open-sourced. From the agent's validation logic (see [§5](#5-anatomy-of-a-root-key-package) for the schema) the process is unambiguous:

1. The service operator builds a `protected` JSON object: current `rootKeys` map, current `disabledRootKeys` / `disabledSigningKeys` lists, incremented `version`, current `published` timestamp, and the `isTest` flag.
2. The operator serializes `protected` to a deterministic byte sequence (the agent verifies a byte-equivalent copy, so canonicalisation matters — see the equality-check note in [§10](#10-observations-and-limitations)).
3. For **each** root key whose private half the operator holds *and* whose public half is hardcoded in the deployed agent fleet, the operator signs the serialized `protected` block with RS256 and appends the signature to the `signatures` array (in the same index order as `protected.rootKeys` — see the positional-coupling warning in [§5](#5-anatomy-of-a-root-key-package)).
4. The signed package is published at the URL the service then advertises via `rootKeyPackageUrl`.

### What is *not* in the package, and why

It is worth being explicit about the negative space:

- **No per-device or per-customer data.** Every device in the fleet downloads the same package. The package is fleet-global; targeting happens at the *update-manifest* layer, not here.
- **No update content or manifest content.** This package is purely trust infrastructure — keys and revocation lists.
- **No certificate chains in the PKI sense.** It is a flat list of RSA public keys with key IDs, plus revocation flags. There is no notion of issuer, validity period, or X.509 here.
- **No customer-controlled signing.** A customer's own signing keys (e.g., the keys that sign the customer's APT repo) live entirely outside this trust chain.

### What does it contain (preview)?

At a high level (full schema and field semantics are in [§5](#5-anatomy-of-a-root-key-package)):

| Field | Purpose |
|-------|---------|
| `protected.rootKeys` | Currently-trusted root public keys, keyed by kid |
| `protected.disabledRootKeys` | Kids of root keys that should be denied at runtime |
| `protected.disabledSigningKeys` | SHA-256 hashes of intermediate signing-key public keys that should be denied |
| `protected.version`, `protected.published`, `protected.isTest` | Metadata |
| `signatures[]` | One RS256 signature per hardcoded root key, over the serialized `protected` block (positional with `rootKeys`) |

The signed document is delivered as a standalone JSON file at the URL above. It is **not** itself a JWS; it is a plain JSON document with a custom signatures array, which the agent validates with its own loop (see [§5](#5-anatomy-of-a-root-key-package) and [§6.2](#62-successful-download--storage-and-runtime-use)). This is a different signing convention than the update manifest (which uses a detached compact JWS — see [§1](#1-why-root-keys-matter)).

## 3. The trust chain at a glance

```
[Agent binary]                                [Service / Cloud]
+----------------+                            +------------------+
| Hardcoded RSA  |  <-- self-validates --     | Root key package |
| root keys      |      (every embedded key   | (JSON + multiple |
| K1, K2         |       must have a valid    |  RS256 sigs over |
+--------+-------+       signature)           |  protected{...}) |
         |                                    +---------+--------+
         | (kid lookup)                                 |
         v                                              v
+----------------+                            +------------------+
| Disk store of  |  <-- replaces -----------  | C2D message      |
| latest         |                            |  updateManifest: |
| root-key pkg   |                            |    "<json str>"  |
+--------+-------+                            |  updateManifest- |
         |                                    |    Signature:    |
         | (used to verify SJWK in            |    "<detached    |
         |  manifest signature header)        |     JWS>"        |
         v                                    +---------+--------+
+----------------+                                      |
| SJWK in JWS    | <----- verified --------------- ----+
| header valid   |        (signed by trusted root key)
+--------+-------+
         |
         v
+----------------+
| Signing key    | <-- check vs. disabledSigningKeys
| in SJWK valid  |     (SHA-256 of pub key)
+--------+-------+
         |
         v
+----------------+
| JWS payload    | <-- VerifyJWSWithKey(sig, signingKey)
| signature valid|     payload = { "sha256": "<hash>" }
+--------+-------+
         |
         v
+----------------+
| SHA-256 of     | <-- Json_ValidateManifestHash
| updateManifest |     re-hashes the manifest string,
| string matches |     compares against payload hash
| payload hash   |
+----------------+
         |
         v
   Manifest now trusted; per-payload SHA-256s
   inside the manifest are checked at download time
```

The independent verification hops are:

1. **Root-key package self-validation** — the downloaded root-key JSON contains a `signatures` array. For **every** hardcoded root key in the agent binary, the package must contain a matching valid RS256 signature over its `protected` properties. ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c))
2. **SJWK validation** — the JWS in `updateManifestSignature` carries an `sjwk` header parameter (a "Signed JSON Web Key" — itself a JWS whose payload is a JWK). The SJWK's own header carries a `kid` that identifies which **root key** signed it. The agent looks up that key in `RootKeyUtility_GetKeyForKid`, which checks both the hardcoded list and the on-disk package's `rootKeys`, while rejecting any kid present in the package's `disabledRootKeys`. ([`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c), [`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c))
3. **Manifest-signature validation** — once the SJWK is trusted, the signing key it carries is used to verify the `updateManifestSignature` JWS itself via `VerifyJWSWithKey`. ([`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c))
4. **Manifest-content binding** — after the JWS is verified, the agent decodes the JWS payload (a JSON object with a `sha256` field) and re-hashes the `updateManifest` string. The two SHA-256 values must match, or the manifest is rejected. ([`workflow_utils.c`](../../src/utils/workflow_utils/src/workflow_utils.c) — `Json_ValidateManifestHash`, called from `workflow_validate_update_manifest_signature`)

Step 2 also runs `IsSigningKeyDisallowed`, which SHA-256-hashes the SJWK public key and rejects it if the hash is in the package's `disabledSigningKeys` list. ([`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c))

## 4. The two hardcoded root keys

The agent binary embeds exactly **two** RSA root keys, defined in [`src/utils/root_key_utils/src/root_key_list.c`](../../src/utils/root_key_utils/src/root_key_list.c):

| Build | Key IDs |
|-------|---------|
| **Prod** (default) | `ADU.200702.R`, `ADU.200703.R` |
| **Test** (`EMBED_TEST_ROOT_KEYS=1`) | `ADU.200702.R.T`, `ADU.200703.R.T` |

Each key is an `RSARootKey` struct ([`root_key_list.h`](../../src/utils/root_key_utils/inc/root_key_list.h)):

```c
typedef struct tagRSARootKey {
    const char* kid;        // e.g., "ADU.200702.R"
    const char* N;          // RSA modulus, base64url-encoded
    const unsigned int e;   // RSA public exponent, 65537 for both keys
} RSARootKey;
```

The test/prod split is controlled by the CMake flag `ADUC_USE_TEST_ROOT_KEYS`, which is propagated as the compile definition `EMBED_TEST_ROOT_KEYS`. See [`src/CMakeLists.txt`](../../src/CMakeLists.txt) and [`src/utils/root_key_utils/CMakeLists.txt`](../../src/utils/root_key_utils/CMakeLists.txt).

### Why two keys, not one?

**`RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys()` requires that the downloaded root-key package contain a valid signature for *every* hardcoded key**, not just one. The loop is unambiguous ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)):

```c
for (size_t i = 0; i < numHardcodedKeys; ++i)
{
    const RSARootKey rootKey = hardcodedRsaKeys[i];
    ADUC_Result validationResult = RootKeyUtility_ValidatePackageWithKey(rootKeyPackage, rootKey);
    if (IsAducResultCodeFailure(validationResult.ResultCode))
    {
        Log_Error("Failed validate pkg with key, ERC: 0x%08x", validationResult.ExtendedResultCode);
        result = validationResult;
        goto done;   // ANY missing/invalid signature kills the package
    }
}
```

The practical consequences of "both must sign":

1. **Defence in depth against single-key compromise.** An attacker who steals one private key still cannot forge a root-key package the agent will accept — they need to steal **both** keys, presumably held in separate HSMs / by separate teams.
2. **Migration runway during key rotation.** When key `K_old` is being retired in favour of `K_new`, the service can publish packages signed by `{K_old, K_new}` for a transition window. Older agents (which only know `K_old, K_other`) keep accepting packages, while newer agents (which know `K_other, K_new`) can also accept them — provided both windows' hardcoded sets overlap with the published signature set. See [§7.4](#74-introducing-a-new-root-key) for the full mechanics.
3. **No "either-or" laxness.** The agent will not silently fall back to a single key if the other's signature is missing or invalid. This is intentional — it makes the security guarantee easy to reason about.

## 5. Anatomy of a root-key package

A root-key package is a JSON document with two top-level objects: `protected` (the data) and `signatures` (an array of RS256 signatures over the serialised `protected` block). The schema is in [`rootkeypackage.schema.json`](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage.schema.json) and the parsed C representation is in [`rootkeypackage_types.h`](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage_types.h).

Real-shaped example (from [test data](../../src/utils/rootkeypackage_utils/tests/testdata/rootkeypackage_utils/rootkeypackage.json)):

```jsonc
{
    "protected": {
        "isTest": false,                          // true => test pkg, must match agent build
        "version": 1,                             // documented as monotonic (see §9)
        "published": 1667343602,                  // unix time when service published it

        "disabledRootKeys": ["rootkey2"],         // kids of revoked root keys

        "disabledSigningKeys": [                  // SHA-256 hashes of revoked
            { "alg": "SHA256",                    // intermediate signing keys
              "hash": "sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E" }
        ],

        "rootKeys": {                             // The currently-trusted root keys
            "rootkey1": { "keyType": "RSA",       // (clients use these to verify
                          "n": "...",             //  SJWKs in update manifests)
                          "e": 65537 },
            "rootkey2": { "keyType": "RSA", "n": "...", "e": 65537 }
        }
    },
    "signatures": [                               // RS256 signatures of the
        { "alg": "RS256", "sig": "..." },         //   serialised `protected` block.
        { "alg": "RS256", "sig": "..." }          // One per hardcoded key the
    ]                                             // package targets.
}
```

The parsed in-memory shape is `ADUC_RootKeyPackage` ([`rootkeypackage_types.h`](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage_types.h)):

```c
typedef struct tagADUC_RootKeyPackage {
    ADUC_RootKeyPackage_ProtectedProperties protectedProperties;
    STRING_HANDLE                            protectedPropertiesJsonString;  // verbatim, for signature check
    VECTOR_HANDLE                            signatures;                     // of ADUC_RootKeyPackage_Signature
} ADUC_RootKeyPackage;
```

### Field semantics

| Field | Meaning |
|-------|---------|
| `isTest` | Tags the package as a test or prod package. The agent enforces a strict match: a prod-build agent **rejects** test packages and a test-build agent rejects prod packages — **unless** the build defines `ADUC_ENABLE_SRVC_E2E_TESTING`, in which case the gating is bypassed entirely. See the precise flag matrix in [§6.2](#62-successful-download--storage-and-runtime-use) and the gate at [`rootkey_workflow.c`](../../src/rootkey_workflow/src/rootkey_workflow.c). |
| `version` | Service-assigned, documented as "monotonic increasing" in the type comment. The agent **parses** this but does not enforce monotonicity locally — see [§10](#10-observations-and-limitations) for the security implications. |
| `published` | Unix timestamp the service published this package. Informational. |
| `disabledRootKeys` | Array of kids. Any root kid here is **unusable for verifying SJWKs in update manifests**, even if its public key is still listed in `rootKeys`. See [§7.3](#73-revoking-a-root-key-one-of-two). |
| `disabledSigningKeys` | Array of `{alg, hash}` objects. Each hash is the SHA-256 of an intermediate signing-key public key (the JWK carried in an SJWK). See [§7.2](#72-revoking-a-signing-key-intermediate). |
| `rootKeys` | The current set of trusted root keys, keyed by kid. The agent looks here during SJWK validation if the kid is not in the hardcoded list. **The enumeration order of this object matters** — see the signature-positioning note immediately below. |
| `signatures[]` | RS256 signatures over the byte-exact serialised `protected` block. **The agent requires one valid signature per *hardcoded* root key.** Extra signatures (e.g., from new keys the agent doesn't know) are allowed and ignored. |

> **⚠️ Signatures are positionally coupled to `rootKeys`.** The validator (`RootKeyUtility_GetSignatureForKey` at [`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)) locates a hardcoded key's signature by finding the key's *index* in `protected.rootKeys`, then indexing into `signatures[]` with that same index ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). In practice this means: the *N*th signature must correspond to the *N*th key declared in `protected.rootKeys`. Reordering `rootKeys` without correspondingly reordering `signatures` will cause every hardcoded key to verify against the wrong signature and fail. Adding a *new* key entry at index *N* without inserting its signature at index *N* will similarly break verification on all older agents.

> **⚠️ Only RS256 is honoured for package self-validation.** Although the schema permits `RS384` and `RS512` in `signatures[].alg` ([schema](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage.schema.json)) and the parser accepts all three, the validator always calls `CryptoUtils_IsValidSignature(CRYPTO_UTILS_SIGNATURE_VALIDATION_ALG_RS256, ...)` regardless of the parsed `alg` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). Packages signed with RS384/RS512 will be rejected. Treat RS256 as a hard requirement until that hardcoded constant changes.

## 6. How the agent uses the root key for content verification

### 6.1 The download trigger

When the cloud sends a deployment via desired-property update, the `OrchestratorUpdateCallback` in [`adu_core_interface.c`](../../src/agent/adu_core_interface/src/adu_core_interface.c) extracts two fields from the *unprotected* portion of the workflow message:

- `workflowId` — used to namespace the temp download directory.
- `rootKeyPackageUrl` — the JSON property name is defined as `ADUCITF_FIELDNAME_ROOTKEY_PACKAGE_URL` in [`update_content.h`](../../src/adu_types/inc/aduc/types/update_content.h).

If the action is `ProcessDeployment`, the orchestrator calls `RootKeyWorkflow_UpdateRootKeys(workflowId, workFolder, rootKeyPkgUrl)` ([`adu_core_interface.c`](../../src/agent/adu_core_interface/src/adu_core_interface.c)) — **before any manifest processing**, because the manifest can't be verified without an up-to-date root-key store. The deployment is aborted if this call fails:

```c
tmpResult = RootKeyWorkflow_UpdateRootKeys(workflowId, workFolder, rootKeyPkgUrl);
if (IsAducResultCodeFailure(tmpResult.ResultCode))
{
    Log_Error("Update Rootkey failed, 0x%08x. Deployment cannot proceed.",
              tmpResult.ExtendedResultCode);
    goto done;
}
```

The download itself is delegated to a pluggable downloader (Delivery Optimization by default; curl when the build flag `ADUC_ROOTKEY_PKG_DOWNLOAD_WITH_CURL` is on — see [How to build](how-to-build-agent-code.md#use-curl-for-rootkey-package-download-instead-of-delivery-optimization-agent-do)). The dispatch happens in `ADUC_RootKeyPackageUtils_DownloadPackage` ([`rootkeypackage_download.c`](../../src/utils/rootkeypackage_utils/src/rootkeypackage_download.c)), which:

1. Creates `<workFolder>/<workflowId>/` as the download sandbox.
2. Honours the build-time `ADUC_ROOTKEY_PKG_URL_OVERRIDE` if set, otherwise uses the URL from the C2D message. (Useful for pinning during testing — see [`CMakeLists.txt`](../../CMakeLists.txt).)
3. **Force-downloads** the file (no hash check at this stage — the package's `signatures` field provides self-referential integrity, so a tampered download will fail signature validation below).

### 6.2 Successful download — storage and runtime use

`RootKeyWorkflow_UpdateRootKeys` ([`rootkey_workflow.c`](../../src/rootkey_workflow/src/rootkey_workflow.c)) drives the post-download pipeline:

1. **Parse** the JSON into an `ADUC_RootKeyPackage` (`ADUC_RootKeyPackageUtils_Parse`).
2. **Validate signatures with hardcoded keys** (`RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys`). This is the moment of bootstrap — see [§4](#4-the-two-hardcoded-root-keys).
3. **Enforce test/prod separation** based on `isTest` vs the agent's build-time test flags. The exact behaviour, captured at [`rootkey_workflow.c`](../../src/rootkey_workflow/src/rootkey_workflow.c), is:

   | Build flags defined | Behaviour for prod pkg (`isTest=false`) | Behaviour for test pkg (`isTest=true`) |
   |---------------------|------------------------------------------|----------------------------------------|
   | None (default prod build) | Accept | **Reject** (`ADUC_ERC_ROOTKEY_TEST_PKG_ON_PROD_AGENT`) |
   | `ADUC_E2E_TESTING_ENABLED` (set by CMake `-DADUC_ENABLE_E2E_TESTING=ON`) | **Reject** (`ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT`) | Accept |
   | `ADUC_ENABLE_SRVC_E2E_TESTING` (CMake `-DADUC_ENABLE_SRVC_E2E_TESTING=ON`) | Accept | Accept (entire gate is skipped via `#ifndef`) |

   **The test/prod gate is independent of the hardcoded root-key set.** Whether the binary embeds prod or test root keys is controlled by the separate flag `ADUC_USE_TEST_ROOT_KEYS` (compile def `EMBED_TEST_ROOT_KEYS`) — see [`CMakeLists.txt`](../../CMakeLists.txt) and [`src/CMakeLists.txt`](../../src/CMakeLists.txt). You can in principle mix and match (e.g. a build with prod root keys but the e2e test gate enabled), though only the matrix above is exercised in CI.
4. **Compare** to the on-disk store using `ADUC_RootKeyUtility_IsUpdateStoreNeeded` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). This does a full structural equality check via `ADUC_RootKeyPackageUtils_AreEqual`. If equal, the workflow returns `ADUC_Result_RootKey_Continue` with ERC `ADUC_ERC_ROOTKEY_PKG_UNCHANGED` — see [§6.4](#64-the-package-unchanged-path).
5. **Atomic write** via `RootKeyUtility_WriteRootKeyPackageToFileAtomically` — writes to `<store>.json-temp`, then renames over `<store>.json`. ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c))
6. **Reload** the package into the process-wide `s_localStore` via `RootKeyUtility_ReloadPackageFromDisk(..., validateSignatures=true)` — re-validating signatures on the just-written file as a paranoia check.

**On-disk location.** Both paths are CMake-configured constants compiled into the binary ([`CMakeLists.txt`](../../CMakeLists.txt)):

```
ADUC_DATA_FOLDER            = /var/lib/adu                              (default)
ADUC_ROOTKEY_STORE_PATH     = /var/lib/adu/rootkeystore                 (directory)
ADUC_ROOTKEY_STORE_PACKAGE_PATH = /var/lib/adu/rootkeystore/rootkeys.json  (file)
```

**Runtime use during manifest verification.** With `s_localStore` populated, the agent can verify update manifests. When the deployment arrives, the C2D message carries two separate fields: `updateManifest` (a stringified JSON manifest) and `updateManifestSignature` (a detached compact JWS whose payload is `{"sha256": "<hash of updateManifest string>"}`). Verification is driven by `workflow_validate_update_manifest_signature` in [`workflow_utils.c`](../../src/utils/workflow_utils/src/workflow_utils.c) and proceeds as follows:

| Step | Code | What it does |
|------|------|--------------|
| 1 | `VerifyJWSWithSJWK(manifestSignature)` — [`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) | Pulls the `sjwk` field from the JWS header. |
| 2 | `VerifySJWK(sjwk)` — [`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) | Reads the `kid` from the SJWK's own JWS header. |
| 3 | `RootKeyUtility_GetKeyForKid(&rootKey, kid)` — [`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c) | First looks up `kid` in the hardcoded list, then in `s_localStore->protectedProperties.rootKeys`. **Rejects if `kid` appears in `s_localStore->protectedProperties.disabledRootKeys`**. Lazily loads `s_localStore` from disk if not yet loaded. |
| 4 | `VerifyJWSWithKey(sjwk, rootKey)` | RS256-verifies the SJWK against the resolved root key. |
| 5 | `RootKeyUtility_GetDisabledSigningKeys(...)` | Fetches the on-disk `disabledSigningKeys` list. |
| 6 | `IsSigningKeyDisallowed(payload, list)` — [`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) | Builds an RSA pub key from `(n,e)` in the SJWK payload, SHA-256-hashes it, and rejects if the hash matches any entry in the list. |
| 7 | `GetKeyFromBase64EncodedJWK(sjwk)` then `VerifyJWSWithKey(manifestSignature, key)` | Verifies the `updateManifestSignature` JWS itself using the now-trusted signing key. After this, the JWS payload (a small JSON object with a `sha256` field) is cryptographically attested — but the manifest *string* is not yet bound to it. |
| 8 | `Json_ValidateManifestHash(updateActionObject)` — [`workflow_utils.c`](../../src/utils/workflow_utils/src/workflow_utils.c) | Decodes the JWS payload, extracts the `sha256` field, and SHA-256-hashes the verbatim `updateManifest` string from the C2D message. If the two hashes match, the manifest is bound to the verified signature. If not, validation fails with `ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED`. |

After all that, the manifest can be parsed. Per-payload SHA-256 hashes declared in the manifest are then checked after each file is downloaded — see the Security Model in [architecture-overview.md](architecture-overview.md#security-model).

### 6.3 Failed download or validation

If any step in `RootKeyWorkflow_UpdateRootKeys` fails, the function returns a failure `ADUC_Result` and the orchestrator aborts the deployment **without** attempting manifest verification ([`adu_core_interface.c`](../../src/agent/adu_core_interface/src/adu_core_interface.c)):

```c
if (IsAducResultCodeFailure(tmpResult.ResultCode))
{
    Log_Error("Update Rootkey failed, 0x%08x. Deployment cannot proceed.",
              tmpResult.ExtendedResultCode);
    goto done;
}
```

The extended result code is also stashed via `RootKeyUtility_SetReportingErc()` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)) and later picked up by the workflow reporter in [`agent_workflow.c`](../../src/adu_workflow/src/agent_workflow.c) — which means the failure is **surfaced to IoT Hub as part of the deployment's reported properties** (visible to the operator via "Last Attempted Update" details).

**Key consequences of a failed root-key update:**

- The agent does **not** fall back to the previous on-disk store for verifying *this* deployment. The deployment fails fast.
- The previous on-disk store is **not modified** — atomic rename means a failed download/parse/validation leaves the existing `rootkeys.json` intact.
- The next deployment will re-attempt the download from scratch.

See [§8](#8-failure-mode-reference) for the full list of failure ERCs.

### 6.4 The "package unchanged" path

If the just-downloaded package is structurally equal to the on-disk store, the function short-circuits ([`rootkey_workflow.c`](../../src/rootkey_workflow/src/rootkey_workflow.c)):

```c
if (!ADUC_RootKeyUtility_IsUpdateStoreNeeded(fileDest, &rootKeyPackage))
{
    Log_Debug("RootKey pkg unchanged, continuing...");
    result.ResultCode        = ADUC_Result_RootKey_Continue;     // == 1200
    result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PKG_UNCHANGED;  // == 0xa0000fff
    goto done;
}
```

`ADUC_Result_RootKey_Continue` is a **success-class** result code (see [`adu_core.h`](../../src/adu_types/inc/aduc/types/adu_core.h)), so the orchestrator's `IsAducResultCodeFailure` check passes and the deployment proceeds. The informational ERC is appended via `workflow_add_erc(nextWorkflow, rootkeyErc)` ([`agent_workflow.c`](../../src/adu_workflow/src/agent_workflow.c)) so operators can see that the rootkey workflow ran and was a no-op, which is useful for diagnosing "is my rootkey workflow even running?" questions in the field.

---

## 7. Root-key rotation and revocation

This section is what the solution operator (Contoso) needs to plan for. The agent supports two distinct revocation primitives, plus a key-introduction flow that requires careful coordination with agent releases.

### 7.1 Two independent revocation lists

A single root-key package carries **two** lists, and they revoke different things:

| List | Field | What it disables | Where it's enforced |
|------|-------|------------------|---------------------|
| Root-key revocation | `protected.disabledRootKeys` | Use of a **root key** (by `kid`) to verify SJWKs in update manifests | `RootKeyUtility_RootKeyIsDisabled` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)), called by `RootKeyUtility_GetKeyForKid` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)) |
| Signing-key revocation | `protected.disabledSigningKeys` | Use of a specific **intermediate signing key** (by SHA-256 of public key) | `IsSigningKeyDisallowed` ([`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c)) |

The lists are **completely independent**. Neither has any effect on whether a root-key package is *itself* accepted — package self-validation is performed exclusively against the agent's *hardcoded* keys (see [§4](#4-the-two-hardcoded-root-keys)). This separation is critical: it means a compromised key can be marked disabled in a package that is still validly signed by that same compromised key, without creating a circular dependency.

### 7.2 Revoking a signing key (intermediate)

This is the **routine** case: you've issued a signing certificate, it's used for a while, you want to retire it (scheduled rotation) or you discovered it's compromised. The flow:

1. **Cloud side:** publish a new root-key package whose `disabledSigningKeys` array includes the SHA-256 hash of the compromised/retiring signing key's public key.
2. The new package's `version` is incremented and `published` is updated.
3. The package is signed (RS256) by **both** currently-active root keys — same as every other package.
4. **No agent change required.** The agent picks up the new package on its next deployment's `RootKeyWorkflow_UpdateRootKeys` call.
5. From that point, **as long as the agent's on-disk store is the new package**, any manifest signed with that signing key is rejected at [`jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) with `JWSResult_DisallowedSigningKey`.

This is the typical operating-procedure response to a signing-key compromise and requires **no** changes to the embedded root keys.

> **⚠️ Revocation is not rollback-resistant.** Step 5's "as long as" is load-bearing. Because the agent does not enforce monotonic `version` (see [§10](#10-observations-and-limitations)), an attacker who can substitute the delivered root-key package URL — e.g. by tampering with the C2D message or hijacking the download URL — can serve an older, still-validly-signed package whose `disabledSigningKeys` list does *not* yet include the compromised key. The agent will accept the older package (it parses, all hardcoded signatures verify) and silently undo the revocation. Operationally this means: signing-key revocation depends on the C2D channel and the package-delivery channel being trustworthy. If those are also compromised, revocation alone cannot save you — see [§10](#10-observations-and-limitations) for mitigations.

### 7.3 Revoking a root key (one of two)

Scenario: one of the two currently-active root keys (say `ADU.200702.R`) is compromised or scheduled for retirement. **This is fundamentally harder than signing-key revocation** because of the bootstrap constraint: the agent's hardcoded list still requires the compromised key to sign every new root-key package.

**What the operator can do today, using only the package mechanism:**

1. Publish a new root-key package whose `disabledRootKeys` array includes `"ADU.200702.R"`.
2. The package is still signed by **both** `ADU.200702.R` and `ADU.200703.R` (the agent's hardcoded keys); the agent's hardcoded validator does not care that one of them is being disabled.
3. After the agent stores this new package, `RootKeyUtility_GetKeyForKid` will refuse to return `ADU.200702.R` even though it's hardcoded. New SJWKs that name `ADU.200702.R` in their `kid` will fail with `JWSResult_DisallowedRootKid` — **as long as the agent's on-disk store is the new package** (see the rollback caveat in [§7.2](#72-revoking-a-signing-key-intermediate); it applies equally here).
4. From now on, the cloud should sign SJWKs only with `ADU.200703.R`. Older SJWKs signed by `ADU.200702.R` (if any are still cached anywhere) become unusable.

**What the operator *cannot* do without an agent rebuild:**

- **Stop requiring `ADU.200702.R` to sign root-key packages.** The validator loop iterates the hardcoded list, full stop. As long as `ADU.200702.R` is in the binary, every root-key package must still bear its signature — even after it's listed in `disabledRootKeys`. This means the compromised key cannot be retired from the *bootstrap* role; it can only be retired from the *runtime-verification* role.
- **Drop the compromised key from the binary.** This requires a new agent release with the key removed from [`root_key_list.c`](../../src/utils/root_key_utils/src/root_key_list.c) and a coordinated rollout (via OS package manager or via an ADU update signed by the still-trusted `ADU.200703.R`).

In summary: **`disabledRootKeys` is a *runtime-deny* mechanism, not a *bootstrap-remove* mechanism**. The hardcoded list is the source of truth for what signs the root-key package. And because there is no monotonic version check, the runtime-deny is only as durable as the integrity of the package-delivery channel.

### 7.4 Introducing a new root key

Scenario: Contoso wants to add a new root key `ADU.301010.R` to the trust set (for future rotation, or to replace a key being phased out).

The cloud-side aspect of this is relatively cheap: the service can include an extra `K_new` entry in `protected.rootKeys` and an extra `K_new`-signed entry in `signatures[]` of any new package. **Older agents will simply not look at K_new** — their hardcoded loop only verifies signatures for the keys they know, so the extra entry is harmless (subject to the positional-coupling rule called out in [§5](#5-anatomy-of-a-root-key-package): the *order* of `rootKeys` and `signatures` must match).

What requires an **agent binary update** is the moment you want the service to be able to *require* `K_new` — i.e. publish a package that omits one of the older keys' signatures. From then on, only agents whose hardcoded list includes `K_new` will accept the package. The general migration pattern:

1. **Generate** the new key pair `(ADU.301010.R)` in the cloud-side HSM.
2. **Start including** a `K_new` entry and `K_new`-signed signature in every published package right away. Older agents ignore it; this is preparation for the rollout.
3. **Release a new agent version `v_next`** whose hardcoded list is the *union*: `{ADU.200702.R, ADU.200703.R, ADU.301010.R}`. This agent will *require all three* signatures on every root-key package it accepts. The service must therefore continue signing with all three for the entire migration window.
4. **Roll out** `v_next` to the device fleet using the existing trust chain.
5. **(Optional) Phase out an old key.** Once the fleet has reached `v_next`, the service can stop signing with (say) `ADU.200702.R` only by simultaneously shipping `v_next+1` whose hardcoded list drops it: `{ADU.200703.R, ADU.301010.R}`. The package-signing set similarly shrinks. **Any device that hasn't yet upgraded past `v_next` will start rejecting packages — so the rollout must be fully complete before signatures are dropped.**

**Key principles:**

- Adding a key to the *service's* package is cheap and non-breaking.
- Adding a key to an *agent's hardcoded list* makes that key a hard requirement for that agent — so the service must keep signing with it until the agent is retired.
- Removing a key from an agent's hardcoded list must be sequenced **after** every device has upgraded past the version that required it.

### 7.5 What if both hardcoded root keys are compromised at once?

This is the catastrophic case. **Once both private keys are in adversary hands, the root-key package mechanism provides no defence**, because:

- An attacker can forge a malicious root-key package signed by both compromised keys.
- The agent's hardcoded validator will accept it as authentic.
- The malicious package can install attacker-chosen `rootKeys`, leave the existing `disabledRootKeys` empty, and arrange that the SJWK in subsequent forged manifests passes verification.

The recovery path **must occur outside the ADU agent itself**:

1. **Ship a new agent binary** with a new hardcoded key set `{K_new1, K_new2}` (and ideally with the compromised keys explicitly listed for runtime denial too, defence-in-depth).
2. **Distribute the new agent via a non-ADU channel** — typically OS package updates (`apt`, `dnf`, manufacturer OTA, MDM). The compromised ADU trust chain cannot be safely used to ship its own replacement.
3. After installation, the new agent ignores the old hardcoded keys and bootstraps trust from `{K_new1, K_new2}`.

This is why **the two hardcoded keys should be managed under fully separated controls**: separate HSMs, separate operator personas, separate audit trails, ideally separate geographic locations. A simultaneous compromise should require breaching two independent systems, not one.

---

## 8. Failure-mode reference

### Root-key workflow ERCs (component `0xa` = `ADUC_COMPONENT_ROOTKEY_WORKFLOW`)

From [`src/inc/aduc/result.h`](../../src/inc/aduc/result.h):

| Constant | Hex | When it fires |
|----------|-----|---------------|
| `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE` | `0xa0000001` | Downloaded file is not valid JSON |
| `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE` | `0xa0000002` | Parsed JSON couldn't be re-serialised (OOM-class) |
| `ADUC_ERC_ROOTKEY_STORE_PATH_CREATE` | `0xa0000003` | Could not `mkdir -p` the rootkey store directory |
| `ADUC_ERC_ROOTKEY_SIGNINGKEY_DISABLE_EVAL_INVALID_HASHALG` | `0xa0000004` | A `disabledSigningKeys` entry uses an unsupported hash algorithm |
| `ADUC_ERC_ROOTKEY_SIGNING_KEY_IS_DISABLED` | `0xa0000005` | Manifest's signing key matched the `disabledSigningKeys` list |
| `ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT` | `0xa0000006` | Test-build agent received a prod package (`isTest=false`) |
| `ADUC_ERC_ROOTKEY_TEST_PKG_ON_PROD_AGENT` | `0xa0000007` | Prod-build agent received a test package (`isTest=true`) |
| `ADUC_ERC_ROOTKEY_PACKAGE_CHANGED` | `0xa0000008` | Informational: store was updated this deployment |
| `ADUC_ERC_ROOTKEY_PKG_UNCHANGED` | `0xa0000fff` | Informational: store unchanged, deployment continues |

### Root-key utility ERCs (component `0x5` = `ADUC_COMPONENT_ROOTKEYPKG_UTIL`)

Notable ones surfaced by the verification chain:

| Constant | Meaning |
|----------|---------|
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_FOR_KEY_NOT_FOUND` | Hardcoded key `kid` has no matching entry in `signatures[]` |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED` | RS256 verification of `protected` block failed |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID` | Manifest's SJWK `kid` is unknown to both hardcoded list and on-disk store |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED` | Manifest's SJWK `kid` is in `disabledRootKeys` |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_LOAD_FROM_STORE` | On-disk store unreadable |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_HARDCODED_ROOTKEY_LOAD_FAILED` | Hardcoded list is empty or malformed (build error) |

### JWS verification results

From [`jws_utils.h`](../../src/utils/jws_utils/inc/jws_utils.h):

| `JWSResult` | Meaning |
|-------------|---------|
| `JWSResult_BadStructure` | Not a well-formed `header.payload.signature` |
| `JWSResult_InvalidSignature` | RS256 check failed |
| `JWSResult_DisallowedRootKid` | SJWK's `kid` matches `disabledRootKeys` |
| `JWSResult_MissingRootKid` | SJWK's `kid` is not known to the agent at all |
| `JWSResult_InvalidRootKid` | Generic failure resolving `kid` |
| `JWSResult_DisallowedSigningKey` | SJWK's public key hash matches `disabledSigningKeys` |

The `rootkey_validator` CLI tool ([`tools/rootkey_validator/README.md`](../../tools/rootkey_validator/README.md)) is the easiest way to reproduce these failures locally against any candidate package.

---

## 9. Operator runbook (cloud-side rotation)

A quick checklist for solution operators (e.g. Contoso) when changes are needed.

### Scenario A — Rotate a signing key (routine, weekly/monthly cadence)

1. Generate the new signing key in the cloud HSM.
2. Start signing **new** update manifests' SJWKs with the new key (the SJWK is signed by one of the *root* keys — see [§6.2](#62-successful-download--storage-and-runtime-use)).
3. Publish a new root-key package with the **old** signing key's public-key SHA-256 added to `disabledSigningKeys`, bump `version` and `published`.
4. Sign the package with both currently-active root keys.
5. Wait until fleet has picked up the new package. No agent change needed.
6. **Rollback caveat:** the agent has no monotonic-version check (see [§10](#10-observations-and-limitations)). If the C2D channel or download URL can be tampered with, an attacker can serve an older still-validly-signed package that lacks the new `disabledSigningKeys` entry, silently undoing the revocation. Ensure the package-delivery path is secured.

### Scenario B — Revoke a root key (one of two is compromised)

1. **Immediately** stop signing new SJWKs with the compromised root key. Use only the other root key.
2. Publish a new root-key package with the compromised `kid` in `disabledRootKeys`, bump `version` and `published`.
3. Sign the package with **both** hardcoded keys (you still need to — see [§7.3](#73-revoking-a-root-key-one-of-two)).
4. Plan an agent release that drops the compromised key from `root_key_list.c` and adds a replacement (see Scenario C).
5. **Rollback caveat applies here too** — see Scenario A step 6. Until the compromised key is removed from the agent binary, runtime denial is the only protection, and it can be undone by package rollback.

### Scenario C — Introduce a new root key (planned rotation)

1. Generate `K_new` in the cloud HSM.
2. Start including a `K_new` entry in `protected.rootKeys` and a `K_new`-signed entry in `signatures[]` of every new package, **keeping the array ordering consistent** (see [§5](#5-anatomy-of-a-root-key-package)). Older agents will ignore the extras.
3. Release agent `v_next` whose hardcoded list is `{K_old1, K_old2, K_new}`. From now on, any agent at `v_next` requires all three signatures.
4. Roll out `v_next` to the fleet. The service must continue signing with all three keys throughout the migration.
5. Once fleet migration is complete and verified, release `v_next+1` that drops the key being retired. Only then can the service stop signing with the retired key. Coordinate carefully — any device left on `v_next` will start failing.

### Scenario D — Both root keys compromised (catastrophe)

1. **Do not** publish anything via ADU — the trust chain is broken.
2. Build and sign a new agent binary with a fresh hardcoded key set out-of-band.
3. Distribute the new agent via the device's OS package channel (`apt`, `dnf`, manufacturer OTA), not via ADU.
4. After all devices are on the new agent, resume normal ADU operation with the new key set.

---

## 10. Observations and limitations

These are notes for future maintainers — behaviours that the current code exhibits which may surprise an operator or warrant future design work.

- **No version-monotonicity check — and this directly weakens revocation.** The `version` field is parsed (`rootkeypackage_parse.c`) but the agent never compares the candidate package's `version` against the on-disk version. A validly-signed older package will be accepted and overwrite a newer on-disk store. The only "did anything change?" check is full structural equality via `ADUC_RootKeyPackageUtils_AreEqual`, which doesn't help with rollback prevention. **The practical impact is that both kinds of package-driven revocation ([§7.2](#72-revoking-a-signing-key-intermediate), [§7.3](#73-revoking-a-root-key-one-of-two)) can be silently undone** by an attacker who can serve a previously-published, still-validly-signed older package — for example by tampering with the C2D message's `rootKeyPackageUrl`, hijacking the download URL, or replaying a captured older package. Mitigations until a monotonic check is added: (1) protect the C2D channel (TLS, IoT Hub authentication), (2) use signed/HTTPS-only download URLs that the service controls, (3) consider setting `ADUC_ROOTKEY_PKG_URL_OVERRIDE` at build time to a pinned trusted URL for high-security deployments.

- **Signatures are positionally coupled to `protected.rootKeys`.** When the validator finds the signature for hardcoded kid `K`, it locates `K` by index within `protected.rootKeys` and then reads `signatures[index]` (see `RootKeyUtility_GetSignatureForKey` and `RootKeyUtility_ValidatePackageWithKey` in [`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). This means package authors must keep the two arrays in lock-step. Inserting or reordering entries in `rootKeys` without correspondingly updating `signatures` will break every hardcoded key's verification, even though the signatures themselves are cryptographically valid. The schema does not document this requirement, and the code does not match signatures by `kid` — so be careful when writing tooling that generates packages.

- **Only RS256 is honoured for package self-validation.** The schema allows `RS256`, `RS384`, `RS512` ([schema](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage.schema.json)) and the parser accepts all three ([`rootkeypackage_parse.c`](../../src/utils/rootkeypackage_utils/src/rootkeypackage_parse.c)), but the validator unconditionally calls RS256 verification regardless of the parsed `alg` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). A package signed with RS384 or RS512 will be rejected with `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED` even though the document is structurally valid. Until the validator honours `alg`, treat RS256 as a hard requirement.

- **Test/prod gating involves three independent build flags.** It is easy to conflate them; the precise meaning of each is:
  - `ADUC_USE_TEST_ROOT_KEYS` (compile def `EMBED_TEST_ROOT_KEYS=1`) controls *which root keys are embedded* in the binary — prod (`ADU.200702.R`/`ADU.200703.R`) or test (`ADU.200702.R.T`/`ADU.200703.R.T`).
  - `ADUC_ENABLE_E2E_TESTING` (compile def `ADUC_E2E_TESTING_ENABLED`) controls the *gating direction*: when defined, prod packages are rejected and test packages accepted; when undefined, the opposite.
  - `ADUC_ENABLE_SRVC_E2E_TESTING` (compile def `ADUC_ENABLE_SRVC_E2E_TESTING`) acts as a **global bypass**: when defined, the entire `isTest` gate is `#ifndef`'d out and any package is accepted regardless of `isTest`.

  These are *orthogonal*. You can build an agent with prod root keys but `ADUC_ENABLE_E2E_TESTING=ON` (it would then only accept test packages — but no test package could possibly be signed by the prod hardcoded keys, so this configuration is functionally unusable). The intended pairings are: prod build = none of these flags; e2e test build = `ADUC_USE_TEST_ROOT_KEYS=ON` + `ADUC_ENABLE_E2E_TESTING=ON`. The service-side E2E flag is for special scenarios only.

- **Equality check is byte-equivalent on the `protected` JSON string.** `ADUC_RootKeyPackageUtils_AreEqual` first compares `protectedPropertiesJsonString` ([`rootkeypackage_utils.c`](../../src/utils/rootkeypackage_utils/src/rootkeypackage_utils.c)). If the service re-serialises the same logical content with different whitespace, the agent will treat it as "changed" and re-write. This is wasted I/O but not a correctness issue.

- **`s_localStore` is a process-global with lazy init.** `RootKeyUtility_GetKeyForKid` will load the store from disk if it hasn't been loaded yet ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)). This means the very first manifest verification after an agent restart pays a disk-read cost. Subsequent verifications are in-memory.

- **Hardcoded list cannot be empty.** The validator returns `ADUC_ERC_UTILITIES_ROOTKEYUTIL_HARDCODED_ROOTKEY_LOAD_FAILED` ([`root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c)) if so. This guards against accidental misconfiguration in `root_key_list.c`.

- **The package self-validation does not consult `disabledRootKeys`.** This is intentional — a hardcoded key cannot bootstrap-disable itself via a package it signs, because then the agent could be locked out by a malicious-but-validly-signed package that disables the only key allowed to sign packages. The hardcoded list is the unconditional bootstrap; `disabledRootKeys` only affects manifest-verification.

- **The test data file `rootkeypackage.json` uses placeholder kids like `"rootkey1"` / `"rootkey2"`.** Real prod packages use the kids from §3 (`ADU.200702.R`, `ADU.200703.R`). The test data exists to exercise parse/equality logic and should not be confused with a real prod package.

---

## Appendix — Quick code-pointer cheat sheet

| Topic | File | Notable functions |
|-------|------|-------------------|
| Hardcoded keys | [`src/utils/root_key_utils/src/root_key_list.c`](../../src/utils/root_key_utils/src/root_key_list.c) | `HardcodedRSARootKeyList[]`, `RootKeyList_GetHardcodedRsaRootKeys` |
| Workflow entry | [`src/rootkey_workflow/src/rootkey_workflow.c`](../../src/rootkey_workflow/src/rootkey_workflow.c) | `RootKeyWorkflow_UpdateRootKeys` |
| Workflow caller | [`src/agent/adu_core_interface/src/adu_core_interface.c`](../../src/agent/adu_core_interface/src/adu_core_interface.c) | `OrchestratorUpdateCallback` |
| Hardcoded validator | [`src/utils/root_key_utils/src/root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c) | `RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys`, `RootKeyUtility_ValidatePackageWithKey` |
| Store load/write | [`src/utils/root_key_utils/src/root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c) | `RootKeyUtility_WriteRootKeyPackageToFileAtomically`, `RootKeyUtility_ReloadPackageFromDisk`, `RootKeyUtility_LoadPackageFromDisk` |
| Runtime key lookup | [`src/utils/root_key_utils/src/root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c) | `RootKeyUtility_GetKeyForKid`, `RootKeyUtility_GetKeyForKidFromHardcodedKeys`, `RootKeyUtility_SearchLocalStoreForKey` |
| Revocation checks | [`src/utils/root_key_utils/src/root_key_util.c`](../../src/utils/root_key_utils/src/root_key_util.c) | `RootKeyUtility_RootKeyIsDisabled`, `RootKeyUtility_GetDisabledSigningKeys` |
| JWS chain | [`src/utils/jws_utils/src/jws_utils.c`](../../src/utils/jws_utils/src/jws_utils.c) | `VerifyJWSWithSJWK`, `VerifySJWK`, `VerifyJWSWithKey`, `IsSigningKeyDisallowed` |
| Download | [`src/utils/rootkeypackage_utils/src/rootkeypackage_download.c`](../../src/utils/rootkeypackage_utils/src/rootkeypackage_download.c) | `ADUC_RootKeyPackageUtils_DownloadPackage` |
| Result codes | [`src/inc/aduc/result.h`](../../src/inc/aduc/result.h) | `ADUC_ERC_ROOTKEY_*` family, `ADUC_Result_RootKey_Continue` |
| Schema | [`src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage.schema.json`](../../src/utils/rootkeypackage_utils/inc/aduc/rootkeypackage.schema.json) | JSON Schema for the package |
| Diagnostic tool | [`tools/rootkey_validator/`](../../tools/rootkey_validator/) | Standalone CLI to download + validate a candidate package |
