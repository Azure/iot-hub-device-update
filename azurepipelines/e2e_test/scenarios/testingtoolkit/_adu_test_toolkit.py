# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
from azure.identity import ClientSecretCredential
from azure.iot.deviceupdate import DeviceUpdateClient
from azure.iot.hub import IoTHubRegistryManager
from azure.core.rest import HttpRequest, HttpResponse
from azure.iot.hub.protocol.models import Device
from azure.iot.hub.protocol.models import Module
from azure.iot.hub.protocol.models import Twin
from urllib.parse import urljoin
import base64
import datetime
import getopt
import json
from msrest.exceptions import HttpOperationError
import os
import sys
import uuid


class DuAutomatedTestConfigurationManager():
    def __init__(self, aduEndpoint="", aduInstanceId="", iotHubUrl="", iotHubConnectionString="", aadRegistrarClientId="", aadRegistrarTenantId="", pathToCertificate="", passwordForCertificate="") -> None:
        """
        Convenience wrapper for the configuration details required for DU automated tests to run.
        """
        self._aduEndpoint = ""
        self._aduInstanceId = ""
        self._iotHubUrl = ""
        self._iotHubConnectionString = ""
        self._aadRegistrarClientId = ""
        self._aadRegistrarTenantId = ""
        self._pathToCertificate = ""
        self._passwordForCertificate = ""
        self._configured = False

    @classmethod
    def FromCliArgs(cls):

        newCls = cls()
        newCls.ParseFromCLI()
        return newCls

    @classmethod
    def FromOSEnvironment(cls):
        newCls = cls()
        newCls.ParseFromOsEnviron()
        return newCls

    def ParseFromOsEnviron(self):
        self._aduEndpoint = os.environ['ADU_ENDPOINT']
        self._aduInstanceId = os.environ['ADU_INSTANCE_ID']
        self._iotHubUrl = os.environ['IOTHUB_URL']
        self._iotHubConnectionString = os.environ['IOTHUB_CONNECTION_STRING']
        self._aadRegistrarClientId = os.environ['AAD_REGISTRAR_CLIENT_ID']
        self._aadRegistrarTenantId = os.environ['AAD_REGISTRAR_TENANT_ID']
        self._aadRegistrarClientSecret = os.environ['AAD_CLIENT_SECRET']

        self._configured = True

    def ParseFromCLI(self):
        #
        # Get opt will use the following for arguments.
        # (-a)--adu-endpoint
        # (-i)--adu-instance-id
        # (-u)--iothub-url
        # (-c)--iothub-connection-string
        # (-r)--aad-registrar-client-id
        # (-t)--aad-registrar-tenant-id
        # (-p)--aad-registrar-client-secret
        shortopts = "a:i:u:c:r:t:p:"
        longopts = ['adu-endpoint=', 'adu-instance-id=', 'iothub-url=', 'iothub-connection-string=',
                    'aad-registrar-client-id=', 'aad-registrar-tenant-id=', 'aad-registrar-client-secret=', ]
        optlist, args = getopt.getopt(sys.argv[1:], shortopts, longopts)

        #
        # Require that all parameters have been passed
        #

        if (len(optlist) < 7):
            return None

        for opt, val in optlist:
            if (opt == "-a" or opt == "--" + longopts[0]):
                self._aduEndpoint = val
            elif (opt == "-i" or opt == "--" + longopts[1]):
                self._aduInstanceId = val
            elif (opt == "-u" or opt == "--" + longopts[2]):
                self._iotHubUrl = val
            elif (opt == "-c" or opt == "--" + longopts[3]):
                self._iotHubConnectionString = val
            elif (opt == "-r" or opt == "--" + longopts[4]):
                self._aadRegistrarClientId = val
            elif (opt == "-t" or opt == "--" + longopts[5]):
                self._aadRegistrarTenantId = val
            elif (opt == "-p" or opt == "--" + longopts[6]):
                self._aadRegistrarClientSecret = val

        self._configured = True

    def CreateClientSecretCredential(self):

        if (not self._configured):
            print("Calling CreateClientSecretCredential without configuring first")
            return None

        return ClientSecretCredential(tenant_id=self._aadRegistrarTenantId, client_id=self._aadRegistrarClientId, client_secret=self._aadRegistrarClientSecret)

    def CreateDeviceUpdateTestHelper(self, credential=None):

        if (not self._configured):
            print("Calling CreateDeviceUpdateTestHelper without configuring first")
            return None
        self.credential = credential
        if (self.credential == None):
            self.credential = self.CreateClientSecretCredential()

        return DeviceUpdateTestHelper(self._aduInstanceId, self._iotHubUrl, self._iotHubConnectionString, self.credential, self._aduEndpoint)


