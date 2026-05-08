#!/usr/bin/env python3
"""
ADU Gen2 Result Code (RC) / Extended Result Code (ERC) Decoder

Decodes numeric result codes into human-readable descriptions.
Layout: [facility:8][category:8][specific:16]

Usage:
    python rc_decoder.py 0x01010001
    python rc_decoder.py 16842753
    python rc_decoder.py --list --facility AGENT
    python rc_decoder.py --search "timeout"
    python rc_decoder.py --parse-log path/to/adu-gen2.log

Copyright (c) Microsoft Corporation. Licensed under the MIT License.
"""

import argparse
import re
import sys

# ═══════════════════════════════════════════════════════════════════════════
# Facilities and Categories
# ═══════════════════════════════════════════════════════════════════════════

FACILITIES = {
    0x01: "AGENT",
    0x02: "DOWNLOAD",
    0x03: "WORKFLOW",
    0x04: "EXTENSION",
    0x05: "COMM",
    0x06: "SECURITY",
}

CATEGORIES = {
    0x01: "CONFIG",
    0x02: "AUTH",
    0x03: "IO",
    0x04: "PROTOCOL",
    0x05: "RESOURCE",
    0x06: "STATE",
    0x07: "DAG",
    0x10: "SCRIPT",
    0x11: "APT",
    0x12: "SWUPDATE",
}

# ═══════════════════════════════════════════════════════════════════════════
# Result Code Registry
# Key: (facility, category, specific) → (macro_name, description)
# ═══════════════════════════════════════════════════════════════════════════

