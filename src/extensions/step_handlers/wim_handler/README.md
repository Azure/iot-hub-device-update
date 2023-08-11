# WIM Handler

## What is WIM?

(TO DO)

## What is the WIM Handler?

(TO DO)

See [How To Implement A Custom Content Handler Extension](../../../../docs/agent-reference/how-to-implement-custom-update-handler.md) for more information.

## WIM Handler Goals

(TO DO)

## Architecture

(TO DO)

## How To Use WIM Handler

(TO DO)

### Registering WIM Handler extension

To enable WIM on the device, the handler must be registered to support **'microsoft/wim:1'**.

Following command can be run manually on the device:

```PowerShell
AducIotAgent.exe --update-type 'microsoft/wim:1' --register-content-handler <full path to the handler file>
```

### Setting WIM Handler Properties

(TO DO)