class DeploymentSubGroupStatus():
    def __init__(self, subGroupJson) -> None:
        self.groupId = subGroupJson["groupId"]
        self.deviceClassId = subGroupJson["deviceClassId"]
        self.deploymentState = subGroupJson["deploymentState"]
        self.totalDevices = subGroupJson["totalDevices"]
        self.devicesInProgressCount = subGroupJson["devicesInProgressCount"]
        self.devicesCompletedFailedCount = subGroupJson["devicesCompletedFailedCount"]
        self.devicesCompletedSucceededCount = subGroupJson["devicesCompletedSucceededCount"]
        self.devicesCanceledCount = subGroupJson["devicesCanceledCount"]

        if ("error" in subGroupJson):
            self.error = subGroupJson["error"]


class DeploymentStatusResponse():
    def __init__(self, deploymentStatusJson) -> None:
        """
        Convenience wrapper object for the deployment status response. Converts the JSON returned by
        the service request into a Python object that makes it easier to access.

        You can see the types and potential values of each of the variables here: https://docs.microsoft.com/en-us/rest/api/deviceupdate/2021-06-01-preview/device-management/get-deployment-status
        """
        self.deploymentState = ""
        self.ParseResponseJson(deploymentStatusJson)

    def ParseResponseJson(self, deploymentStatusJson):
        self.deploymentState = deploymentStatusJson["deploymentState"]
        self.groupId = deploymentStatusJson["groupId"]

        subgroupStatus = deploymentStatusJson["subgroupStatus"]

        self.subgroupStatuses = []
        for status in subgroupStatus:
            self.subgroupStatuses.append(DeploymentSubGroupStatus(status))


class DiagnosticsDeviceStatus:
    def __init__(self,deviceStatusJson):
        self.deviceId = deviceStatusJson["deviceId"]
        self.moduleId = ""

        if ("moduleId" in deviceStatusJson):
            self.moduleId = deviceStatusJson["moduleId"]

        self.status = deviceStatusJson["status"]

        if ("resultCode" in deviceStatusJson):
            self.resultCode = deviceStatusJson["resultCode"]
        else:
            print ("No result code")
        if ("extendedResultCode" in deviceStatusJson):
            self.extendedResultCode = deviceStatusJson["extendedResultCode"]
        else:
            print("No extended result code")
        self.logLocation = deviceStatusJson["logLocation"]

class DiagnosticLogCollectionStatusResponse():
    def __init__(self):
        """
        Convenience wrapper object for the diagnostic log collection status response.
        Converts the JSON returned by the service request into a Python object that makes it easier to access.
        You can see the types and potential values of each of the variables here: https://docs.microsoft.com/en-us/rest/api/deviceupdate/2021-06-01-preview/device-management/get-deployment-status
        """
        super().__init__()
        self.operationId = ""
        self.status = ""
        self.createdDateTime = ""
        self.lastActionDateTime = ""
        self.deviceStatus = []

    def ParseResponseJson(self, logCollectionStatusResponseJson):
        self.operationId = logCollectionStatusResponseJson["operationId"]
        self.createdDateTime = logCollectionStatusResponseJson["createdDateTime"]
        self.lastActionDateTime = logCollectionStatusResponseJson["lastActionDateTime"]
        self.status = logCollectionStatusResponseJson["status"]

        deviceStatuses = logCollectionStatusResponseJson["deviceStatus"]

        for status in deviceStatuses:
            deviceStatus = DiagnosticsDeviceStatus(status)
            self.deviceStatus.append(deviceStatus)

        return self

class UpdateId():
    def __init__(self, provider, name, version) -> None:
        """
        Convenience wrapper for update-ids being used for deployments following the standard methods.
        """
        self.provider = provider
        self.name = name
        self.version = version

    def __str__(self) -> str:
        return '{ "provider":"' + str(self.provider) + '", "name": "' + str(self.name) + '", "version": "' + str(self.version) + '"}'

