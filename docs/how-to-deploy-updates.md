# How to Deploy Updates

Use this section to create and monitor update deployments to your managed devices. 

## Create a Goal state deployment

1. Select the Groups and Deployments tab at the top of the page.
2. View the compliance chart and groups list. You should see a new update available for your tag based or default group
3. Select Deploy next to the "one or more updates available", and confirm that the Descriptive label you added when importing is present and looks correct.
4. Confirm the correct group is selected as the target group. 
5. Click Deploy
6. Select 'Start Immediately' or schedule to a desired date and time
7. Create a automatic rollback policy if needed
8. Select Create
9. View the compliance chart. You should see the update is now in progress.


## Monitor the created goal state deployment

1. Select the group you deployed to, and go to the Current Updates or Deployment History tab to confirm that the deployment is in progress
2. Here you can view the deployment details, update details, and target device class details. You can optionally add a friendly name for the device class
3. Select Refresh to view the latest status details. 
4. You can go to the group basics view to search the status for a particular device, or filter to view devices that have failed the deployment


## Azure IoT CLI instructions

1. Ensure you have the ADU bash CLI https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer 
2. Follow the instructions in the manage deployment area https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer#managing-deployments 
3. Example command to create deployment with cloud-initiated rollback policy
  
    ```markdown
    az iot device-update device deployment create -n test-adu-35d878a5c16248 -i myinstance1 --group-id MyGroup --deployment-id mydeployment1002 --update-provider digimaun --update-name adutest --update-version 1.0.0.2 --failed-count 10 --failed-percentage 20 --rollback-update-provider digimaun --rollback-update-name adutest --rollback-update-version 1.0.0.0
    ```
4. Group management - https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer#device-groups
	```markdown
			§ az iot device-update device group show -n test-adu-35d878a5c16248 -i myinstance1 --group-id MyGroup --update-compliance
    ```
5. Class management - https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer#device-classes
	```markdown
			§ az iot device-update device class show -n test-adu-35d878a5c16248 -i myinstance1 --class-id cdad9d100395aba0a920525b4fe6ac24c6c7ad44 --gid MyGroup --update-compliance
    ```
6. Deployment management - https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer#managing-deployments
	```markdown
			§ az iot device-update device deployment show -n test-adu-35d878a5c16248 -i myinstance1 --gid MyGroup --deployment mydeployment1002 --status
    ```
    ```markdown
			§ az iot device-update device deployment show -n test-adu-35d878a5c16248 -i myinstance1 --gid MyGroup --cid cdad9d100395aba0a920525b4fe6ac24c6c7ad44 --deployment mydeployment1002 --status
    ```
    ```markdown
			§ az iot device-update device deployment list-devices -n test-adu-35d878a5c16248 -i myinstance1 --gid MyGroup --class-id cdad9d100395aba0a920525b4fe6ac24c6c7ad44 --deployment-id mydeployment1002 --filter "deviceId eq 'd0' and deviceState eq 'InProgress'"
    ```
7. Remember you can also retry/cancel/delete deployments. Look through the CLI docs for examples.

