# utils

**Type:** Collection of ~30 Static C/C++ Utility Libraries

## Description

A large collection of **reusable utility libraries**, each in its own subdirectory. These provide foundational capabilities used across the entire Device Update agent codebase.

## Utility Libraries

| Directory | Description |
|---|---|
| **`c_utils/`** | Core C string/bit operations, connection string parsing, memory helpers |
| **`crypto_utils/`** | Cryptographic operations, Base64 encoding/decoding |
| **`config_utils/`** | Configuration file parsing and management |
| **`hash_utils/`** | File and data hash computation and verification |
| **`jws_utils/`** | JSON Web Signature (JWS) parsing and validation |
| **`root_key_utils/`** | Root key management and validation |
| **`rootkeypackage_utils/`** | Root key package parsing and handling |
| **`workflow_utils/`** | Update workflow data structures and helper functions |
| **`workflow_data_utils/`** | Workflow data serialization and deserialization |
| **`entity_utils/`** | Update entity (file, update ID) management |
| **`contract_utils/`** | Extension contract version checking and validation |
| **`eis_utils/`** | Edge Identity Service (EIS) integration utilities |
| **`d2c_messaging/`** | Device-to-cloud messaging helpers |
| **`auto_utils/`** | RAII/auto-cleanup wrappers for C resources |
| **`apiproto_utils/`** | API protocol utilities |

## Purpose

By centralizing common functionality into small, focused libraries, the `utils` directory promotes code reuse and consistency across the agent's modules. Each utility library is independently buildable and testable.

## Dependencies

Varies by library; common dependencies include `adu_types`, OpenSSL, Parson (JSON), and standard C libraries.
