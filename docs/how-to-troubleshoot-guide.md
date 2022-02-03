# Troubleshooting Guide

Please check the Troubleshooting documentation first at [Device Update for IoT Hub](Aka.ms/iot-hub-device-update-docs) all issues. This document provides additional articles to help investigate Device update for IoT Hub Agent related issues.

## Device Update for IoT Hub Agent

If you run into issues with the Device Update Agent, you have a couple options to help troubleshoot. Start by collecting the Device Update logs as shown below

### Collect Logs

* On the Raspberry Pi Device there are two sets of logs found here:

    ```markdown
    /adu/logs
    ```

    ```markdown
    /var/cache/deliveryoptimization-agent/log
    ```

* For the packaged client the logs are found here:

    ```markdown
    /var/log/adu
    ```

    ```markdown
    /var/cache/deliveryoptimization-agent/log
    ```

* For the Simulator (Device Update Agent version 0.7.0 and older) the logs are found here:

    ```markdown
    /tmp/aduc-logs
    ```

### ResultCode and ExtendedResultCode

The 'ADUCoreInterface' interface reports `ResultCode` and
`ExtendedResultCode` which can be used to diagnose failures. [Learn
More in Device Update Plug and Play](https://docs.microsoft.com/azure/iot-hub-device-update-docs/concepts-device-update-plug-and-play) about the 'ADUCoreInterface' interface.

#### ResultCode

`ResultCode` is a general status code and follows http status code convention.
[Learn More](https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html) about http
status codes.

#### ExtendedResultCode

`ExtendedResultCode` is an integer with encoded error information.

You will most likely see the `ExtendedResultCode` as a signed integer in the PnP
interface. To decode the `ExtendedResultCode`, convert the signed integer to
unsigned hex. Only the first 4 bytes of the `ExtendedResultCode` are used and
are of the form `F` `FFFFFFF` where the first nibble is the *Facility Code* and
the rest of the bits are the *Error Code*.

### Error Codes explained

* [Device Update for IoT Hub](../src/inc/aduc/result.h)
* [Delivery Optimization](https://github.com/microsoft/do-client/blob/main/client-lite/src/include/do_error.h)

### Facility Codes explained

| Facility Code     | Description  |
|-------------------|--------------|
| D                 | Error raised from the DO SDK|
| E                 | Error code is an errno |

For example:

`ExtendedResultCode` is `-536870781`

The unsigned hex representation of `-536870781` is `FFFFFFFF E0000083`.

| Ignore    | Facility Code  | Error Code   |
|-----------|----------------|--------------|
| FFFFFFFF  | E              | 0000083      |

`0x83` in hex is `131` in decimal which is the errno value for `ENOLCK`.
