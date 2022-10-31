# End-To-End (E2E) Test Automation Pipeline

## DISCLAIMER

This pipeline and test suite is published here for the convenience of our customers. We make ABSOLUTELY NO COMMITMENTS to maintaining, updating, or responding to questions about the pipeline. It is provided as is.

## Introduction

This section describes the structure of the scenarios in the pipeline, how Terraform is used within this project to deploy the virtual machines (VMs) to be used for testing, the structure of the `e2etest.yml` file, and how results are reported. Do not consider this an exhaustive list of capabilities and work done on the E2E test automation infrastructure just an introduction to the project.

## Build Pipelines

Underneath the `build/` directory there are  `*.yml` files that implement the cloud build process for each of type of the Device Update Agent. Below is a matrix of currently supported pipeline builds

| Distribution |  Architecture| Yaml File |
|--------------|--------------|-----------|
| Ubuntu 18.04 | amd64 | adu-ubuntu-amd64-build.yml |
| Ubuntu 20.04 | arm64 | adu-ubuntu-arm64-build.yml |

The `*.yml` files contained in this directory are templated so they can either be called by other jobs or by the build pipelines directly.

## Directory Structure and Relevant Files

- The `scenarios/` directory contains subdirectories for each of the distribution and architecture combinations that Device Update for IotHub currently implements for automated E2E testing as well as the `testingtoolkit`.
- Within each distribution and architecture directory there is always a directory named `vm-setup` which contains the `setup.sh` script for installing the Device Update for IotHub artifact under test on the VM as well as a `devicesetup.py` used by the pipeline to create the device or module for the test and generate the `du-config.json` configuration file. These two files are used to setup each of the artifacts under test for their parent distribution and architecture scenario.
- The rest of the files in the directory are the individual tests for the scenario.

## Supported Scenarios and Scenario Development Plans

The following is a list of the currently planned scenarios that will be run by the automated E2E test pipeline:

1. APT Deployment
2. Diagnostics Log Collection
3. Multi-Component Update
4. Bundle Updates (Multi-Step Updates)
5. Agent Update

The following tests are not expected to be supported by the E2E pipeline in it's current form:

1. Nested Edge
2. SwUpdate Deployment

All together there are 7 tests that will be run per distribution, however it is likely that the Nested Edge and SwUpdate Deployment's will need their own VMs/networks to be properly configured in order for these tests to be run. They are noted here for the sake of completeness and transparency.

## Terraform Infrastructure