RESULT_CODES = {
    # AGENT / CONFIG
    (0x01, 0x01, 0x0001): ("ADUC_RC_AGENT_CONFIG_FILE_NOT_FOUND", "Configuration file not found"),
    (0x01, 0x01, 0x0002): ("ADUC_RC_AGENT_CONFIG_PARSE_ERROR", "Configuration file parse error (invalid JSON/format)"),
    (0x01, 0x01, 0x0003): ("ADUC_RC_AGENT_CONFIG_MISSING_FIELD", "Required configuration field is missing"),
    (0x01, 0x01, 0x0004): ("ADUC_RC_AGENT_CONFIG_INVALID_VALUE", "Configuration field has an invalid value"),
    (0x01, 0x01, 0x0005): ("ADUC_RC_AGENT_CONFIG_VERSION_MISMATCH", "Configuration version is incompatible"),
    (0x01, 0x01, 0x0006): ("ADUC_RC_AGENT_CONFIG_DIR_NOT_FOUND", "Configuration directory not found"),
    (0x01, 0x01, 0x0007): ("ADUC_RC_AGENT_CONFIG_PERMISSION_DENIED", "Permission denied reading configuration"),
    # AGENT / AUTH
    (0x01, 0x02, 0x0001): ("ADUC_RC_AGENT_AUTH_IDENTITY_FAILURE", "Agent identity authentication failure"),
    (0x01, 0x02, 0x0002): ("ADUC_RC_AGENT_AUTH_IDENTITY_NOT_PROVISIONED", "Device identity not provisioned"),
    # AGENT / IO
    (0x01, 0x03, 0x0001): ("ADUC_RC_AGENT_IO_PIPE_CREATE_FAILURE", "Failed to create IPC pipe"),
    (0x01, 0x03, 0x0002): ("ADUC_RC_AGENT_IO_SOCKET_ERROR", "Socket communication error"),
    (0x01, 0x03, 0x0003): ("ADUC_RC_AGENT_IO_LOG_WRITE_FAILURE", "Failed to write to log file"),
    (0x01, 0x03, 0x0004): ("ADUC_RC_AGENT_IO_LOG_OPEN_FAILURE", "Failed to open log file"),
    (0x01, 0x03, 0x0005): ("ADUC_RC_AGENT_IO_IPC_SEND_FAILURE", "Failed to send IPC message"),
    (0x01, 0x03, 0x0006): ("ADUC_RC_AGENT_IO_IPC_RECV_FAILURE", "Failed to receive IPC message"),
    # AGENT / RESOURCE
    (0x01, 0x05, 0x0001): ("ADUC_RC_AGENT_RESOURCE_OUT_OF_MEMORY", "Out of memory"),
    (0x01, 0x05, 0x0002): ("ADUC_RC_AGENT_RESOURCE_THREAD_CREATE_FAIL", "Failed to create thread"),
    (0x01, 0x05, 0x0003): ("ADUC_RC_AGENT_RESOURCE_MUTEX_INIT_FAIL", "Failed to initialize mutex"),
    (0x01, 0x05, 0x0004): ("ADUC_RC_AGENT_RESOURCE_FD_EXHAUSTED", "File descriptors exhausted"),
    # AGENT / STATE
    (0x01, 0x06, 0x0001): ("ADUC_RC_AGENT_STATE_ALREADY_RUNNING", "Agent is already running"),
    (0x01, 0x06, 0x0002): ("ADUC_RC_AGENT_STATE_NOT_INITIALIZED", "Agent is not initialized"),
    (0x01, 0x06, 0x0003): ("ADUC_RC_AGENT_STATE_SHUTDOWN_IN_PROGRESS", "Agent shutdown is in progress"),
    (0x01, 0x06, 0x0004): ("ADUC_RC_AGENT_STATE_RESTART_REQUIRED", "Agent restart is required"),

    # DOWNLOAD / CONFIG
    (0x02, 0x01, 0x0001): ("ADUC_RC_DOWNLOAD_CONFIG_INVALID_URL", "Download URL is invalid"),
    (0x02, 0x01, 0x0002): ("ADUC_RC_DOWNLOAD_CONFIG_INVALID_HASH_ALG", "Invalid hash algorithm specified"),
    (0x02, 0x01, 0x0003): ("ADUC_RC_DOWNLOAD_CONFIG_INVALID_DEST_PATH", "Invalid destination path for download"),
    (0x02, 0x01, 0x0004): ("ADUC_RC_DOWNLOAD_CONFIG_MISSING_SIZE", "Expected file size not specified"),
    # DOWNLOAD / AUTH
    (0x02, 0x02, 0x0001): ("ADUC_RC_DOWNLOAD_AUTH_TOKEN_EXPIRED", "Download auth token has expired"),
    (0x02, 0x02, 0x0002): ("ADUC_RC_DOWNLOAD_AUTH_UNAUTHORIZED", "Unauthorized to download (HTTP 401)"),
    (0x02, 0x02, 0x0003): ("ADUC_RC_DOWNLOAD_AUTH_FORBIDDEN", "Download forbidden (HTTP 403)"),
    # DOWNLOAD / IO
    (0x02, 0x03, 0x0001): ("ADUC_RC_DOWNLOAD_IO_NETWORK_ERROR", "Network error during download"),
    (0x02, 0x03, 0x0002): ("ADUC_RC_DOWNLOAD_IO_FILE_OPEN_FAILURE", "Failed to open output file for download"),
    (0x02, 0x03, 0x0003): ("ADUC_RC_DOWNLOAD_IO_HASH_MISMATCH", "Downloaded file hash does not match expected"),
    (0x02, 0x03, 0x0004): ("ADUC_RC_DOWNLOAD_IO_SIZE_MISMATCH", "Downloaded file size does not match expected"),
    (0x02, 0x03, 0x0005): ("ADUC_RC_DOWNLOAD_IO_RENAME_FAILURE", "Failed to rename temp file to final path"),
    (0x02, 0x03, 0x0006): ("ADUC_RC_DOWNLOAD_IO_HASH_COMPUTE_FAILURE", "Failed to compute file hash"),
    (0x02, 0x03, 0x0007): ("ADUC_RC_DOWNLOAD_IO_TIMEOUT", "Download I/O operation timed out"),
    (0x02, 0x03, 0x0008): ("ADUC_RC_DOWNLOAD_IO_PARTIAL_WRITE", "Partial write to disk during download"),
    (0x02, 0x03, 0x0009): ("ADUC_RC_DOWNLOAD_IO_DISK_FULL", "Disk full during download"),
    (0x02, 0x03, 0x000A): ("ADUC_RC_DOWNLOAD_IO_CONNECTION_RESET", "Connection reset during download"),
    # DOWNLOAD / PROTOCOL
    (0x02, 0x04, 0x0001): ("ADUC_RC_DOWNLOAD_PROTOCOL_CURL_INIT_FAIL", "Failed to initialize curl handle"),
    (0x02, 0x04, 0x0002): ("ADUC_RC_DOWNLOAD_PROTOCOL_CURL_PERFORM_FAIL", "curl_easy_perform returned error"),
    (0x02, 0x04, 0x0003): ("ADUC_RC_DOWNLOAD_PROTOCOL_HTTP_4XX", "HTTP 4xx client error response"),
    (0x02, 0x04, 0x0004): ("ADUC_RC_DOWNLOAD_PROTOCOL_HTTP_5XX", "HTTP 5xx server error response"),
    (0x02, 0x04, 0x0005): ("ADUC_RC_DOWNLOAD_PROTOCOL_TLS_HANDSHAKE", "TLS handshake failure"),
    (0x02, 0x04, 0x0006): ("ADUC_RC_DOWNLOAD_PROTOCOL_DNS_FAILURE", "DNS resolution failure"),
    (0x02, 0x04, 0x0007): ("ADUC_RC_DOWNLOAD_PROTOCOL_REDIRECT_LOOP", "HTTP redirect loop detected"),
    # DOWNLOAD / RESOURCE
    (0x02, 0x05, 0x0001): ("ADUC_RC_DOWNLOAD_RESOURCE_ALLOC_FAILURE", "Memory allocation failure in download"),
    (0x02, 0x05, 0x0002): ("ADUC_RC_DOWNLOAD_RESOURCE_TOO_MANY_RETRIES", "Download exceeded retry limit"),
    # DOWNLOAD / STATE
    (0x02, 0x06, 0x0001): ("ADUC_RC_DOWNLOAD_STATE_ALREADY_DOWNLOADING", "Download already in progress"),
    (0x02, 0x06, 0x0002): ("ADUC_RC_DOWNLOAD_STATE_CANCELLED", "Download was cancelled"),
    (0x02, 0x06, 0x0003): ("ADUC_RC_DOWNLOAD_STATE_NOT_STARTED", "Download has not been started"),

    # WORKFLOW / CONFIG
    (0x03, 0x01, 0x0001): ("ADUC_RC_WORKFLOW_CONFIG_INVALID_MANIFEST", "Update manifest is invalid or malformed"),
    (0x03, 0x01, 0x0002): ("ADUC_RC_WORKFLOW_CONFIG_MISSING_HANDLER", "No handler registered for update type"),
    (0x03, 0x01, 0x0003): ("ADUC_RC_WORKFLOW_CONFIG_MISSING_STEP_ID", "Step is missing required ID field"),
    (0x03, 0x01, 0x0004): ("ADUC_RC_WORKFLOW_CONFIG_INVALID_UPDATE_TYPE", "Invalid or unrecognized update type"),
    (0x03, 0x01, 0x0005): ("ADUC_RC_WORKFLOW_CONFIG_MANIFEST_TOO_LARGE", "Update manifest exceeds size limit"),
    (0x03, 0x01, 0x0006): ("ADUC_RC_WORKFLOW_CONFIG_MISSING_COMPAT_INFO", "Missing compatibility information in manifest"),
    # WORKFLOW / IO
    (0x03, 0x03, 0x0001): ("ADUC_RC_WORKFLOW_IO_PERSIST_WRITE_FAILURE", "Failed to persist workflow state to disk"),
    (0x03, 0x03, 0x0002): ("ADUC_RC_WORKFLOW_IO_PERSIST_READ_FAILURE", "Failed to read persisted workflow state"),
    (0x03, 0x03, 0x0003): ("ADUC_RC_WORKFLOW_IO_RESULT_FILE_ERROR", "Error reading/writing step result file"),
    (0x03, 0x03, 0x0004): ("ADUC_RC_WORKFLOW_IO_TEMP_DIR_CREATE_FAIL", "Failed to create temporary working directory"),
    # WORKFLOW / RESOURCE
    (0x03, 0x05, 0x0001): ("ADUC_RC_WORKFLOW_RESOURCE_TOO_MANY_STEPS", "Too many steps in workflow"),
    (0x03, 0x05, 0x0002): ("ADUC_RC_WORKFLOW_RESOURCE_ALLOC_FAILURE", "Memory allocation failure in workflow engine"),
    (0x03, 0x05, 0x0003): ("ADUC_RC_WORKFLOW_RESOURCE_STEP_ALLOC_FAIL", "Failed to allocate step result storage"),
    # WORKFLOW / STATE
    (0x03, 0x06, 0x0001): ("ADUC_RC_WORKFLOW_STATE_ALREADY_EXECUTING", "Workflow is already executing"),
    (0x03, 0x06, 0x0002): ("ADUC_RC_WORKFLOW_STATE_HANDLER_NOT_FOUND", "Step handler not found for update type"),
    (0x03, 0x06, 0x0003): ("ADUC_RC_WORKFLOW_STATE_CYCLE_DETECTED", "Cycle detected in workflow execution"),
    (0x03, 0x06, 0x0004): ("ADUC_RC_WORKFLOW_STATE_CANCELLED", "Workflow execution was cancelled"),
    (0x03, 0x06, 0x0005): ("ADUC_RC_WORKFLOW_STATE_DEADLOCK", "Deadlock detected in workflow"),
    (0x03, 0x06, 0x0006): ("ADUC_RC_WORKFLOW_STATE_STEP_FAILED", "A workflow step failed"),
    (0x03, 0x06, 0x0007): ("ADUC_RC_WORKFLOW_STATE_ROLLBACK_FAILED", "Workflow rollback failed"),
    # WORKFLOW / DAG
    (0x03, 0x07, 0x0001): ("ADUC_RC_WORKFLOW_DAG_CYCLE_DETECTED", "Cycle detected in DAG dependency graph"),
    (0x03, 0x07, 0x0002): ("ADUC_RC_WORKFLOW_DAG_UNKNOWN_DEPENDENCY", "DAG references unknown dependency node"),
    (0x03, 0x07, 0x0003): ("ADUC_RC_WORKFLOW_DAG_NODE_NOT_FOUND", "DAG node not found"),
    (0x03, 0x07, 0x0004): ("ADUC_RC_WORKFLOW_DAG_INVALID_TRANSITION", "Invalid DAG node state transition"),
    (0x03, 0x07, 0x0005): ("ADUC_RC_WORKFLOW_DAG_MAX_DEPTH_EXCEEDED", "DAG exceeded maximum recursion depth"),
    (0x03, 0x07, 0x0006): ("ADUC_RC_WORKFLOW_DAG_ALLOC_FAILURE", "Memory allocation failure in DAG engine"),

    # EXTENSION / CONFIG
    (0x04, 0x01, 0x0001): ("ADUC_RC_EXTENSION_CONFIG_INVALID_DESCRIPTOR", "Extension descriptor is invalid"),
    (0x04, 0x01, 0x0002): ("ADUC_RC_EXTENSION_CONFIG_VERSION_MISMATCH", "Extension SDK version mismatch"),
    (0x04, 0x01, 0x0003): ("ADUC_RC_EXTENSION_CONFIG_MISSING_VTABLE", "Extension is missing required vtable"),
    (0x04, 0x01, 0x0004): ("ADUC_RC_EXTENSION_CONFIG_INVALID_TYPE", "Invalid extension type specified"),
    # EXTENSION / AUTH
    (0x04, 0x02, 0x0001): ("ADUC_RC_EXTENSION_AUTH_SIG_VERIFY_FAILED", "Extension signature verification failed"),
    (0x04, 0x02, 0x0002): ("ADUC_RC_EXTENSION_AUTH_UNSIGNED_EXTENSION", "Extension is not signed (unsigned)"),
    (0x04, 0x02, 0x0003): ("ADUC_RC_EXTENSION_AUTH_HASH_MISMATCH", "Extension binary hash does not match manifest"),
    # EXTENSION / IO
    (0x04, 0x03, 0x0001): ("ADUC_RC_EXTENSION_IO_DLOPEN_FAILURE", "dlopen() failed to load shared library"),
    (0x04, 0x03, 0x0002): ("ADUC_RC_EXTENSION_IO_DLSYM_FAILURE", "dlsym() failed to find required symbol"),
    (0x04, 0x03, 0x0003): ("ADUC_RC_EXTENSION_IO_SCAN_DIR_FAILURE", "Failed to scan extension directory"),
    (0x04, 0x03, 0x0004): ("ADUC_RC_EXTENSION_IO_FILE_READ_FAILURE", "Failed to read extension file"),
    # EXTENSION / PROTOCOL
    (0x04, 0x04, 0x0001): ("ADUC_RC_EXTENSION_PROTOCOL_ABI_MISMATCH", "Extension ABI version mismatch"),
    (0x04, 0x04, 0x0002): ("ADUC_RC_EXTENSION_PROTOCOL_INIT_FAILED", "Extension initialization call failed"),
    (0x04, 0x04, 0x0003): ("ADUC_RC_EXTENSION_PROTOCOL_CALL_FAILED", "Extension vtable call returned error"),
    (0x04, 0x04, 0x0004): ("ADUC_RC_EXTENSION_PROTOCOL_VTABLE_INCOMPLETE", "Extension vtable has NULL function pointers"),
    # EXTENSION / RESOURCE
    (0x04, 0x05, 0x0001): ("ADUC_RC_EXTENSION_RESOURCE_ALLOC_FAILURE", "Memory allocation failure in extension"),
    (0x04, 0x05, 0x0002): ("ADUC_RC_EXTENSION_RESOURCE_REGISTRY_FULL", "Extension registry is full"),
    (0x04, 0x05, 0x0003): ("ADUC_RC_EXTENSION_RESOURCE_HANDLE_LIMIT", "Extension handle limit reached"),
    # EXTENSION / STATE
    (0x04, 0x06, 0x0001): ("ADUC_RC_EXTENSION_STATE_ALREADY_INITIALIZED", "Extension is already initialized"),
    (0x04, 0x06, 0x0002): ("ADUC_RC_EXTENSION_STATE_NOT_INITIALIZED", "Extension is not initialized"),
    (0x04, 0x06, 0x0003): ("ADUC_RC_EXTENSION_STATE_CAPABILITY_NOT_FOUND", "Requested capability not found in extension"),
    (0x04, 0x06, 0x0004): ("ADUC_RC_EXTENSION_STATE_ALREADY_LOADED", "Extension is already loaded"),
    # EXTENSION / SCRIPT
    (0x04, 0x10, 0x0001): ("ADUC_RC_EXTENSION_SCRIPT_NOT_FOUND", "Script file not found"),
    (0x04, 0x10, 0x0002): ("ADUC_RC_EXTENSION_SCRIPT_TIMEOUT", "Script execution timed out"),
    (0x04, 0x10, 0x0003): ("ADUC_RC_EXTENSION_SCRIPT_NONZERO_EXIT", "Script exited with non-zero status"),
    (0x04, 0x10, 0x0004): ("ADUC_RC_EXTENSION_SCRIPT_PERMISSION_DENIED", "Permission denied executing script"),
    (0x04, 0x10, 0x0005): ("ADUC_RC_EXTENSION_SCRIPT_RESULT_PARSE_ERROR", "Failed to parse script result file"),
    (0x04, 0x10, 0x0006): ("ADUC_RC_EXTENSION_SCRIPT_INVALID_EXIT_CODE", "Script exit code is invalid/unrecognized"),
    (0x04, 0x10, 0x0007): ("ADUC_RC_EXTENSION_SCRIPT_EXEC_FAILURE", "Failed to exec/spawn script process"),
    (0x04, 0x10, 0x0008): ("ADUC_RC_EXTENSION_SCRIPT_SANDBOX_VIOLATION", "Script violated sandbox policy"),
    # EXTENSION / APT
    (0x04, 0x11, 0x0001): ("ADUC_RC_EXTENSION_APT_PACKAGE_NOT_FOUND", "APT package not found in repositories"),
    (0x04, 0x11, 0x0002): ("ADUC_RC_EXTENSION_APT_GET_FAILURE", "apt-get command returned error"),
    (0x04, 0x11, 0x0003): ("ADUC_RC_EXTENSION_APT_DPKG_ERROR", "dpkg reported an error"),
    (0x04, 0x11, 0x0004): ("ADUC_RC_EXTENSION_APT_LOCK_CONTENTION", "APT/dpkg lock is held by another process"),
    (0x04, 0x11, 0x0005): ("ADUC_RC_EXTENSION_APT_SOURCE_LIST_ERROR", "APT sources list is invalid"),
    (0x04, 0x11, 0x0006): ("ADUC_RC_EXTENSION_APT_DEPENDENCY_BROKEN", "APT package dependencies are broken"),
    (0x04, 0x11, 0x0007): ("ADUC_RC_EXTENSION_APT_AUTH_FAILURE", "APT repository authentication failure"),
    # EXTENSION / SWUPDATE
    (0x04, 0x12, 0x0001): ("ADUC_RC_EXTENSION_SWUPDATE_FILE_NOT_FOUND", "SWU image file not found"),
    (0x04, 0x12, 0x0002): ("ADUC_RC_EXTENSION_SWUPDATE_BINARY_MISSING", "swupdate binary not found on system"),
    (0x04, 0x12, 0x0003): ("ADUC_RC_EXTENSION_SWUPDATE_VERIFY_FAILED", "SWU image signature verification failed"),
    (0x04, 0x12, 0x0004): ("ADUC_RC_EXTENSION_SWUPDATE_HW_REV_MISMATCH", "Hardware revision mismatch for SWU image"),
    (0x04, 0x12, 0x0005): ("ADUC_RC_EXTENSION_SWUPDATE_INSTALL_FAILED", "SWUpdate installation step failed"),
    (0x04, 0x12, 0x0006): ("ADUC_RC_EXTENSION_SWUPDATE_APPLY_FAILED", "SWUpdate apply step failed"),
    (0x04, 0x12, 0x0007): ("ADUC_RC_EXTENSION_SWUPDATE_ROLLBACK_FAILED", "SWUpdate rollback failed"),

    # COMM / CONFIG
    (0x05, 0x01, 0x0001): ("ADUC_RC_COMM_CONFIG_MISSING_ENDPOINT", "Communication endpoint not configured"),
    (0x05, 0x01, 0x0002): ("ADUC_RC_COMM_CONFIG_INVALID_DEVICE_ID", "Invalid device ID in configuration"),
    (0x05, 0x01, 0x0003): ("ADUC_RC_COMM_CONFIG_NO_PROVIDERS", "No communication providers configured"),
    (0x05, 0x01, 0x0004): ("ADUC_RC_COMM_CONFIG_INVALID_SCOPE_ID", "Invalid DPS scope ID"),
    # COMM / AUTH
    (0x05, 0x02, 0x0001): ("ADUC_RC_COMM_AUTH_FAILURE", "Communication authentication failure"),
    (0x05, 0x02, 0x0002): ("ADUC_RC_COMM_AUTH_TOKEN_REFRESH_FAILURE", "Failed to refresh auth token"),
    (0x05, 0x02, 0x0003): ("ADUC_RC_COMM_AUTH_CERTIFICATE_REJECTED", "Server rejected client certificate"),
    (0x05, 0x02, 0x0004): ("ADUC_RC_COMM_AUTH_SAS_TOKEN_EXPIRED", "SAS token has expired"),
    # COMM / IO
    (0x05, 0x03, 0x0001): ("ADUC_RC_COMM_IO_SEND_TIMEOUT", "Timed out sending message to service"),
    (0x05, 0x03, 0x0002): ("ADUC_RC_COMM_IO_RECEIVE_TIMEOUT", "Timed out waiting for service response"),
    (0x05, 0x03, 0x0003): ("ADUC_RC_COMM_IO_SEND_FAILURE", "Failed to send message"),
    (0x05, 0x03, 0x0004): ("ADUC_RC_COMM_IO_RECEIVE_FAILURE", "Failed to receive message"),
    # COMM / PROTOCOL
    (0x05, 0x04, 0x0001): ("ADUC_RC_COMM_PROTOCOL_CONNECT_FAILURE", "Failed to connect to service endpoint"),
    (0x05, 0x04, 0x0002): ("ADUC_RC_COMM_PROTOCOL_POLL_FAILURE", "Failed to poll for updates from service"),
    (0x05, 0x04, 0x0003): ("ADUC_RC_COMM_PROTOCOL_REPORT_FAILURE", "Failed to report status to service"),
    (0x05, 0x04, 0x0004): ("ADUC_RC_COMM_PROTOCOL_UNEXPECTED_RESPONSE", "Unexpected protocol response from service"),
    (0x05, 0x04, 0x0005): ("ADUC_RC_COMM_PROTOCOL_ALL_PROVIDERS_FAILED", "All communication providers failed"),
    (0x05, 0x04, 0x0006): ("ADUC_RC_COMM_PROTOCOL_MESSAGE_TOO_LARGE", "Message exceeds protocol size limit"),
    # COMM / RESOURCE
    (0x05, 0x05, 0x0001): ("ADUC_RC_COMM_RESOURCE_ALLOC_FAILURE", "Memory allocation failure in comm layer"),
    # COMM / STATE
    (0x05, 0x06, 0x0001): ("ADUC_RC_COMM_STATE_NOT_CONNECTED", "Not connected to service"),
    (0x05, 0x06, 0x0002): ("ADUC_RC_COMM_STATE_ALREADY_CONNECTED", "Already connected to service"),
    (0x05, 0x06, 0x0003): ("ADUC_RC_COMM_STATE_RECONNECTING", "Currently reconnecting to service"),
    (0x05, 0x06, 0x0004): ("ADUC_RC_COMM_STATE_SHUTDOWN", "Communication layer is shut down"),

    # SECURITY / CONFIG
    (0x06, 0x01, 0x0001): ("ADUC_RC_SECURITY_CONFIG_MISSING_CERT_PATH", "Certificate file path not configured"),
    (0x06, 0x01, 0x0002): ("ADUC_RC_SECURITY_CONFIG_INVALID_KEY_FORMAT", "Private key format is invalid"),
    (0x06, 0x01, 0x0003): ("ADUC_RC_SECURITY_CONFIG_MISSING_KEY_PATH", "Private key file path not configured"),
    (0x06, 0x01, 0x0004): ("ADUC_RC_SECURITY_CONFIG_INVALID_ALGORITHM", "Specified crypto algorithm is not supported"),
    # SECURITY / AUTH
    (0x06, 0x02, 0x0001): ("ADUC_RC_SECURITY_AUTH_CERT_EXPIRED", "Certificate has expired"),
    (0x06, 0x02, 0x0002): ("ADUC_RC_SECURITY_AUTH_INVALID_CHAIN", "Certificate chain validation failed"),
    (0x06, 0x02, 0x0003): ("ADUC_RC_SECURITY_AUTH_KEY_MISMATCH", "Public/private key mismatch"),
    (0x06, 0x02, 0x0004): ("ADUC_RC_SECURITY_AUTH_CRL_CHECK_FAILURE", "Certificate revocation list check failed"),
    (0x06, 0x02, 0x0005): ("ADUC_RC_SECURITY_AUTH_SIG_INVALID", "Signature data is malformed"),
    (0x06, 0x02, 0x0006): ("ADUC_RC_SECURITY_AUTH_SIG_VERIFY_FAILED", "Signature cryptographic verification failed"),
    (0x06, 0x02, 0x0007): ("ADUC_RC_SECURITY_AUTH_KEY_LOAD_FAILURE", "Failed to load signing/verification key"),
    (0x06, 0x02, 0x0008): ("ADUC_RC_SECURITY_AUTH_HASH_MISMATCH", "Content hash does not match signature"),
    (0x06, 0x02, 0x0010): ("ADUC_RC_SECURITY_AUTH_INVALID_ARG", "Invalid argument to security function"),
    (0x06, 0x02, 0x0011): ("ADUC_RC_SECURITY_AUTH_CHAIN_INVALID", "Certificate chain is structurally invalid"),
    # SECURITY / IO
    (0x06, 0x03, 0x0001): ("ADUC_RC_SECURITY_IO_KEYSTORE_ACCESS_FAILURE", "Failed to access key store"),
    (0x06, 0x03, 0x0002): ("ADUC_RC_SECURITY_IO_CERT_FILE_OPEN", "Failed to open certificate file"),
    (0x06, 0x03, 0x0003): ("ADUC_RC_SECURITY_IO_HASH_COMPUTE_FAILURE", "Failed to compute cryptographic hash"),
    (0x06, 0x03, 0x0004): ("ADUC_RC_SECURITY_IO_PARSE_FAILURE", "Failed to parse security data structure"),
    (0x06, 0x03, 0x0005): ("ADUC_RC_SECURITY_IO_KEY_READ_FAILURE", "Failed to read key file"),
    (0x06, 0x03, 0x0010): ("ADUC_RC_SECURITY_IO_CERT_FILE_PARSE", "Failed to parse certificate file (DER/PEM)"),
    (0x06, 0x03, 0x0011): ("ADUC_RC_SECURITY_IO_CERT_FILE_OPEN_2", "Failed to open certificate file (alternate path)"),
    # SECURITY / RESOURCE
    (0x06, 0x05, 0x0001): ("ADUC_RC_SECURITY_RESOURCE_STORE_FAILURE", "Certificate store operation failed"),
    (0x06, 0x05, 0x0010): ("ADUC_RC_SECURITY_RESOURCE_ALLOC_FAILURE", "Memory allocation failure in security module"),
}


