# Root Key Package Security System# Root Key Package Security in Device Update Agent



The Azure Device Update (ADU) agent implements a robust security infrastructure based on a root key package system for validating update manifest signatures and ensuring cryptographic integrity throughout the update process.This document explains how the Azure Device Update (ADU) agent downloads and uses root key packages to establish a secure chain of trust for update operations.



## Overview## Overview



The root key package system provides:The root key package system is a critical security component that ensures only authorized and authentic updates can be installed on devices. It establishes a cryptographic chain of trust from hardcoded keys in the agent binary to update content validation.



- **Cryptographic Root of Trust**: Hierarchical key validation starting from hardcoded root keys## Security Architecture

- **Update Manifest Signature Verification**: Validates all update manifests before processing

- **Key Rotation Support**: Supports updating and disabling keys through versioned packages### Trust Chain

- **Secure Download**: Root key packages are downloaded and validated using multiple signatures

- **Test and Production Keys**: Separate key hierarchies for testing and production environmentsThe ADU agent implements a multi-layered security architecture:



## Architecture```

Hardcoded Root Keys (in agent binary)

### Root Key Hierarchy    ↓ (validates)

Downloaded Root Key Package

```    ↓ (validates)

Hardcoded Root Keys (embedded in agent binary)Update Manifest Signatures

    ↓ (validates)    ↓ (authorizes)

Root Key Package (downloaded from service)Update Content

    ↓ (contains)```

Signing Keys (used to validate update manifests)

    ↓ (validates)This chain ensures that:

Update Manifests (contains update instructions)1. Root key packages are validated against immutable keys in the agent

```2. Update manifests are validated against trusted root keys

3. Update content is authorized through cryptographic signatures

### Key Components

## Root Key Package Download Process

1. **Hardcoded Root Keys**: Compiled into the agent binary, these provide the ultimate root of trust

2. **Root Key Package**: Downloaded JSON containing current valid signing keys and disabled key lists### 1. Initiation

3. **Signing Keys**: Keys used to sign update manifests, validated against root key package

4. **Disabled Key Lists**: Mechanisms to revoke compromised keysThe root key download process is triggered during update deployment processing in the main agent workflow:



## Root Key Package Structure- **When**: At the start of `ProcessDeployment` action

- **Where**: In `adu_core_interface.c`

### Package Format- **Requirement**: Must succeed before update manifest validation can proceed



The root key package is a JSON document with the following structure:### 2. Download Configuration



```jsonThe agent supports two download methods:

{

  "protected": "<base64url-encoded protected properties>",#### Delivery Optimization (Default)

  "signatures": [- Uses Microsoft's Delivery Optimization service for efficient downloads

    {- Provides bandwidth optimization and peer-to-peer capabilities

      "alg": "RS256",- Default timeout: 1 hour

      "signature": "<base64url-encoded signature>"- Implementation: `DownloadRootKeyPkg_DO()`

    }

  ]#### cURL Alternative

}- Uses system `curl` command as fallback method

```- Requires `/usr/bin/curl` to be available on the device

- Enabled at build time with `--rootkeypkg-curl` flag

### Protected Properties- Command format: `/usr/bin/curl -L -C - -o <output> -m 3600 <url>`

- Implementation: `DownloadRootKeyPkg_Curl()`

The protected properties contain the actual key data and metadata:

### 3. Download Workflow

```json

{The `RootKeyWorkflow_UpdateRootKeys()` function orchestrates the process:

  "isTest": false,

  "version": 2,1. **Setup**: Configure downloader with workflow ID and work folder

  "published": "2023-10-15T10:30:00Z",2. **Download**: Fetch root key package from URL in deployment metadata

  "disabledRootKeys": [3. **Parse**: Load and validate JSON structure

    "ADU.200702.R"4. **Validate**: Verify package signatures against hardcoded keys

  ],5. **Store**: Atomically update local root key store if package has changed

  "disabledSigningKeys": [6. **Reload**: Load new keys into memory for immediate use

    {

      "alg": "SHA256",### 4. Error Handling

      "hash": "<base64url-encoded hash of disabled public key>"

    }- Download failures block the entire update process

  ],- Specific extended result codes are reported for different failure scenarios

  "rootKeys": [- Transient failures support retry mechanisms

    {- All errors are propagated to the cloud service for visibility

      "kid": "ADU.200703.R",

      "kty": "RSA",## Root Key Package Validation

      "n": "<base64url-encoded RSA modulus>",

      "e": "AQAB"### Signature Verification

    }

  ]Root key packages are cryptographically signed and validated using:

}

