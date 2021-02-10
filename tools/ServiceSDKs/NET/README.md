# Azure Device Update client library for .NET

The library provides access to the Azure Device Update service that enables customers to publish updates for their IoT devices to the cloud, group their IoT devices and then deploy these updates to their device groups.

 [Azure Device Update product documentation](https://github.com/Azure/adu-private-preview/blob/master/docs/adu-overview.md)


## Getting started

The complete Microsoft Azure SDK can be downloaded from the [Microsoft Azure Downloads](https://azure.microsoft.com/en-us/downloads/?sdk=net) page and ships with support for building deployment packages, integrating with tooling, rich command line tooling, and more.

For the best development experience, developers should use the official Microsoft NuGet packages for libraries. NuGet packages are regularly updated with new functionality and hotfixes.

### Prerequisites

- Microsoft Azure Subscription: To call Microsoft Azure services, you need to create an [Azure subscription](https://azure.microsoft.com/free/)
- Azure Device Update instance
- Azure IoT Hub instance

### Install the package

Install the Azure Device Update [client library for .NET](https://github.com/Azure/adu-private-preview-sdk/tree/main/NET/Library) by adding a reference to Azure.Iot.DeviceUpdate.dll assembly to your project.

### Authenticate the Client

In order to interact with the Azure Device Update service, you will need to create an instance of a [TokenCredential class](https://docs.microsoft.com/en-us/dotnet/api/azure.core.tokencredential?view=azure-dotnet) and pass it to the constructor of your UpdateClient, DeviceClient and DeploymentClient class.

## Key concepts

Azure Device Update is a managed service that enables you to deploy over-the-air updates for your IoT devices. The client library has three main components:
- **UpdatesClient**: update management (import, enumerate, delete, etc.)
- **DevicesClient**: device management (enumerate devices and retrieve device properties)
- **DeploymentsClient**: deployment management (start and monitor update deployments to a set of devices)

## Troubleshooting

All Azure Device Update service operations will throw a RequestFailedException on failure with helpful ErrorCodes.

For example, if you use the `GetUpdateAsync` operation and the model you are looking for doesn't exist, you can catch that specific [HttpStatusCode](https://docs.microsoft.com/en-us/dotnet/api/system.net.httpstatuscode?view=netcore-3.1) to decide the operation that follows in that case.

```csharp
try
{
    Response<Update> update = await _updatesClient.GetUpdateAsync(
      "provider", "name", "1.0.0.0")
      .ConfigureAwait(false);
}
catch (RequestFailedException ex) when (ex.Status == (int)HttpStatusCode.NotFound)
{
    // Update does not exist.
}

```

## Examples and next steps 

Get started with our Azure Device Update .NET [samples](https://github.com/Azure/adu-private-preview-sdk/tree/main/NET/NET%20Samples), [libraries](https://github.com/Azure/adu-private-preview-sdk/tree/main/NET/Library) and [swagger](https://github.com/Azure/adu-private-preview-sdk/tree/main/Swagger)

Please send an email to aduprivatepreviewsdk@microsoft.com to request access to these links.

## Contributing

If you encounter any bugs or have suggestions, please file an issue in the [Issues](https://github.com/Azure/azure-sdk-for-net/issues) section of the project.