def decode_code(value):
    """Decode a 32-bit result code into its components."""
    facility = (value >> 24) & 0xFF
    category = (value >> 16) & 0xFF
    specific = value & 0xFFFF
    return facility, category, specific


def format_decode(value):
    """Return a formatted decode string for a result code value."""
    if value == 0:
        return "ADUC_RESULT2_SUCCESS (Success — no error)"

    facility, category, specific = decode_code(value)
    key = (facility, category, specific)

    facility_name = FACILITIES.get(facility, f"UNKNOWN(0x{facility:02X})")
    category_name = CATEGORIES.get(category, f"UNKNOWN(0x{category:02X})")

    lines = []
    if key in RESULT_CODES:
        macro, desc = RESULT_CODES[key]
        lines.append(f"  Name:        {macro}")
        lines.append(f"  Description: {desc}")
    else:
        lines.append("  Name:        (unknown — not in registry)")
        lines.append("  Description: No description available")

    lines.append(f"  Facility:    {facility_name} (0x{facility:02X})")
    lines.append(f"  Category:    {category_name} (0x{category:02X})")
    lines.append(f"  Specific:    0x{specific:04X} ({specific})")
    lines.append(f"  Raw:         0x{value:08X} ({value})")

    return "\n".join(lines)


def parse_value(s):
    """Parse a hex or decimal string to int."""
    s = s.strip()
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    return int(s)


