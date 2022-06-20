# ADU Testing Toolkit For Python

## Introduction

The goal of this project is to write testing helpers that ease the creation of tests for the DU Client.

These are for testing. Not for running high level operations. If you're looking for easy to use Python extensions for Device Update for IotHub please look [here](https://github.com/Azure/azure-sdk-for-python). The Testing Toolkit was written in Python 3.9. It is expected that any tests making use of the testing toolkit also uses that version.

## Dependencies

### Language Support

This toolkit was explicitly written for Python 3.9. Other versions have not been tested.

### Azure SDK For Python azure-iot-deviceupdate

This toolkit is expected to be used with the DeviceUpdate package for Python from the Azure SDK for Python. While the sdk does not directly implement the configurations held here it does provide the user with a solid basis for running requests against a DeviceUpdate Account. After setting up the DeviceUpdateClient, the toolkit then builds HTTPRequests for each of the supported operations.

You can see an example of the toolkit being used [here](https://github.com/Azure/azure-sdk-for-python/blob/main/sdk/deviceupdate/azure-iot-deviceupdate/samples/sample_hello_world.py)

You can install just the azure-iot-deviceupdate module with

```shell
    pip install azure-iot-deviceupdate
```

You can also clone the azure-sdk-for-python repository and look at the module source files if you have questions. The repository is extremely verbose. It's a very useful reference.

### Azure IoT SDK for Python azure.iot.hub

The toolkit also makes use of the azure.iot.hub Python module for running requests against the IotHub. This package comes from the Azure IoT SDK Python found [here](https://github.com/Azure/azure-iot-sdk-python). If you're examining the contents, wondering about return types, or objects it's highly recommended you clone the repository and look at the underlying layers.

For the Python Module you can install it with

```shell
    pip install azure.iot.hub
```

### Azure Identity

In order to make a connection with the IotHub or Adu Account you need to have an associated identity. The toolkit is built to run using an (Azure Active Directory Application Registrar)[https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app]. Using the AAD Registrar and the azure.identity package we can create a connection to the Adu Account for sending messages/data/commands to the DU REST API. 

The module can be installed using: 

```shell
    pip install azure-identity
```

Then in your script if you need to load the dependency you can use something like the following snippet to load the AAD Registrar's data into a credential that can be used for authenticating calls:

```python
from azure.identity import ClientSecretCredential

credential=ClientSecretCredential(tenant_id=_aadRegistrarTenantId, client_id=_aadRegistrarClientId, client_secret=_aadRegistrarClientSecret)
```

### How to Install Dependencies

The dependencies can be installed by running the following:

```shell
    pip install azure-iot-deviceupdate azure.iot.hub azure-identity
```

or using the requirements.txt file:

```shell
    python3 -m pip install -r <path-to-requirements-file>
```

## Requirements for instantiating a Device Update client in Python

Note this is a developers note and NOT something required to be completed by the user. All the user needs to know is how to get those values and pump them through to the testing toolkit.

1. A Token Credential: retrieved from the Azure Identity using the code above
2. The DU endpoint: the endoint for the DeviceUpdate account
3. The DU Instance Id: the instance identifier for the du instance you're trying to connect to.

## Requirements for instantiating an IotHubRegistryManager client in Python

1. An IotHub URL: The URL for the iothub you're trying to connect to
2. A Token Credential: retrieved from Azure IDentity using the code above.

## Supported Operations

1. Create Devices or Modules using a subscription or account-id
2. Retrieve a Module or Device twin
3. Delete devices or modules
4. Add an IotHub group to a device or module
5. Create an ADU Group from an IotHub Group
6. Delete an ADU Group
7. Query for the connection status of a device
8. Create a deployment
9. Start a deployment
10. Query for the status and metadata surrounding a deployment
11. Delete a deployment
12. Run a diagnostics workflow

## What is generally needed to send an HTTP Request? 

There are three values that will always be required :

1. The Account endpoint (Account refers to DU Account we should be able to get from above... maybe.)
2. The Instance-Id (the accounts instance identifier)

## What is needed to Authenticate The IotHubRegistryManager?

This section is an expansion of the one above describing how and what we need to load so we can get the IotHubRegistryManager online. 

The IotHub Registry manager is able to connect to an IotHub's device and module registration process for creating, managing, and retrieving portions of the device and module twin. To authenticate the connection to the IotHubRegistry manager we need to use the [Azure Identity Python Package](https://pypi.org/project/azure-identity/). The package can be installed using:

```bash
    pip install azure-identity
```

For the actual setup we expect all of these scripts to be running in the Pipelines within the context of an Azure CLI. The IotHubs we're using SHOULD (not tested still working on that) just require us to use the DefaultCredential values to get access to the DU account.

You can see an [example](https://github.com/Azure/azure-iot-sdk-python/blob/main/azure-iot-hub/samples/iothub_registry_manager_token_credential_sample.py) for the type of authentication we're looking at.

## Using the Toolkit in Tests

For using the toolkit it's recommended to use the `DuAutomatedTestConfigurationManager` class to create the `DeviceUpdateTestHelper` class. It manages all of the variables that need to be passed to it in order to make a connection. You can either instantiate the class using command line arguments (implemented for testing) or use the OS Environment variables like the pipelines do. The choice is up to you but any test that is added to the pipeline will be required to use the OS environment variables. 
