# Azure E2E Test Pipelines ReadMe

## Introduction
This document is intended as a description of the structure, flow, and meaning of the files held within this directory for the purposes of running builds and completing our automated test environment. Our goal is to provide an outline for the current state, future state, and the operation of the pipelines. 

## Build Pipelines
Underneath the `pipelinesourcebuilds/` directory there are a list of `*.yml` files that describe the build process for each of the build pipelines. These describe the individual build processes for the distribution and architecture listed in the name. Below is a matrix of currently supported pipeline builds

| Distro |  Architecture| Yaml File |
|--------|--------------|-----------|
| Ubuntu 18.04 | amd64 | adu-ubuntu-amd64-build.yml |
| Ubuntu 20.04 | arm64 | adu-ubuntu-arm64-build.yml |
| Debian 9 | arm32 | adu-debian-arm32-build.yml | 

The `*.yml` files contained in this directory are templated so they can either be called by other jobs or by the build pipelines directly. 

## End-To-End Test Automation Pipeline
### Introduction
The following is a description of the files, directories, structure, and layout of the end-to-end test automation pipeline. It will start by describing the structure of the individual scenarios, the mapping of directories to distributions and architectures, the description for the setup steps for each virtual machine, how terraform is used within this project to deploy the virtual machines to be used for testing, the structure of the `e2etest.yml` file and how it integrates with the rest of the project, and how results are reported. Please do not consider this an exhaustive list of capabilities and work done on the end-to-end test automation infrastructure. This merely serves as an introduction to the project. 

### Directory Structure and Relevant Files
The `scenarios/` directory contains subdirectories for each of the Linux distribution and architecture combinations that Device Update for IotHub currently supports for automated end-to-end testing. The top-level subdirectories are named with the distributions and architectures that are currently covered as well as the `testingtoolkit` which the end-to-end automated testing pipeline uses for running the tests. Within the distribution and architecture directories there is a directory named `vm-setup` which contains the `setup.sh` script for installing the Device Update for IotHub artifact under test on the virtual machine as well as a `devicesetup.py` which is used by the pipeline to create the device or module to be used for the test and generates the configuration file for the Device Update for IotHub artifact. These two files are used in concert to setup each of the artifacts under test for their parent distribution and architecture scenario. The rest of the files in the directory are the individual testing scenarios for the artifact under test. 

### Supported Scenarios and Scenario Development Plans
The following is a list of the currently planned scenarios that will be run by the automated end-to-end test pipeline: 
1. APT Deployment
2. Diagnostics Log Collection
3. Multi-Component Update
4. Bundle Updates (Multi-Step Updates)
5. Agent Update

The current list of manual tests run by engineers periodically throughout development and before every list is:
1. Nested Edge 
2. SwUpdate Deployment

All together there are 7 tests that will be run per distribution, however it is likely that the Nested Edge and SwUpdate Deployment's will need their own virtual machines/networks to be properly configured in order for these tests to be run. They are noted here for the sake of completeness and transparency. 

### Terraform Infrastructure