def cmd_decode(args):
    """Decode one or more result codes."""
    for code_str in args.codes:
        try:
            value = parse_value(code_str)
        except ValueError:
            print(f"Error: '{code_str}' is not a valid number", file=sys.stderr)
            continue
        print(f"\n=== Result Code: 0x{value:08X} ===")
        print(format_decode(value))


def cmd_list(args):
    """List all codes, optionally filtered by facility."""
    facility_filter = None
    if args.facility:
        name = args.facility.upper()
        for fid, fname in FACILITIES.items():
            if fname == name:
                facility_filter = fid
                break
        if facility_filter is None:
            print(f"Error: Unknown facility '{args.facility}'", file=sys.stderr)
            print(f"  Available: {', '.join(FACILITIES.values())}", file=sys.stderr)
            sys.exit(1)

    category_filter = None
    if args.category:
        name = args.category.upper()
        for cid, cname in CATEGORIES.items():
            if cname == name:
                category_filter = cid
                break

    count = 0
    for (fac, cat, spec), (macro, desc) in sorted(RESULT_CODES.items()):
        if facility_filter is not None and fac != facility_filter:
            continue
        if category_filter is not None and cat != category_filter:
            continue
        value = (fac << 24) | (cat << 16) | spec
        print(f"  0x{value:08X}  {macro}")
        print(f"             {desc}")
        count += 1

    print(f"\n  Total: {count} result code(s)")


