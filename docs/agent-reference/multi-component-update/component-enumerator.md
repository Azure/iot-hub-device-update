# Device Component Enumerator

The Device Component Enumerator is a linux shared library that provide APIs that can be used for querying components on the device, and their properties.  

## APIs

The enumerator plug-in must implement following APIs:

```c++

/**
 * @brief Returns all components infomation in JSON format.
 * @params includeProperties Indicates whether to include optional component's properties in the output string.
 */
std::string GetAllComponents(const bool includeProperties) 

/**
 * @brief Returns all components in specified group in JSON format.
 * @params groupName The group name 
 * @params includeProperties Indicates whether to include optional component's properties in the output string.
 */
std::string GetComponentsByGroup(const char* groupName, const bool includeProperties) 

/**
 * @brief Returns all components in specified Component Class in JSON format.
 * @params className The Component Class name 
 * @params includeProperties Indicates whether to include optional component's properties in the output string.
 */
std::string GetComponentsByClass(const char* className, const bool includeProperties) 

```

> **NOTE:** This APIs contract is pending a design review with our partner

## Example Output

```json
{
    "manufacture" : "contoso",
    "model" : "smart-vacuum",

    "updatableComponents" : [   
        {
            "id" : "0",
            "name" : "host-firmware",
            "group" : "system",
            "manufacture" : "contoso",
            "model" : "smart-vacuum",
            "version" : "1.0.0",
            "description" : "A host device firmware"
        },
        {
            "id" : "1",
            "name" : "host-rootfs",
            "group" : "system",
            "manufacture" : "contoso",
            "model" : "smart-vacuum",
            "version" : "1.0.0",
            "description" : "A host device root file system."
        },
        {
            "id" : "2",
            "name" : "host-bootfs",
            "group" : "system",
            "manufacture" : "contoso",
            "model" : "smart-vacuum",
            "version" : "1.0.0",
            "description" : "A host device boot file system."
        },
        {
            "id" : "serial#ABCDE000001",
            "name" : "front-usb-camera",
            "group" : "usb-camera",
            "manufacture" : "contoso",
            "model" : "usb-cam-0001",
            "version" : "1.0.0",
            "description" : "Front camera."
        },
        {
            "id" : "serial#ABCDE000002",
            "name" : "rear-usb-camera",
            "group" : "usb-camera",
            "manufacture" : "contoso",
            "model" : "usb-cam-0001",
            "version" : "1.0.0",
            "description" : "Rear camera."
        },
        {
            "id" : "serial#WXYZ000010",
            "name" : "wheels-motor-controller",
            "group" : "usb-motor-controller",
            "manufacture" : "contoso",
            "model" : "usb-mc-0001",
            "version" : "1.0.0",
            "description" : "Primary drive motor controller."
        },
        {
            "id" : "serial#WXYZ000020",
            "name" : "vacuum-motor-controller",
            "group" : "usb-motor-controller",
            "manufacture" : "contoso",
            "model" : "usb-mc-0001",
            "version" : "1.0.0",
            "description" : "Primary vacuum motor controller."
        }
    ]
}
```
