# Azure Device Update client library for Python

The library provides access to the Azure Device Update service that enables customers to publish updates for their IoT devices to the cloud, group their IoT devices and then deploy these updates to their device groups.

[Azure Device Update product documentation](https://github.com/Azure/adu-private-preview/blob/master/docs/adu-overview.md)

## Getting started

### Prerequisites

- Microsoft Azure Subscription: To call Microsoft Azure services, you need to create an [Azure subscription](https://azure.microsoft.com/free/)
- Azure Device Update instance
- Azure IoT Hub instance
- Python 2.7, or 3.5 or later is required to use this package.

### Install the package

Install the Azure Device Update client library for Python with [pip](https://pypi.org/project/pip/):

```
pip install {your_local_path_to}azure_deviceupdate-0.0.0.1-py2.py3-none-any.whl
pip install azure-identity
```

## Key concepts

Azure Device Update is a managed service that enables you to deploy over-the-air updates for your IoT devices. The client library has one main component named **AzureDeviceUpdateServiceDataPlane**.  The component allows you to access all three main client services:
- **UpdatesOperations**: update management (import, enumerate, delete, etc.)
- **DevicesOperations**: device management (enumerate devices and retrieve device properties)
- **DeploymentsOperations**: deployment management (start and monitor update deployments to a set of devices)

## Troubleshooting

The Azure Device Update client will raise exceptions defined in [Azure Core][azure_core].

## Examples and next steps 

Get started with our Azure Device Update Python [samples](https://github.com/Azure/adu-private-preview-sdk/tree/main/Python/Python%20Samples), [libraries](https://github.com/Azure/adu-private-preview-sdk/tree/main/Python/Libraries) and [swagger](https://github.com/Azure/adu-private-preview-sdk/tree/main/Swagger)

Please send an email to aduprivatepreviewsdk@microsoft.com to request access to these links.

## Contributing

If you encounter any bugs or have suggestions, please file an issue in the [Issues](https://github.com/Azure/azure-sdk-for-python/issues) section of the project.