def cmd_search(args):
    """Search codes by keyword."""
    query = args.query.lower()
    results = []
    for (fac, cat, spec), (macro, desc) in RESULT_CODES.items():
        if query in macro.lower() or query in desc.lower():
            results.append((fac, cat, spec, macro, desc))

    if not results:
        print(f"No result codes matching '{args.query}'")
        return

    print(f"Found {len(results)} result code(s) matching '{args.query}':\n")
    for fac, cat, spec, macro, desc in sorted(results):
        value = (fac << 24) | (cat << 16) | spec
        print(f"  0x{value:08X}  {macro}")
        print(f"             {desc}")


def cmd_parse_log(args):
    """Parse a log file for result codes and decode them."""
    # Match patterns like: code=0x01010001, result=0x02030003, rc=16842753
    pattern = re.compile(
        r"(?:code|result|rc|erc|Result2|ADUC_Result2)[=:\s]+(?:0[xX][0-9a-fA-F]{1,8}|\d+)"
    )
    hex_pattern = re.compile(r"(0[xX][0-9a-fA-F]{1,8}|\d{7,10})")

    seen = set()
    try:
        with open(args.log_file, "r", encoding="utf-8", errors="replace") as f:
            for line_num, line in enumerate(f, 1):
                matches = pattern.findall(line)
                for match in matches:
                    nums = hex_pattern.findall(match)
                    for num_str in nums:
                        try:
                            value = parse_value(num_str)
                        except ValueError:
                            continue
                        if value == 0 or value in seen:
                            continue
                        # Sanity: facility should be 1-6
                        fac = (value >> 24) & 0xFF
                        if fac < 1 or fac > 6:
                            continue
                        seen.add(value)
                        print(f"\nLine {line_num}: {line.rstrip()}")
                        print(f"=== Result Code: 0x{value:08X} ===")
                        print(format_decode(value))
    except FileNotFoundError:
        print(f"Error: Log file not found: {args.log_file}", file=sys.stderr)
        sys.exit(1)

    if not seen:
        print("No result codes found in log file.")


