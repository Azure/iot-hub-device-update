# Root Key Package Security in Device Update Agent

This document explains how the Azure Device Update (ADU) agent downloads and uses root key packages to establish a secure chain of trust for update operations.

## Overview

The root key package system is a critical security component that ensures only authorized and authentic updates can be installed on devices. It establishes a cryptographic chain of trust from hardcoded keys in the agent binary to update content validation.

## Security Architecture

### Trust Chain

The ADU agent implements a multi-layered security architecture:

```
Hardcoded Root Keys (in agent binary) 
    ↓ (validates)
Downloaded Root Key Package 
    ↓ (validates) 
Update Manifest Signatures 
    ↓ (authorizes)
Update Content
```

This chain ensures that:
1. Root key packages are validated against immutable keys in the agent
2. Update manifests are validated against trusted root keys
3. Update content is authorized through cryptographic signatures

## Root Key Package Download Process

### 1. Initiation

The root key download process is triggered during update deployment processing in the main agent workflow:

- **When**: At the start of `ProcessDeployment` action
- **Where**: In `adu_core_interface.c`
- **Requirement**: Must succeed before update manifest validation can proceed

### 2. Download Configuration

The agent supports two download methods:

#### Delivery Optimization (Default)
- Uses Microsoft's Delivery Optimization service for efficient downloads
- Provides bandwidth optimization and peer-to-peer capabilities
- Default timeout: 1 hour
- Implementation: `DownloadRootKeyPkg_DO()`

#### cURL Alternative
- Uses system `curl` command as fallback method
- Requires `/usr/bin/curl` to be available on the device
- Enabled at build time with `--rootkeypkg-curl` flag
- Command format: `/usr/bin/curl -L -C - -o <output> -m 3600 <url>`
- Implementation: `DownloadRootKeyPkg_Curl()`

### 3. Download Workflow

The `RootKeyWorkflow_UpdateRootKeys()` function orchestrates the process:

1. **Setup**: Configure downloader with workflow ID and work folder
2. **Download**: Fetch root key package from URL in deployment metadata
3. **Parse**: Load and validate JSON structure
4. **Validate**: Verify package signatures against hardcoded keys
5. **Store**: Atomically update local root key store if package has changed
6. **Reload**: Load new keys into memory for immediate use

### 4. Error Handling

- Download failures block the entire update process
- Specific extended result codes are reported for different failure scenarios
- Transient failures support retry mechanisms
- All errors are propagated to the cloud service for visibility

## Root Key Package Validation

### Signature Verification

Root key packages are cryptographically signed and validated using:

- **Algorithm**: RS256 (RSA with SHA-256)
- **Keys**: Hardcoded RSA root keys embedded in agent binary
- **Process**: Multi-signature validation against all available hardcoded keys
- **Implementation**: `RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys()`

### Package Structure Validation

The agent validates:
- JSON structure and required fields
- Key format and parameters (RSA modulus, exponent)
- Signature integrity and authenticity
- Test vs. production package compatibility

### Environment Compatibility

The agent enforces package-environment compatibility:

- **Production agents**: Only accept production root key packages
- **Test agents**: Only accept test root key packages
- **E2E testing**: Special handling for service end-to-end testing scenarios
- **Validation**: Based on `isTest` flag in package protected properties

## Root Key Usage

### Update Manifest Verification

Root keys are primarily used to validate update manifest signatures:

1. **Process**: `workflow_validate_update_manifest_signature()`
2. **Algorithm**: RS256 signature verification
3. **Target**: Update manifest content and metadata
4. **Result**: Authorization to proceed with update installation

### Key Management

#### Local Storage
- **Location**: `ADUC_ROOTKEY_STORE_PACKAGE_PATH`
- **Format**: JSON file with cryptographic keys and metadata
- **Updates**: Only when package content changes (hash comparison)
- **Operations**: Atomic file writes ensure consistency

#### Memory Management
- Keys loaded into memory during validation operations
- Automatic cleanup and resource management
- Support for multiple concurrent key operations

#### Version Control
- Package versioning and change detection
- Rollback protection through signature validation
- Audit trail through logging and error reporting

## Security Features

### Defense in Depth

1. **Hardcoded Keys**: Ultimate trust anchor in agent binary
2. **Package Validation**: Cryptographic verification of root key packages
3. **Signature Verification**: Validation of all update content signatures
4. **Environment Isolation**: Separation of test and production environments

### Key Rotation Support

- **Dynamic Updates**: Root keys can be updated without agent rebuilds
- **Backward Compatibility**: Supports gradual key rotation strategies
- **Validation Chain**: New keys validated against existing trust anchors
- **Atomic Operations**: Ensures consistent key state during updates

### Attack Mitigation

The root key system protects against:

- **Man-in-the-Middle**: Cryptographic signature validation
- **Content Tampering**: Hash verification and signature checks
- **Unauthorized Updates**: Trust chain validation
- **Key Compromise**: Multi-key validation and rotation capabilities

## Configuration

### Build-Time Options

#### Root Key Package Download Method
```bash
# Use cURL instead of Delivery Optimization
./scripts/build.sh -c --rootkeypkg-curl
```

#### Storage Paths
- `ADUC_ROOTKEY_STORE_PATH`: Directory for root key storage
- `ADUC_ROOTKEY_STORE_PACKAGE_PATH`: Full path to package file
- `ADUC_ROOTKEY_PKG_URL_OVERRIDE`: Override package URL (testing)

### Runtime Configuration

- **Package URL**: Specified in deployment metadata from cloud service
- **Workflow ID**: Used for download sandboxing and isolation
- **Work Folder**: Temporary storage location during download process

## Error Codes

### Extended Result Codes (ERC)

Common root key package error codes:

- `ADUC_ERC_INVALIDARG`: Invalid parameters passed to root key workflow
- `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE`: Failed to parse downloaded package
- `ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE`: Failed to serialize package data
- `ADUC_ERC_ROOTKEY_PKG_UNCHANGED`: Package unchanged, no update needed
- `ADUC_ERC_ROOTKEY_PACKAGE_CHANGED`: Package successfully updated
- `ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT`: Production package on test agent
- `ADUC_ERC_ROOTKEY_TEST_PKG_ON_PROD_AGENT`: Test package on production agent

### Download Error Codes

- `ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION`: Download exception
- Delivery Optimization specific error codes (0x80190xxx range)
- cURL process exit codes and error conditions

## Best Practices

### Deployment

1. **Always validate**: Ensure root key package URLs are correct in deployments
2. **Monitor failures**: Track root key package download and validation failures
3. **Environment matching**: Use appropriate packages for test vs. production
4. **Network requirements**: Ensure devices can reach root key package URLs

### Security

1. **Key rotation**: Regularly update root key packages for security
2. **Monitoring**: Watch for signature validation failures
3. **Incident response**: Have procedures for compromised key scenarios
4. **Audit logging**: Maintain logs of all root key operations

### Troubleshooting

1. **Download issues**: Check network connectivity and URL accessibility
2. **Validation failures**: Verify package signatures and environment compatibility  
3. **Storage problems**: Ensure adequate disk space and write permissions
4. **Performance**: Monitor download times and optimize network configuration

## Related Documentation

- [Device Update Agent Extended Result Codes](device-update-agent-extended-result-codes.md)
- [How to Troubleshoot Guide](../how-to-troubleshoot-guide.md)
- [Agent Reference Overview](../agent-reference/)