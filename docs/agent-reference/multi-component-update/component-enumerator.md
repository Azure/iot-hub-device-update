# Device Component Enumerator 

The Device Component Enumerator is a linux shared library that provide APIs that can be used for querying components on the device, and their properties.  

The enumerator plug-in must implement following APIs:
```

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

### Example Output
```
```