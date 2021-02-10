# Azure Device Update client library for Java

The library provides access to the Azure Device Update service that enables customers to publish updates for their IoT devices to the cloud, and then deploy these updates to their devices (approve updates to groups of devices managed and provisioned in IoT Hub). 

## Getting started

The complete Microsoft Azure SDK can be downloaded from the [Microsoft Azure Downloads](https://azure.microsoft.com/en-us/downloads/?sdk=net) page and ships with support for building deployment packages, integrating with tooling, rich command line tooling, and more.

For the best development experience, developers should use the official Microsoft NuGet packages for libraries. NuGet packages are regularly updated with new functionality and hotfixes.

### Prerequisites

- Microsoft Azure Subscription: To call Microsoft Azure services, you need to create an [Azure subscription](https://azure.microsoft.com/free/)
- Azure Device Update instance
- Azure IoT Hub instance

### Authenticate the client

In order to interact with the Azure Device Update service, you will need to create an instance of a [TokenCredential class](https://docs.microsoft.com/en-us/dotnet/api/azure.core.tokencredential?view=azure-dotnet) and pass it to the constructor of your AzureDeviceUpdateClientBuilder class.

## Key concepts

Azure Device Update is a managed service that enables you to deploy over-the-air updates for your IoT devices. The client library has three main components:

- **Updates**: update management (import, enumerate, delete, etc.)
- **Devices**: device management (enumerate devices and retrieve device properties)
- **Deployments**: deployment management (start and monitor update deployments to a set of devices)

You can learn more about Azure Device Update by visiting [Azure Device Updates](https://github.com/Azure/adu-private-preview/tree/release/v0.2.0-private-preview).

## Troubleshooting

All Azure Device Update service operations will throw a ErrorResponseException on failure with helpful ErrorCodes.

For example, if you use the `getUpdateAsync` operation and the model you are looking for doesn't exist, you can catch that specific [HttpStatusCode](https://docs.microsoft.com/en-us/dotnet/api/system.net.httpstatuscode?view=netcore-3.1) to decide the operation that follows in that case.

```java
try {
Update update = client.getUpdates().getUpdateAsync(
"provider", "name", "1.0.0.0")
 .block();
}
catch (HttpResponseException ex) {
if (ex.getResponse().getStatusCode() == 404) {
// Update does not exist.
 }
}

```

## Next steps

Get started with our Azure Device Update Java [samples](https://github.com/Azure/adu-private-preview-sdk/tree/main/Java/Java%20Samples), [libraries](https://github.com/Azure/adu-private-preview-sdk/tree/main/Java/Library) and [swagger](https://github.com/Azure/adu-private-preview-sdk/tree/main/Swagger)

Please send an email to aduprivatepreviewsdk@microsoft.com to request access to these links.

## Contributing

If you encounter any bugs or have suggestions, please file an issue in the [Issues](https://github.com/Azure/azure-sdk-for-net/issues) section of the project.