def main():
    parser = argparse.ArgumentParser(
        description="ADU Gen2 Result Code (RC/ERC) Decoder",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  %(prog)s 0x01010001              Decode a hex result code
  %(prog)s 16842753                Decode a decimal result code
  %(prog)s 0x01010001 0x02030007   Decode multiple codes
  %(prog)s --list --facility AGENT List all AGENT facility codes
  %(prog)s --search timeout        Search for codes related to "timeout"
  %(prog)s --parse-log agent.log   Find and decode codes in a log file
""",
    )

    parser.add_argument(
        "codes",
        nargs="*",
        help="Result code(s) to decode (hex 0x... or decimal)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List all registered result codes",
    )
    parser.add_argument(
        "--facility",
        type=str,
        help="Filter by facility name (AGENT, DOWNLOAD, WORKFLOW, EXTENSION, COMM, SECURITY)",
    )
    parser.add_argument(
        "--category",
        type=str,
        help="Filter by category name (CONFIG, AUTH, IO, PROTOCOL, RESOURCE, STATE, DAG, SCRIPT, APT, SWUPDATE)",
    )
    parser.add_argument(
        "--search",
        type=str,
        dest="query",
        help="Search codes by keyword",
    )
    parser.add_argument(
        "--parse-log",
        type=str,
        dest="log_file",
        help="Parse a log file for result codes and decode them",
    )

    args = parser.parse_args()

    if args.log_file:
        cmd_parse_log(args)
    elif args.query:
        cmd_search(args)
    elif args.list:
        cmd_list(args)
    elif args.codes:
        cmd_decode(args)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