class DeviceUpdateTestHelper:
    def __init__(self, aduInstanceId, iothubUrl, iothub_connection_string, adu_credential, endpoint="") -> None:
        """
        Test Helper for the Device Update Test Automation work. The object wraps different parts of the required functions
        and works with the azure.iot.deviceupdate.DeviceUpdateClient and azure.iot.hub.IotHubRegistryManager objects to
        complete all of these operations

        :param str aduInstanceId: The InstanceId for the DeviceUpdate Account to be connected to
        :param str credential: The Azure token credential object retrieved from azure-identity
        :param str endpoint: The endpoint for the Device Update instance
        :ivar str _aduInstanceId: The InstanceId for the DeviceUpdate Account to be connected to
        :ivar str _aduEndpoint: The endpoint for the Device Update instance
        :ivar AzureCredential _credential: The Azure token credential object retrieved from azure-identity
        :ivar str _iotHubUrl: The url for the iothub to be used for testing
        :ivar DeviceUpdateClient _aduAcnt: The Device Update Client account object instantiated using the parameters used for DeviceUpdate operations
        :ivar IotHubRegistryManager _hubRegistryManager: The IotHubRegistryManager object instantiated using the parameters used for IotHub operations
        """

        # Internal Values for managing the connection to DU and the IotHub
        self._aduInstanceId = aduInstanceId
        self._aduEndpoint = endpoint
        self._iothub_connection_string = iothub_connection_string
        self._aduCredential = adu_credential
        self._iotHubUrl = iothubUrl
        self._aduAcnt = DeviceUpdateClient(endpoint, aduInstanceId, adu_credential)

        self._hubRegistryManager = IoTHubRegistryManager.from_connection_string(iothub_connection_string)

        self._base_url = f'https://{self._aduEndpoint}/deviceUpdate/{self._aduInstanceId}/'
        self._iotHubApiVersion = "?api-version=2021-06-01-preview"
        self._aduApiVersion= "?api-version=2022-07-01-preview"

    def CreateDevice(self, deviceId, isIotEdge=False):
        """
        Use the IotHubRegistryManager to create the device using the passed deviceId and isIotEdge parameters using the IotHubRegistryManager

        :param str deviceId: The device-id to create the device with
        :param bool isIotEdge: Flag that determines whether the device should be created as an IotEdge device or an IotDevice
        :returns: A connection string on success; An empty string on failure
        """
        primary_key = base64.b64encode(str(uuid.uuid4()).encode()).decode()
        secondary_key = base64.b64encode(str(uuid.uuid4()).encode()).decode()
        device_status = "enabled"

        self._hubRegistryManager.create_device_with_sas(deviceId, primary_key, secondary_key, device_status, isIotEdge)

        # Should always be of type Device
        device = self._hubRegistryManager.get_device(deviceId)

        if (type(device) != Device):
            print(
                "Return type for retrieving the device state is not Device. Requested Raw response?")

        # Can't guarantee that the device will be connected but the generation-id should work
        if (device.generation_id == ""):
            return ""

        # Once we can confirm that the device has been created we can make the connection string
        connectionString = "HostName=" + self._iotHubUrl + ";DeviceId=" + deviceId + ";SharedAccessKey=" + primary_key

        return connectionString

    def DeleteDevice(self, deviceId):
        """
        Deletes the device specified by deviceId

        :param str deviceId: the identifier of the device to be deleted
        :returns: True on successful deletion, false otherwise
        """
        try:

            self._hubRegistryManager.delete_device(deviceId)

        except HttpOperationError:
            return False

        return True

    def CreateModuleForExistingDevice(self, deviceId, moduleId):
        """
        Use the IotHubRegistryManager to create the module on the device using the passed deviceId and moduleId parameters using the IotHubRegistryManager

        :param str deviceId: The device on which to create the module
        :param str moduleId: The module name to be used when creating the module
        :returns: A connection string on success; An empty string on failure
        """
        primary_key = base64.b64encode(str(uuid.uuid4()).encode()).decode()
        secondary_key = base64.b64encode(str(uuid.uuid4()).encode()).decode()
        device_status = "enabled"

        self._hubRegistryManager.create_module_with_sas(deviceId, moduleId, managed_by="", primary_key=primary_key, secondary_key=secondary_key)

        module = self._hubRegistryManager.get_module(deviceId, moduleId)

        if (module.generation_id == ""):
            return ""

        # Once we can confirm that the module has been created we can make the connection string
        connectionString = "HostName=" + self._iotHubUrl + ";DeviceId=" + deviceId + ";ModuleId=" + moduleId + ";SharedAccessKey=" + primary_key

        return connectionString

    def DeleteModuleOnDevice(self, deviceId, moduleId):
        """
        Deletes the module specified by moduleId on the device

        :param str deviceId: the device for the module
        :param str moduleId: the module to be deleted
        :returns: True on successful deletion, False otherwise
        """

        try:

            self._hubRegistryManager.delete_module(deviceId, moduleId)

        except HttpOperationError:
            return False

        return True

    def AddDeviceToGroup(self, deviceId, groupName):
        """
        Patches the device twin "tags" value on the device twin to include the key ADUGroup and value of the parameter groupName

        :param str deviceId: the device-id of the device to add to the group
        :param str groupName: Name of the group to add the device to
        :returns: True on success; False on failure
        """

        newTagForTwin = Twin(tags={"ADUGroup": groupName})

        updatedTwin = self._hubRegistryManager.update_twin(deviceId, newTagForTwin)

        if (updatedTwin.tags["ADUGroup"] != groupName):
            return False

        return True

    def AddModuleToGroup(self, deviceId, moduleId, groupName):
        """
        Patches the module twin "tags" value on the device twin to include the key ADUGroup and value of the parameter groupName

        :param str deviceId: the device-id of the device for the module
        :param str moduleId: the module-id for the module to which to add the group
        :param str groupName: Name of the group to add the module to
        :returns: True on success; False on failure
        """

        newTagForTwin = Twin(tags={"ADUGroup": groupName})
        twin = self.GetDeviceTwinForDevice(deviceId)

        updatedTwin = self._hubRegistryManager.update_module_twin(deviceId, moduleId, newTagForTwin)

        if (updatedTwin.tags["ADUGroup"] != groupName):
            return False

        return True

    def GetDeviceTwinForDevice(self, deviceId):
        """
        Returns the twin of the device

        :param str deviceId: Identifier for the device to retreive the Twin for
        :returns: An object of type azure.iot.hub.protocols.models.Twin which encapsulates the twin
        """
        return self._hubRegistryManager.get_twin(deviceId)

    def GetModuleTwinForModule(self, deviceId, moduleId):
        """
        Returns the twin of the module

        :param str deviceId: Identifier for the device for the module
        :param str moduleId: Identifier for the module for which to retrieve the twin
        :returns: An object of type azure.iot.hub.protocols.models.Twin which encapsulates the twin
        """
        return self._hubRegistryManager.get_module_twin(deviceId, moduleId)

    def GetConnectionStatusForDevice(self, deviceId):
        """
        Returns the connection state of the device

        :param str deviceId: the deviceId for the device
        :returns: the string representation of the connection_state
        """
        return self._hubRegistryManager.get_device(deviceId).connection_state

    def GetConnectionStatusForModule(self, deviceId, moduleId):
        """
        Returns the connection state of the module

        :param str deviceId: the deviceId for the device
        :param str moduleId: the module for which to retrieve the connection_state
        :returns: the string representation of the connection_state
        """
        return self._hubRegistryManager.get_module(deviceId, moduleId).connection_state

    def StartDeploymentForGroup(self, deploymentId, groupName, updateId):
        """
        Starts the deployment for the specified groupname and updateId

        :param str deploymentId: the id for the deployment
        :param str groupName: the id for the group to which to deploy the update
        :param str updateId: the update-id for the deployment
        :param str rollbackPolicy: the string version of the rollback policy
        :param str failure: the string body of the failure definition
        :returns: the status code for the deployment request, 200 for success, all other values are failures
        """
        requestString = "/deviceUpdate/" + self._aduInstanceId + "/management/groups/" + groupName + "/deployments/" + deploymentId + self._aduApiVersion

        if (type(updateId) != UpdateId):
            print("Unusable type for updateId, use the UpdateId class")

        jsonBodyString = '{"deploymentId": "' + deploymentId + '","groupId": "' + groupName + '","startDateTime": "' + str(datetime.datetime.now()) + '","update":{ "updateId": ' + str(updateId) + ' } }'

        deploymentStartRequest = HttpRequest("PUT", requestString, json=json.loads(jsonBodyString))

        deploymentStartResponse = self._aduAcnt.send_request(deploymentStartRequest)

        return deploymentStartResponse.status_code

    def StopDeployment(self, deploymentId, groupName, deviceClassId):
        """
        Stops the deployment for the specified groupname

        :param str deploymentId: the id for the deployment
        :param str groupName: the id for the group to which to cancel the deployment
        :returns: the status code for the deployment cancel request, 200 for success, all other values are failures
        """
        # /deviceUpdate/{instanceId}/management/groups/{groupId}/deviceClassSubgroups/{deviceClassId}/deployments/{deploymentId}
        requestString = "/deviceUpdate/" + self._aduInstanceId + "/management/groups/" + groupName + "/deployments/" + deploymentId + self._aduApiVersion

        deploymentCancelRequest = HttpRequest("POST", requestString)

        deploymentCancelResponse = self._aduAcnt.send_request(deploymentCancelRequest)

        return deploymentCancelResponse.status_code

    def GetDeploymentStatusForGroup(self, deploymentId, groupName):
        """
        Returns the deployment state for the given deploymentId and groupName

        :param str deploymentId: the id for the deployment
        :param str groupName: the id for the group to which the deployment was made
        :returns: An object of type DeploymentStatusResponse, this will be empty on failure
        """
        deploymentStatusRequest = HttpRequest("GET", "/deviceUpdate/" + self._aduInstanceId + "/management/groups/" + groupName + "/deployments/" + deploymentId + "/status" + self._aduApiVersion)

        deploymentStatusResponse = self._aduAcnt.send_request(deploymentStatusRequest)

        if (deploymentStatusResponse.status_code != 200):
            return None

        deploymentStateResponseJson = DeploymentStatusResponse(deploymentStatusResponse.json())

        return deploymentStateResponseJson

    def DeleteDeployment(self, deploymentId, groupId):
        """
        Deletes the deployment specified by deploymentId for group specified by groupId

        :param str deploymentId: the identifier for the deployment to be deleted
        :param str groupId: the group-id for group on which the deployment was operating
        :returns: the status code of the response to delete the deployment
        """
        requestString = '/deviceUpdate/' + self._aduInstanceId + '/management/groups/' + groupId + '/deployments/' + deploymentId + self._aduApiVersion

        deleteDeploymentRequest = HttpRequest("DELETE", requestString)

        deleteDeploymentResponse = self._aduAcnt.send_request(deleteDeploymentRequest)

        return deleteDeploymentResponse.status_code

    def RunDiagnosticsOnDeviceOrModule(self,deviceId,operationId,description,moduleId=""):
        """
        Initiates a diagnostics log collection flow for the specified device or module using the operationId and description passed

        :param str deviceId: the device-id to target
        :param str operationId: the operation-id for the diagnostics workflow
        :param str description: the description for the diagnostics workflow
        :param str moduleId: Optional parameter only to be specified when targeting a module
        :returns: the status code of the request to start the diagnostics operation
        """
        jsonBody = None
        if (len(moduleId) == 0):
            jsonBody = {
                "deviceList":[
                    {
                        "deviceId": deviceId
                    },
                ],
                "description": description,
                "operationId":operationId
            }
        else:
            jsonBody = {
                "deviceList": [
                    {
                        "deviceId": deviceId,
                        "moduleId": moduleId
                    }
                ],
                "description": description,
                "operationId":operationId
            }

        collectLog_url = f'management/deviceDiagnostics/logCollections/{operationId}{self._iotHubApiVersion}'

        requestString = urljoin(self._base_url, collectLog_url)

        diagnosticsRequest = HttpRequest("PUT", requestString, json=jsonBody)

        diagnosticsResponse = self._aduAcnt.send_request(diagnosticsRequest)

        return diagnosticsResponse.status_code

    def GetDiagnosticsLogCollectionStatus(self, operationId):
        """
        Returns the log collection state for the given operationId
        :param str operationId: Log collection operation identifier
        :returns: An object of type DiagnosticLogCollectionStatusResponse, this will be empty on failure
        """
        getLogCollectDetail_url = f'management/deviceDiagnostics/logCollections/{operationId}/detailedstatus{self._aduApiVersion}'

        requestString = urljoin(self._base_url, getLogCollectDetail_url)

        logCollectionStatusRequest = HttpRequest("GET", requestString)

        logCollectionStatusResponse = self._aduAcnt.send_request(logCollectionStatusRequest)

        if (logCollectionStatusResponse.status_code != 200):
            return DiagnosticLogCollectionStatusResponse()

        logCollectionStatusResponseJson = DiagnosticLogCollectionStatusResponse().ParseResponseJson(logCollectionStatusResponse.json())

        return logCollectionStatusResponseJson

    def DeleteADUGroup(self, aduGroupId):
        """
        Deletes the ADUGroup declared by aduGroupId

        :param str aduGroupId: the ADU Group to delete
        :returns: the status code of the response
        """
        requestString = '/deviceUpdate/' + self._aduInstanceId + '/management/groups/' + aduGroupId + self._aduApiVersion

        deleteAduGroupRequest = HttpRequest("DELETE", requestString)

        deleteAduGroupResponse = self._aduAcnt.send_request(deleteAduGroupRequest)

        return deleteAduGroupResponse.status_code