```- **Algorithm**: RS256 (RSA with SHA-256)

- **Keys**: Hardcoded RSA root keys embedded in agent binary

### Field Descriptions- **Process**: Multi-signature validation against all available hardcoded keys

- **Implementation**: `RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys()`

| Field | Type | Description |

|-------|------|-------------|### Package Structure Validation

| `isTest` | boolean | Whether this is a test package (allows test keys) |

| `version` | number | Monotonically increasing version number |The agent validates:

| `published` | string | ISO 8601 timestamp of package publication |- JSON structure and required fields

| `disabledRootKeys` | array | List of key IDs (kids) that are disabled |- Key format and parameters (RSA modulus, exponent)

| `disabledSigningKeys` | array | List of public key hashes that are disabled |- Signature integrity and authenticity

| `rootKeys` | array | Current valid root keys |- Test vs. production package compatibility



### Root Key Format### Environment Compatibility



Each root key contains:The agent enforces package-environment compatibility:



- **kid**: Key identifier (e.g., "ADU.200703.R")- **Production agents**: Only accept production root key packages

- **kty**: Key type (currently only "RSA" supported)- **Test agents**: Only accept test root key packages

- **n**: RSA modulus (base64url-encoded)- **E2E testing**: Special handling for service end-to-end testing scenarios

- **e**: RSA exponent (typically "AQAB" for 65537)- **Validation**: Based on `isTest` flag in package protected properties



## Security Model## Root Key Usage



### Signature Validation Process### Update Manifest Verification



1. **Download**: Root key package downloaded from ADU serviceRoot keys are primarily used to validate update manifest signatures:

2. **Parse**: JSON structure validated and parsed

3. **Signature Verification**: All signatures validated against hardcoded root keys1. **Process**: `workflow_validate_update_manifest_signature()`

4. **Version Check**: Ensure package version is newer than stored version2. **Algorithm**: RS256 signature verification

5. **Storage**: Valid package stored for use in manifest validation3. **Target**: Update manifest content and metadata

4. **Result**: Authorization to proceed with update installation

### Key Validation Chain

### Key Management

```mermaid

graph TB#### Local Storage

    A[Update Manifest] --> B[Extract Signature]- **Location**: `ADUC_ROOTKEY_STORE_PACKAGE_PATH`

    B --> C[Find Signing Key]- **Format**: JSON file with cryptographic keys and metadata

    C --> D[Check Against Disabled Keys]- **Updates**: Only when package content changes (hash comparison)

    D --> E[Validate with Root Key Package]- **Operations**: Atomic file writes ensure consistency

    E --> F[Verify Manifest Signature]

    F --> G[Accept/Reject Update]#### Memory Management

    - Keys loaded into memory during validation operations

    H[Root Key Package] --> I[Validate Signatures]- Automatic cleanup and resource management

    I --> J[Check Against Hardcoded Keys]- Support for multiple concurrent key operations

    J --> K[Verify Package Integrity]

    K --> L[Store if Valid]#### Version Control

    L --> E- Package versioning and change detection

```- Rollback protection through signature validation

- Audit trail through logging and error reporting

### Hardcoded Root Keys

## Security Features

The agent binary contains embedded root keys that serve as the ultimate trust anchor:

### Defense in Depth

- **Production Keys**: Used for validating production root key packages

- **Test Keys**: Used for validating test root key packages (when `isTest: true`)1. **Hardcoded Keys**: Ultimate trust anchor in agent binary

- **Multiple Keys**: Multiple root keys provide redundancy and rotation capability2. **Package Validation**: Cryptographic verification of root key packages

3. **Signature Verification**: Validation of all update content signatures

## Configuration4. **Environment Isolation**: Separation of test and production environments



### Build-Time Configuration### Key Rotation Support



Root key package behavior is configured at build time:- **Dynamic Updates**: Root keys can be updated without agent rebuilds

- **Backward Compatibility**: Supports gradual key rotation strategies

```cmake- **Validation Chain**: New keys validated against existing trust anchors

# Root key package download method- **Atomic Operations**: Ensures consistent key state during updates

set(ADUC_ROOTKEY_PKG_DOWNLOAD_WITH_CURL ON)  # Use curl instead of DO

### Attack Mitigation

# Root key package storage path

set(ADUC_ROOTKEY_STORE_PACKAGE_PATH "/var/lib/adu/rootkey.json")The root key system protects against:



