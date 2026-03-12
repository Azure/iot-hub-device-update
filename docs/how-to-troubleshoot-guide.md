# Troubleshooting Guide

For comprehensive troubleshooting documentation, visit [Device Update for IoT Hub](https://learn.microsoft.com/azure/iot-hub-device-update/). This document provides additional guidance for investigating Device Update agent issues.

## Collecting Logs

Agent logs are located at:

```
/var/log/adu
```

Delivery Optimization logs (when using DO content downloader):

```
/var/cache/deliveryoptimization-agent/log
```

> **Tip**: Use the [Diagnostics Log Collection](diagnostics-log-collection.md) feature
> to remotely collect and upload logs to Azure Storage.

## Common Issues

### Connection and Authentication

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| Agent fails to connect to IoT Hub | Incorrect connection string or expired SAS token | Verify `du-config.json` connection settings |
| X.509 authentication fails | Certificate expired, wrong path, or CA mismatch | See [X.509 Troubleshooting](agent-reference/how-to-x509-authentication.md#troubleshooting) |
| Agent connects but no deployments received | Device not in target group or deployment not created | Check IoT Hub device twin and deployment status |

### Update Processing

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| Download fails | Network issue, storage URL expired, or content downloader error | Check network connectivity; review DO or curl downloader logs |
| Delta download handler errors | Missing delta dependencies or source file not cached | See [Building with Delta Handler](agent-reference/building-with-delta-handler.md) for setup |
| Installation stuck at "In Progress" | Handler script timeout or crash | Check handler logs; review script permissions |
| Update reports failure with ExtendedResultCode | Handler or agent error | Decode the result code (see below) |

### Service Status API

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| GetAduServiceStatus returns error | API service not running or FIFO path mismatch | Verify agent is running; check FIFO file permissions |
| SDK cannot connect to agent | Permission denied on FIFO | Ensure calling process user is in the `adu` group |

## ResultCode and ExtendedResultCode

The `ADUCoreInterface` reports `ResultCode` and `ExtendedResultCode` for diagnosing failures.
See [Device Update Plug and Play](https://docs.microsoft.com/azure/iot-hub-device-update/device-update-plug-and-play) for interface details.

### ResultCode

`ResultCode` is a general status code following HTTP status code conventions.
See [HTTP Status Codes](https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html) for reference.

### ExtendedResultCode

`ExtendedResultCode` is an integer with encoded error information.

The `ExtendedResultCode` may appear as a signed integer in the PnP interface.
To decode it, convert the signed integer to unsigned hex. Only the first 4 bytes
are used, in the form `F` `FFFFFFF` where the first nibble is the **Facility Code**
and the remaining bits are the **Error Code**.

### Error Code References

- [Device Update Result Codes](../src/inc/aduc/result.h)
- [Delivery Optimization Error Codes](https://github.com/microsoft/do-client/blob/main/client-lite/src/include/do_error.h)
- [Extended Result Codes Reference](agent-reference/device-update-agent-extended-result-codes.md)

### Facility Codes

| Facility Code | Description |
|---------------|------------|
| D | Error raised from the DO SDK |
| E | Error code is an errno |

**Example**: `ExtendedResultCode` is `-536870781`

The unsigned hex representation is `FFFFFFFF E0000083`:

| Ignore | Facility Code | Error Code |
|--------|--------------|------------|
| FFFFFFFF | E | 0000083 |

`0x83` in hex = `131` in decimal = errno [`ENOTRECOVERABLE`](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/errno.h)

## Getting Help

- [GitHub Issues](https://github.com/Azure/iot-hub-device-update/issues)
- [Azure Support](https://azure.microsoft.com/support/)
- [Device Update Documentation](https://learn.microsoft.com/azure/iot-hub-device-update/)
