# WIoT Handler

## What is WIoT?

(TO DO)

## What is the WIoT Handler?

(TO DO)

See [How To Implement A Custom Content Handler Extension](../../../../docs/agent-reference/how-to-implement-custom-update-handler.md) for more information.

## WIoT Handler Goals

(TO DO)

## Architecture

(TO DO)

## How To Use WIoT Handler

(TO DO)

### Registering WIoT Handler extension

To enable WIoT on the device, the handler must be registered to support **'microsoft/wiot:1'**.

Following command can be run manually on the device:

```PowerShell
AducIotAgent.exe --update-type 'microsoft/wiot:1' --register-content-handler <full path to the handler file>
```

### Setting WIoT Handler Properties

(TO DO)