# Override URL (for testing)- **Man-in-the-Middle**: Cryptographic signature validation

set(ADUC_ROOTKEY_PKG_URL_OVERRIDE "https://custom.url/rootkey.json")- **Content Tampering**: Hash verification and signature checks

```- **Unauthorized Updates**: Trust chain validation

- **Key Compromise**: Multi-key validation and rotation capabilities

### Runtime Configuration

## Configuration

The agent automatically downloads and validates root key packages:

### Build-Time Options

- **Automatic Download**: Triggered during update manifest processing

- **Caching**: Valid packages cached locally at `ADUC_ROOTKEY_STORE_PACKAGE_PATH`#### Root Key Package Download Method

- **Version Checking**: Only newer versions accepted```bash

- **Fallback**: Agent uses hardcoded keys if package unavailable# Use cURL instead of Delivery Optimization

./scripts/build.sh -c --rootkeypkg-curl

## Download Methods```



### Delivery Optimization (Default)#### Storage Paths

- `ADUC_ROOTKEY_STORE_PATH`: Directory for root key storage

By default, root key packages are downloaded using the Delivery Optimization (DO) client:- `ADUC_ROOTKEY_STORE_PACKAGE_PATH`: Full path to package file

- `ADUC_ROOTKEY_PKG_URL_OVERRIDE`: Override package URL (testing)

```cpp

ADUC_Result DownloadRootKeyPkg_DO(const char* url, const char* targetFilePath);### Runtime Configuration

```

- **Package URL**: Specified in deployment metadata from cloud service

### cURL Alternative- **Workflow ID**: Used for download sandboxing and isolation

- **Work Folder**: Temporary storage location during download process

For environments where DO is not available, curl can be used:

## Error Codes

```bash

# Build with curl download support### Extended Result Codes (ERC)

./scripts/build.sh -c --rootkeypkg-curl

```Common root key package error codes:



```cpp- `ADUC_ERC_INVALIDARG`: Invalid parameters passed to root key workflow

ADUC_Result DownloadRootKeyPkg_Curl(const char* url, const char* targetFilePath);- `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE`: Failed to parse downloaded package

```- `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE`: Failed to serialize package data

- `ADUC_ERC_ROOTKEY_PKG_UNCHANGED`: Package unchanged, no update needed

## Development and Testing- `ADUC_ERC_ROOTKEY_PACKAGE_CHANGED`: Package successfully updated

- `ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT`: Production package on test agent

### Test Root Keys- `ADUC_ERC_ROOTKEY_TEST_PKG_ON_PROD_AGENT`: Test package on production agent



Test packages (`"isTest": true`) allow additional test keys for development:### Download Error Codes



```json- `ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION`: Download exception

{- Delivery Optimization specific error codes (0x80190xxx range)

  "protected": {- cURL process exit codes and error conditions

    "isTest": true,

    "version": 1,## Best Practices

    "rootKeys": [

      {### Deployment

        "kid": "TEST.20231015.R",

        "kty": "RSA",1. **Always validate**: Ensure root key package URLs are correct in deployments

        "n": "test-modulus...",2. **Monitor failures**: Track root key package download and validation failures

        "e": "AQAB"3. **Environment matching**: Use appropriate packages for test vs. production

      }4. **Network requirements**: Ensure devices can reach root key package URLs

    ]

  }### Security

}

```1. **Key rotation**: Regularly update root key packages for security

2. **Monitoring**: Watch for signature validation failures

### Key Generation3. **Incident response**: Have procedures for compromised key scenarios

4. **Audit logging**: Maintain logs of all root key operations

For testing, generate RSA key pairs:

### Troubleshooting

```bash

# Generate private key1. **Download issues**: Check network connectivity and URL accessibility

openssl genrsa -out testkey.pem 20482. **Validation failures**: Verify package signatures and environment compatibility

3. **Storage problems**: Ensure adequate disk space and write permissions

# Extract public key components4. **Performance**: Monitor download times and optimize network configuration

openssl rsa -in testkey.pem -noout -modulus | sed 's/Modulus=//' | \

  xxd -r -p | base64url## Related Documentation

openssl rsa -in testkey.pem -noout -text | grep Exponent

```- [Device Update Agent Extended Result Codes](device-update-agent-extended-result-codes.md)

- [How to Troubleshoot Guide](../how-to-troubleshoot-guide.md)

### Package Creation- [Agent Reference Overview](../agent-reference/)

Create test root key packages:

```bash
# Create protected properties JSON
cat > protected.json << 'EOF'
{
  "isTest": true,
  "version": 1,
  "published": "2023-10-15T10:30:00Z",
  "rootKeys": [...]
}
EOF

# Base64URL encode
protected_b64=$(cat protected.json | base64url)

# Sign with test key
echo -n "$protected_b64" | openssl dgst -sha256 -sign testkey.pem | base64url
```

## Validation and Debugging

### Package Validation

The agent provides utilities for validating root key packages:

```cpp
// Load and validate package
ADUC_RootKeyPackage* package = NULL;
ADUC_Result result = RootKeyUtility_LoadPackageFromDisk(
    &package,
    "/var/lib/adu/rootkey.json",
    true /* validateSignatures */
);

if (IsAducResultCodeSuccess(result.ResultCode)) {
    // Package is valid
}
```

### Debug Information

Enable detailed logging for root key operations:

```bash
# Run agent with debug logging
sudo ./AducIotAgent -l 0  # Debug level
```

Look for log messages containing:
- `RootKeyPackage`
- `Signature validation`
- `Key validation`
- `Root key store`

### Common Issues

#### Invalid Signatures
```
ERROR: Signature validation failed for root key package
```
**Solution**: Verify package was signed with correct hardcoded keys

#### Version Conflicts
```
ERROR: Root key package version X is not newer than stored version Y
```
**Solution**: Ensure new package has higher version number

#### Disabled Keys
```
ERROR: Signing key is disabled in root key package
```
**Solution**: Update to use non-disabled keys or obtain newer root key package

#### Missing Package
```
ERROR: Root key package not found
```
**Solution**: Ensure agent can download package or manually place valid package

## Security Considerations

### Key Rotation

- **Planned Rotation**: Update root key package with new keys before old keys expire
- **Emergency Rotation**: Use `disabledRootKeys` to immediately revoke compromised keys
- **Graceful Transition**: Include both old and new keys during transition period

### Package Integrity

- **Multiple Signatures**: Each package signed by multiple root keys for redundancy
- **Version Monotonicity**: Prevents rollback attacks with old packages
- **Timestamp Validation**: Published timestamps help detect replay attacks

### Hardcoded Key Protection

- **Secure Storage**: Root keys embedded in binary, harder to extract
- **Multiple Keys**: No single point of failure
- **Test Separation**: Test keys cannot validate production packages

## API Reference

### Core Functions

```cpp
// Package loading and validation
ADUC_Result RootKeyUtility_LoadPackageFromDisk(
    ADUC_RootKeyPackage** rootKeyPackage,
    const char* fileLocation,
    bool validateSignatures
);

// Package download
ADUC_Result ADUC_RootKeyPackageUtils_DownloadPackage(
    const char* rootKeyPkgUrl,
    const char* workflowId,
    ADUC_RootKeyPkgDownloaderInfo* downloaderInfo,
    STRING_HANDLE* outRootKeyPackageDownloadedFile
);

// Package parsing
ADUC_Result ADUC_RootKeyPackageUtils_Parse(
    const char* jsonString,
    ADUC_RootKeyPackage* outRootKeyPackage
);

// Key validation
ADUC_Result RootKeyUtility_GetKeyForKid(
    CryptoKeyHandle* key,
    const char* kid
);

// Disabled key checking
bool RootKeyUtility_RootKeyIsDisabled(
    const ADUC_RootKeyPackage* rootKeyPackage,
    const char* keyId
);
```

### Error Codes

| Extended Result Code | Description |
|---------------------|-------------|
| `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE` | Failed to parse JSON |
| `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE` | Failed to serialize JSON |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED` | Signature validation failed |
| `ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_FOR_KEY_NOT_FOUND` | Signature for key not found |

## File Locations

| File | Purpose |
|------|---------|
| `/var/lib/adu/rootkey.json` | Cached root key package |
| `/var/log/adu/` | Agent logs with root key operations |
| Binary embedded | Hardcoded root keys |

## Related Documentation

- [Device Update Agent Extended Result Codes](device-update-agent-extended-result-codes.md)
- [Update Manifest v4 Schema](update-manifest-v4-schema.md)
- [How to Troubleshoot Guide](how-to-troubleshoot-guide.md)
- [How to Implement Custom Update Handler](how-to-implement-custom-update-handler.md)

## Version History

| Version | Changes |
|---------|---------|
| 1.0 | Initial root key package support |
| 1.1 | Added disabled signing keys support |
| 1.2 | Added curl download option, improved error handling |