#### Introduction
Terraform is an automated infrastructure deployment tool that allows the user to define different hosts in `*.yml` files for deploying an infrastructure using Azure VMs based upon runtime configurations. You can read more about the Terraform on Azure [here](https://docs.microsoft.com/en-us/azure/developer/terraform/). Our Terraform script deploys Azure virtual machine hosts defined within `terraform/hosts/` to create the virtual machines to be used for testing. As of this writing the Terraform scripts are written such that they can deploy Ubuntu 18.04, Debian 9, Debian 10, and Ubuntu 20.04 virtual machines running on the amd64 architecture. 

#### Terraform File Structure and Use
Within the Device Update for IotHub End-To-End Automated Test pipeline the Terraform host definition file is located at `terraform/host/DeviceUpdateHost.tf`. The host file consumes the variables passed to it by the `e2etest.yml` file. These variables are defined within the `terraform/host/variables.tf` with default values but will change depending on the distribution and architecture requested by the `e2etest.yml` file. Terraform is able to handle these variable changes and allows the Device Update for IotHub End-To-End Automated Test pipeline to spawn multiple virtual machines with the same base configurations but running different distributions, architectures, and with different setup steps. 

The manner that these setup steps are applied will be described in another section.

#### Terraform Variables

The following is a list of variables that the Device Update for IotHub Terraform script consumes to provision and then setup the virtual machines. 

| Variable Name | Description | Default Value|
|---------------|-------------|--------------|
| vm_name       | name for the virtual machine | test-device|
| image_publisher | publisher for the image to be used to create the virtual machine | Canonical |
| image_offer   | the name of the offering to be used from the `image_publisher` | UbuntuServer  |
| image_sku     | the sku for the `image_publisher` and `image_offer` to be used to create the virtual machine | 18.04-LTS |
| image_version | the version for the `image_sku` to be used | latest |
| environment_tag | the tag to be used for creating the resources within the Azure Subscription | "deviceupdate-e2etest" |
| test_setup_tarball | this variable should be the path to or actual tarball used for the setup of the device | "" | 
| vm_du_tarball_script | script commands to be used for installing the device-update artifact under test as well as supporting information | "" |

#### Terraform Virtual Machine Setup Process

##### Introduction
Terraform accepts multiple variable inputs for creating the virtual machine each time `terraform apply` is called with any host file. These variables are what are used to configure the virtual machine for the correct distribution and architecture as well as providing the appropriate artifact and supporting material for setting up the virtual machine for the test. Within the `e2etest.yaml` file we take two steps for working on the Terraform virtual machines. 

##### Terraform Commands
The first is to initialize Terraform

```bash
    terraform init
```

All this completes is setting up the Terraform tool itself and doesn't perform anything specific to Device Update for IotHub. The second step is where the Terraform Host template (`terraform/host/DeviceUpdateHost.tf`) is used to create the virtual machine. The variables are passed to the virtual machine which are then used by the actual setup script to create them. Most of the variables below are standard. The subscription id, subscription id, resource group, and tenant id are the pipeline secrets used by the Terraform script to provision the virtual machine form Azure. The client id and client secret setup the ssh options so that testers can ssh into the test vms in case they need to reproduce and diagnose a problem. The key vault id is the identifier used by the virtual machine to upload logs from the virtual machine if needed. The rest of the variables are described in the section above. 

```bash 
    terraform apply subscription_id="$(SUBSCRIPTION_ID)" -var tenant_id="$(TENANT_ID)" -var client_id="$(CLIENT_ID)" -var client_secret="$(CLIENT_SECRET)" -var key_vault_id="$(KEY_VAULT_ID)" -var resource_group_name=$(RESOURCE_GROUP_NAME) -var vm_name=$(distroName) -var image_offer=$(image_offer) -var image_publisher=$(image_publisher) -var image_sku=$(image_sku) -var image_version=$(image_version) -var test_setup_tarball=$(scenarioPath)/testsetup.tar.gz -var vm_du_tarball_script=$(du_tarball_script) -auto-approve
```

##### Device Update Artifact Setup On the Terraform Virtual Machine

The two most important variables for the End-To-End Test Automation pipeline within the Terraform scripts is the `test_setup_tarball` and `vm_du_tarball_script`. 

The `test_setup_tarball` is an archive containing the artifact to be tested, the configuration files for the artifact (eg a du-config.json, an iotedge config.toml, etc), and a `setup.sh` bash file that contains the steps to install the dependencies for the artifact, install the artifact itself, and then installing the appropriate configuration files for the Device Update for IotHub artifact. 

The `vm_du_tarball_script` is set with the commands to unpack the archive and run the `setup.sh` script. The common value for `vm_du_tarball_script` is

```bash
    tar -xvf ./testsetup.tar.gz &&
    sudo ./testsetup/setup.sh
```

These steps should be the same for all virtual machines being created by the virtual machine but the author of the `e2etest.yaml` file can change this if it becomes necessary. 

### End-To-End Automated Test Pipeline Automation YAML File

#### Introduction

The End-To-End Automated Test Pipeline is defined in `e2etest.yaml` file. What follows is a description of the core components and process for running each of the test scenarios for the various distributions and architectures targeted by the automated test pipeline. The pipeline is run once per night and exercises all scenarios held underneath the `scenarios` directory (note this is the top level directory which is in the folder structure as in one of the above sections). The discussion here will focus on the three stages of the `e2etest.yaml` file that are most important: the Terraform virtual machine (can also be called per distribution and architecture) provisioning stage, the test execution and results posting stage, and the Terraform teardown stage. 

#### Terraform Virtual Machine Provisioning Stage

The Terraform Virtual Machine Provisioning Section is a set of steps which are run according to the defined variables in the matrix. The steps start with using the appropriate template build task for building the Device Update agent that will be used for the tests. The next few steps are for setting up the tarball archive used by Terraform to setup the artifact for testing, including it's dependencies and configurations. First the artifact from the build template is downloaded and placed into the scenario's `testsetup` directory (created by the script underneath the scenario eg `scenarios/ubuntu-18.04-amd64/testsetup/`). Next the `scenario/<distribution>-<architecture>/vm-setup/devicesetup.py` script is run. This script is written per distribution and architecture set. It uses the `testingtoolkit` to communicate with the IotHubRegistry in order to create a device or module for the artifact under test to use for an IotHub connection. The `devicesetup.py` script will always output the configuration required by the artifact in order to establish a connection. The artifact will always use a `du-config.json` file but might also include an IotEdge `config.toml` file already filled with the provisioning information for the testing device. The outputs are up to the scenario writer to determine. Next the `scenario/<distribution>-<architecture>/vm-setup/setup.sh` script is copied into the `testsetup/` directory. The `setup.sh` script is an informed script that expects certain outputs from the `devicesetup.py` script in order to run. It manages installing the Device Update artifact's dependencies, installs the requisite configuration information generated by `devicesetup.py`, and restarts the Device Update agent at the end of it's run. This script is expected to be run on the device during the setup step for the virtual machine. After running the `setup.sh` script the virtual machine is expected to always be able to connect to the IotHub device created for it using it's configurations. Once the `testsetup/` directory has been filled with the required documents it is tared and compressed into the `testsetup.tar.gz` tarball which is then passed to the Terraform virtual machine to be used during by the Terraform scripts to setup the artifact on the Terraform provisioned virtual machine. 

At the end of these steps there should be a Terraform provisioned virtual machine running the appropriate Device Update artifact and connected to the IotHub for every entry in the `matrix:` section of the `TerraformVMInitialization` within the `e2etest.yaml` file. 

#### Run Tests and Post Results Stage

The next stage will target each of the previously provisioned devices running each `*.py` script within the respective scenario's directory. Each of the tests addresses a singular test scenario. The output from each one of these scripts is an xml file in JUnit XML format. All of the tests for each of the virtual machines is run before all of the results are posted to the DevTest hub from which the testers and developers can view the results. 

#### Terraform Infrastructure Tear Down

The tear down steps just use the simply `terraform destroy` command sto destroy each of the matrixed scenarios described above. 

## End-To-End Automated Test Pipeline Development Process 

### Introduction 