Terraform is an automated infrastructure deployment tool that allows the user to define elements of their infrastructure in `*.yml` files. You can read more about the Terraform on Azure [here](https://docs.microsoft.com/en-us/azure/developer/terraform/). Our Terraform script deploys Azure VM hosts defined within `terraform/hosts/` to create the VMs used for testing.

### Terraform File Structure and Use for Creating VMs

| File      | Description |
|:---------:|-------------|
| `terraform/host/DeviceUpdateHost.tf` | Defines the infrastructure needed to setup the VM for the E2E test |
| `terraform/host/variables.tf` | Defines the input variables for the `DeviceUpdateHost.tf` to tailor the VM creation process per test scenario |
| `terraform/resource_group/ResourceGroup.tf` | Defines the process for creating the resource group to be contain all the resources created per run of the pipeline |
| `terraform/resource_group/variables.tf` | Defines the input variables for creating the resource group |
| `terraform/resource_group/outputs.tf` | Exposes the variables and definitions created by `ResourceGroup.tf` |

### Terraform Variables

The following is a list of variables that the Device Update for IotHub Terraform script consumes to provision and then setup the VMs.

| Variable Name | Description | Default Value|
|---------------|-------------|--------------|
| vm_name       | name for the VM | test-device|
| image_publisher | publisher for the image to be used to create the VM | Canonical |
| image_offer   | the name of the offering to be used from the `image_publisher` | UbuntuServer  |
| image_sku     | the sku for the `image_publisher` and `image_offer` to be used to create the VM | 18.04-LTS |
| image_version | the version for the `image_sku` to be used | latest |
| environment_tag | the tag to be used for creating the resources within the Azure Subscription | "du-e2etest" |
| test_setup_tarball | this variable should be the path to or actual tarball used for the setup of the device | "" |
| vm_du_tarball_script | script commands to be used for installing the device-update artifact under test as well as supporting information | "" |

## E2E Automated Test Pipeline Automation YAML File

The E2E Automated Test Pipeline is defined in `e2etest.yaml` file. The pipeline exercises all scenarios within the `scenarios` directory. The discussion here will focus on the three stages of the `e2etest.yaml` file that are most important: the Terraform VM provisioning stage, the test execution and results posting stage, and the Terraform teardown stage.

### Pipeline Variables and Secrets

Before discussing the stages of the pipeline we need to define and discuss the pipeline variables used throughout the `e2etest.yaml` file. These are defined in Azure Dev Ops instead of the yaml file but are referenced throughout the script. They usually within `$()`. These are used for holding secrets and values that the pipeline needs to access other resources but their values are not to be public. The pipeline variables can be separated into two types. The first is for the pipeline itself to complete the VM provisioning and run standard operations. The second set of variables is used by the `testingtoolkit` to enable and authenticate communication with the IotHub and ADU account for setting up and running the tests.

#### Reference for Managing, Editing, and Viewing Pipeline Variables

| Variable Name | Description | Where it's Used | Where it comes from | How to view it doc link |
|:-------------:|-------------|-----------------|---------------------|-------------------------|
| SUBSCRIPTION_ID | ID for the subscription for creating the VMs using Terraform | Pipeline | This value comes from the Azure Subscription being used for the testing infrastructure. | [Link](https://docs.microsoft.com/en-us/azure/azure-portal/get-subscription-tenant-id#find-your-azure-subscription)|
| SERVICE_CONNECTION_NAME |The name of the Service Connection that is used to authenticate calls to Azure for deleting the resource groups and authenticating the pipeline calls to larger systems.| Pipeline |  The service connection name comes from the Azure DevOps project Service Connection and is used for connecting to other services which have given the service connection a role. | [Link](https://docs.microsoft.com/en-us/azure/devops/pipelines/library/service-endpoints?view=azure-devops&tabs=yaml#create-a-service-connection)|
| TERRAFORM_TENANT_ID |Tenant ID for the Azure Active Directory registrar that allows Terraform to access the Azure Key Vault, provision VMs, and authentication for various other services.|Pipeline| Comes from an Azure Active Directory registered application. | [Link](https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app)|
| TERRAFORM_CLIENT_ID | Client ID for the Azure Active Directory registrar that allows Terraform to access the Azure Key Vault, provision VMs, and authentication for various other services.|Pipeline| Comes from an Azure Active Directory registered application. Follow the same instructions as for the tenant ID but look for the client ID instead. | [Link](<https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app>)|
| TERRAFORM_CLIENT_SECRET | Client secret used in conjunction with the Tenant and Client IDs for the Azure Active Directory registrar that authenticates the user. |Pipeline| Comes from the secret blade within the same page you found the client and tenant ids on. | [Link](<https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app>)|
| AZURE_KEY_VAULT_ID |  The ID of the key vault where the ssh keys for the provisioned VMs are stored for remote debugging of failed scenarios.|Pipeline| Comes from the properties tab of any Azure Key Vault | [Link](<https://docs.microsoft.com/en-us/azure/key-vault/general/overview>)|
| ADU_ENDPOINT |The end point for the ADU Account that's being used for testing.|Testing Toolkit| This value is actually the HostName for the adu-account. You can find it on the Overview blade of any ADU Account | [Link](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-resources)|
| ADU_INSTANCE_ID | The instance of ADU that's being used for testing.|Testing Toolkit| Comes from the instance name tied to the ADU account being used for testing | [Link](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/create-device-update-account)|
| IOTHUB_CONNECTION_STRING |The connection string for the IotHub used to authenticate calls to the `IotHubRegistryManager`.|Testing Toolkit| Comes from the Shared access policies blade under Security Settings in your IotHub. You can also set | [Link](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security)|
| IOTHUB_URL |Also called the HostName, this is the url for the IotHub being used for testing.|Testing Toolkit| Also called the hostname comes from the IotHub overview page. You can just copy and paste the string.  |  [Link](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal)|
| AAD_REGISTRAR_TENANT_ID |Tenant ID for the Azure Active Directory registrar that authenticates the `testingtoolkit` calls to the Azure Device Update Account.|Testing Toolkit| Comes from an Azure Active Directory registered application. | [Link](https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app)|
| AAD_REGISTRAR_CLIENT_ID | Client ID for the Azure Active Directory registrar that authenticates the `testingtoolkit` calls to the Azure Device Update Account.|Testing Toolkit| Comes from an Azure Active Directory registered application. Follow the same instructions as for the tenant ID but look for the client ID instead. | [Link](https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app)|
| ADU_CLIENT_SECRET | Client secret for the Azure Active Directory registrar that authenticates the `testingtoolkit` calls to the Azure Device Update Account.|Testing Toolkit| Comes from the secret blade within the same page you found the client and tenant ids on. | [Link](https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app)|

### Building Device Update for IotHub Artifacts for Testing

The first stage in the pipeline invokes the build pipelines for all of the artifacts required for the tests. At the end of this stage all of the artifacts are published to the E2E test pipeline. At the end of a run the artifacts will always be available. In the case of a failure this allows the engineers to reproduce the error on their own VM if they like.

### Terraform VM Provisioning Stage

The Terraform VM Provisioning Section is a set of steps which are run according to the defined variables in the matrix. The steps for provisioning can be simplified to these steps:

1. The first step for provisioning the VMs is to prepare the dependencies, artifact, and configuration for the Device Update for IotHub artifact being tested.
   1. First the artifact from the build template is downloaded and placed into the scenario's `testsetup` directory (created by the script underneath the scenario e.g.`scenarios/ubuntu-18.04-amd64/testsetup/`).
   2. Next `scenario/<distribution>-<architecture>/vm-setup/devicesetup.py` script is run. The `devicesetup.py` script uses the `testingtoolkit` to communicate with the IotHubRegistry in order to create a device or module for the artifact under test to use. The `devicesetup.py` script will always output the configuration required by the artifact in order to establish a connection. The outputs are up to the scenario writer to determine but by the end of execution the scenario should have everything it needs to setup a DeviceUpdate artifact for testing.
   3. Finally the `scenario/<distribution>-<architecture>/vm-setup/setup.sh` script is copied into the `testsetup/` directory. It manages installing the Device Update artifact's dependencies, installs the configuration information generated by `devicesetup.py`, and restarts the Device Update agent. This script is expected to be run on the device during the setup step for the VM.
   4. After running the `setup.sh` script the VM is expected to be able to connect to the IotHub.
   5. Once the `testsetup/` directory has been filled with the required documents it is archived and compressed into the `testsetup.tar.gz` tarball which is then passed to the Terraform VM to be used during by the Terraform scripts to setup the artifact on the Terraform provisioned VM.
2. Once the has been created the pipeline uses the `terraform apply` command to create the VM passing the tarball and the per-scenario matrix commands for running the setup.sh command on the VM is passed along with the required values to create the VMs under the pipeline run's resource group.
3. After creating each VM the Terraform state is then published to the pipeline artifacts so Terraform is able to clean up it's dependencies later on in the process.

### Run Tests and Post Results Stage

The next stage will target each of the previously provisioned devices running each `*.py` script within the respective scenario's directory. The output from each one of these tests is an xml file in JUnit XML format. All of the tests for each of the VMs is run before all of the results are posted to the DevTest hub from which the testers and developers can view the results.

### Terraform Infrastructure Tear Down

The tear down steps use `terraform destroy` command to destroy each of the matrixed scenarios described in the VMInitialization job. There's a final step run after that cleans up the resource group for all of the resources. At the end of each successful test all infrastructure created during the run of the pipeline will be destroyed.

### Terraform VM Setup Process

Terraform accepts multiple variable inputs for creating the VM machine each time `terraform apply` is called with any host file. These variables are what are used to configure the VM for the correct distribution and architecture as well as providing the appropriate artifact and supporting material for setting up the VM for the test.

#### Terraform VM Initialization

The Terraform Host template (`terraform/host/DeviceUpdateHost.tf`) to create the VMs. Variables are passed to the VM which are used by the setup script to create and configure the VMs for testing. You can examine the `e2etest.yaml` file has an example invocation including the variables.

#### Device Update Artifact Setup On the Terraform VM

The two most important variables for the E2E Test Automation pipeline within the Terraform scripts is the `test_setup_tarball` and `vm_du_tarball_script`.

The `test_setup_tarball` is an archive containing the artifact to be tested, the configuration files for the artifact (e.g. a du-config.json, an IoTEdge config.toml, etc), and a `setup.sh` bash file that contains the steps to install the dependencies for the artifact, install the artifact itself, and then installing the appropriate configuration files for the Device Update for IotHub artifact.

The `vm_du_tarball_script` is set with the commands to unpack the archive and run the `setup.sh` script. The common value for `vm_du_tarball_script` is

```bash
    tar -xvf /tmp/testsetup.tar.gz -C ./ &&
    chmod u=rwx,g=rwx,o=rx ./testsetup/setup.sh &&
    ./testsetup/setup.sh
```

These steps should be the same for all VMs being created by the VM but the author of the `e2etest.yaml` file can change this if it becomes necessary.

## E2E Automated Test Pipeline Development Process

The development process laid out here is for adding new scenarios as well as adding new tests. As a reminder a `scenario` is defined as a new distribution and architecture combination that has not been previously implemented. A test is apart of the coverage of a scenario.

### Adding New Scenarios

The process for adapting or adding a new scenario can be accomplished by following these steps:

1. Create a new directory for the scenario under the directory `scenarios/`. The naming convention should follow `<distribution-name>-<architecture>` substituting the distribution name and the desired architecture to target.
2. Next under your new scenario directory create the `vm_setup` directory.
   1. Within this directory create the `setup.sh` script for installing the DeviceUpdate for IotHub agent on the VM as well as setup any and all needed dependencies (e.g. for an IotEdge device provisioned by the IoT Identity Service add the steps to install aziot-edge and configure it with the required information)
   2. The next step is create the `devicesetup.py` script under the same directory that creates the device in the IotHub and the corresponding `du-config.json` file. This can create whatever provisioning information or work that you'd like to have on the device but you'll have to modify the `TerraformVMInitialization` step to use that information provided your scenario needs more than the standard du-config.json file.
   3. For most cases the above two steps can be satisfied by just copying the `vm_setup` directory from any of the other scenarios and doing basic modifications. It should be easy to customize from there.
3. After creating the setup files for the VMs you need to add the scenario to the `TerraformVMInitialization` and `RunTestsAndPostResults` matrices. For both you need to modify the `matrix:` section to include the information for Terraform to create the VM an example of how Debian 10 might be added to the `TerraformVMInitialization` stage below. An explanation of what these are for and how they're used is provided in the sections above.

    ```yaml
        debian-9-amd64:
            distroName: debian-10
            image_publisher: CentOS
            image_offer: Debian10
            image_sku: debian-10
            image_version: latest
            packagePattern: "*debian-90-amd64*/*.deb"
            package_pipeline: "Azure.adu-private-preview.e2e-test"
            scenarioSetupDir: './scenarios/debian-10-amd64/'
            du_tarball_script: >-
                tar -xvf /tmp/testsetup.tar.gz -C ./ &&
                u=rwx,g=rwx,o=rx ./testsetup/setup.sh &&
                ./testsetup/setup.sh
    ```

    An example of the matrix for the `RunTestsAndPostResults` matrix is below. You can read more about what goes into this section above.:

    ```yaml
        debian-10-amd64:
            distroName: debian-10
            scenarioPath: scenarios/debian-10-amd64/
    ```

4. Once the scenario has been added to the `e2etest.yaml` file and the supporting scenario directory and it's constituent parts are set up it's time to start adding tests. All tests for the device should be added to it's scenario directory. There's expected patterns and names for these files. That is covered in the below section on adding tests.

### Adding New Tests to a Scenario

These steps assume that the scenario has already been setup under the `scenarios/` directory and that the `vm_setup` directory has already been created under that scenario directory. Each scenario is composed of a combination of tests and "meta" files for setting up and managing the device or module that's used by the scenario.

#### Meta Files for the Automated Tests

For all scenarios the test file names are the same, however there are some "meta" files that are used for setting the ADU Group, centralizing names for things that will be used by multiple tests (e.g. the device id and/or module id), and deleting the device that should also be added for every scenario.

As of the last edit of this page the meta files are:

| File Name | Description |
|---------------|-------------|
|`scenario_definitions.py` | Contains variables that are needed by multiple tests or are global to all tests to be used in the scenario (e.g. device-id/module-id, ADU Group Name, Deployment Group Name, etc) |
| `add_device_to_adu_group.py` | Takes the device id and/or module-id plus the ADU Group for the device/module defined in `scenario_definitions.py` and creates an ADU Group for that device/module with that ADU Group name |
| `delete_device.py` | Cancels or stops all deployments on the ADU Group , deletes the ADU Group, and then finally deletes the device or module for the scenario |

#### Test File Names

For each test there is a specific name that is hard coded into the yaml file for that test. The following is a list of the currently defined test files:

| Test Type/ Goal | File Name |
|-----------------|-----------|
| APT Deployment | `apt_deployment.py` |
| Diagnostics Run| `diagnostics.py` |
| Multi-Component Update | `mcu_deployment.py` |
| Bundle Update | `bundle_update.py` |
| Agent Update | `agent_update.py` |

Every scenario will have these defined in the repository. If there are no tests to be run or the test hasn't been implemented yet (say we're adding Debian 10 but we're not ready to add the full automated tests for bundle or multi-component updates) the files should be left with only the base test that allows the pipeline to run even if the file has no functionality.

#### Structure of a Test File

Before reading this section it's highly recommended that the reader peruse the ubuntu-18.04 test directory to see what the style looks like and what we're using to accomplish the actual testing. Additionally the reader should read the README.md for the Testing Toolkit that all of the Device Update for IotHub tests are built upon. The reader should not have to know how the toolkit specifically works but at least understand how to invoke and use the toolkit.

The Device Update for IotHub E2E Automation Tests are written using the Python3.9 `unittest` module and the tests output is converted into JUnit format using the `xmlrunner` `xunit_plugin`  module. It's a bit confusing but the actual output of the xmlrunner transform is NOT XUnit such as Azure Pipelines calls it (Azure Pipelines considers XUnit 2.0 .NET to be XUnit instead of just XUnit which is effectively the same format as JUnit). Instead the `xunit_plugin.transform` function allows us to transform from the Python `unittest.TestCase` object output to an Azure Pipelines consumable format (in this case JUnit or the old XUnit format).

Each test file contains one Python class that inherits from `unittest.TestCase` and has one or more methods that implement some test related to it's parent caller. The APT Deployment test for Ubuntu 18.04 has been documented to help readers start writing their own tests [here](./scenarios/ubuntu-18.04-amd64/apt_deployment.py).

For documentation on the tooling used you can read about the Python `unittest` module [here](https://docs.python.org/3/library/unittest.html), the XML Runner and XUnit plugin [here](https://github.com/xmlrunner/unittest-xml-reporting), and the Testing Toolkit itself [here](./scenarios/testingtoolkit/README.md).
