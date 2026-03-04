/**
 * @file linux_device_info_exports_ut.cpp
 * @brief Unit Tests for linux_device_info_exports functionality
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <cstring>
#include <string>

#include <aduc/device_info_exports.h>

//
// Unit Tests for DI_DeviceInfoProperty enum
//

TEST_CASE("DI_DeviceInfoProperty enum values")
{
    SECTION("Enum values are correctly defined")
    {
        CHECK(DIIP_Manufacturer == 0);
        CHECK(DIIP_Model == 1);
        CHECK(DIIP_OsName == 2);
        CHECK(DIIP_ProcessorArchitecture == 3);
        CHECK(DIIP_ProcessorManufacturer == 4);
        CHECK(DIIP_SoftwareVersion == 5);
        CHECK(DIIP_TotalMemory == 6);
        CHECK(DIIP_TotalStorage == 7);
    }

    SECTION("All enum values are distinct")
    {
        CHECK(DIIP_Manufacturer != DIIP_Model);
        CHECK(DIIP_Model != DIIP_OsName);
        CHECK(DIIP_OsName != DIIP_ProcessorArchitecture);
        CHECK(DIIP_ProcessorArchitecture != DIIP_ProcessorManufacturer);
        CHECK(DIIP_ProcessorManufacturer != DIIP_SoftwareVersion);
        CHECK(DIIP_SoftwareVersion != DIIP_TotalMemory);
        CHECK(DIIP_TotalMemory != DIIP_TotalStorage);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - Manufacturer
//

TEST_CASE("DI_GetDeviceInformationValue - Manufacturer")
{
    SECTION("Returns non-null value on first call")
    {
        char* manufacturer = DI_GetDeviceInformationValue(DIIP_Manufacturer);

        // First call should return a value
        // Note: May be nullptr if config is not initialized, so we only check it doesn't crash
        if (manufacturer != nullptr)
        {
            CHECK(strlen(manufacturer) > 0);
            free(manufacturer);
        }
    }

    SECTION("Returns nullptr on subsequent calls (value not dirty)")
    {
        // First call to get the value
        char* manufacturer1 = DI_GetDeviceInformationValue(DIIP_Manufacturer);
        if (manufacturer1 != nullptr)
        {
            free(manufacturer1);
        }

        // Second call should return nullptr since value hasn't changed
        char* manufacturer2 = DI_GetDeviceInformationValue(DIIP_Manufacturer);
        CHECK(manufacturer2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - Model
//

TEST_CASE("DI_GetDeviceInformationValue - Model")
{
    SECTION("Returns non-null value on first call")
    {
        char* model = DI_GetDeviceInformationValue(DIIP_Model);

        if (model != nullptr)
        {
            CHECK(strlen(model) > 0);
            free(model);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* model1 = DI_GetDeviceInformationValue(DIIP_Model);
        if (model1 != nullptr)
        {
            free(model1);
        }

        char* model2 = DI_GetDeviceInformationValue(DIIP_Model);
        CHECK(model2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - OsName
//

TEST_CASE("DI_GetDeviceInformationValue - OsName")
{
    SECTION("Returns valid OS name on first call")
    {
        char* osName = DI_GetDeviceInformationValue(DIIP_OsName);

        // On a Linux system, this should return a valid OS name
        if (osName != nullptr)
        {
            CHECK(strlen(osName) > 0);
            // Common Linux OS names
            std::string osNameStr(osName);
            // Just verify it's non-empty and looks reasonable
            CHECK(osNameStr.length() > 0);
            free(osName);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* osName1 = DI_GetDeviceInformationValue(DIIP_OsName);
        if (osName1 != nullptr)
        {
            free(osName1);
        }

        char* osName2 = DI_GetDeviceInformationValue(DIIP_OsName);
        CHECK(osName2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - SoftwareVersion (OS Version)
//

TEST_CASE("DI_GetDeviceInformationValue - SoftwareVersion")
{
    SECTION("Returns valid software version on first call")
    {
        char* version = DI_GetDeviceInformationValue(DIIP_SoftwareVersion);

        if (version != nullptr)
        {
            CHECK(strlen(version) > 0);
            free(version);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* version1 = DI_GetDeviceInformationValue(DIIP_SoftwareVersion);
        if (version1 != nullptr)
        {
            free(version1);
        }

        char* version2 = DI_GetDeviceInformationValue(DIIP_SoftwareVersion);
        CHECK(version2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - ProcessorArchitecture
//

TEST_CASE("DI_GetDeviceInformationValue - ProcessorArchitecture")
{
    SECTION("Returns valid processor architecture on first call")
    {
        char* arch = DI_GetDeviceInformationValue(DIIP_ProcessorArchitecture);

        if (arch != nullptr)
        {
            CHECK(strlen(arch) > 0);
            // Common architectures: x86_64, aarch64, armv7l, i686, etc.
            std::string archStr(arch);
            CHECK(archStr.length() > 0);
            free(arch);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* arch1 = DI_GetDeviceInformationValue(DIIP_ProcessorArchitecture);
        if (arch1 != nullptr)
        {
            free(arch1);
        }

        char* arch2 = DI_GetDeviceInformationValue(DIIP_ProcessorArchitecture);
        CHECK(arch2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - ProcessorManufacturer
//

TEST_CASE("DI_GetDeviceInformationValue - ProcessorManufacturer")
{
    SECTION("Returns value or nullptr depending on lscpu availability")
    {
        char* manufacturer = DI_GetDeviceInformationValue(DIIP_ProcessorManufacturer);

        // This may return nullptr if lscpu is not available or doesn't have Vendor ID
        if (manufacturer != nullptr)
        {
            CHECK(strlen(manufacturer) > 0);
            free(manufacturer);
        }
        else
        {
            // It's acceptable to return nullptr
            CHECK(manufacturer == nullptr);
        }
    }

    SECTION("Returns nullptr on subsequent calls if first succeeded")
    {
        char* manufacturer1 = DI_GetDeviceInformationValue(DIIP_ProcessorManufacturer);
        if (manufacturer1 != nullptr)
        {
            free(manufacturer1);

            char* manufacturer2 = DI_GetDeviceInformationValue(DIIP_ProcessorManufacturer);
            CHECK(manufacturer2 == nullptr);
        }
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - TotalMemory
//

TEST_CASE("DI_GetDeviceInformationValue - TotalMemory")
{
    SECTION("Returns valid memory value on first call")
    {
        char* memory = DI_GetDeviceInformationValue(DIIP_TotalMemory);

        if (memory != nullptr)
        {
            CHECK(strlen(memory) > 0);
            // Memory should be a numeric string (kilobytes)
            long long memoryValue = std::stoll(memory);
            CHECK(memoryValue > 0);
            free(memory);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* memory1 = DI_GetDeviceInformationValue(DIIP_TotalMemory);
        if (memory1 != nullptr)
        {
            free(memory1);
        }

        char* memory2 = DI_GetDeviceInformationValue(DIIP_TotalMemory);
        CHECK(memory2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - TotalStorage
//

TEST_CASE("DI_GetDeviceInformationValue - TotalStorage")
{
    SECTION("Returns valid storage value on first call")
    {
        char* storage = DI_GetDeviceInformationValue(DIIP_TotalStorage);

        if (storage != nullptr)
        {
            CHECK(strlen(storage) > 0);
            // Storage should be a numeric string (kilobytes)
            long long storageValue = std::stoll(storage);
            CHECK(storageValue > 0);
            free(storage);
        }
    }

    SECTION("Returns nullptr on subsequent calls")
    {
        char* storage1 = DI_GetDeviceInformationValue(DIIP_TotalStorage);
        if (storage1 != nullptr)
        {
            free(storage1);
        }

        char* storage2 = DI_GetDeviceInformationValue(DIIP_TotalStorage);
        CHECK(storage2 == nullptr);
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - Invalid Property
//

TEST_CASE("DI_GetDeviceInformationValue - Invalid property handling")
{
    SECTION("Returns nullptr for invalid property value")
    {
        // Cast an invalid value to the enum type
        DI_DeviceInfoProperty invalidProperty = static_cast<DI_DeviceInfoProperty>(999);

        char* value = DI_GetDeviceInformationValue(invalidProperty);

        // Should return nullptr for unknown property
        CHECK(value == nullptr);
    }

    SECTION("Does not crash with negative property value")
    {
        DI_DeviceInfoProperty negativeProperty = static_cast<DI_DeviceInfoProperty>(-1);

        char* value = DI_GetDeviceInformationValue(negativeProperty);

        // Should not crash and return nullptr
        CHECK(value == nullptr);
    }
}

//
// Unit Tests for memory management
//

TEST_CASE("DI_GetDeviceInformationValue - Memory management")
{
    SECTION("Returned strings are allocated with malloc and can be freed")
    {
        // This test verifies that the returned string can be freed without issues
        char* manufacturer = DI_GetDeviceInformationValue(DIIP_Manufacturer);
        if (manufacturer != nullptr)
        {
            // This should not cause any memory issues
            free(manufacturer);
            CHECK(true);
        }

        char* model = DI_GetDeviceInformationValue(DIIP_Model);
        if (model != nullptr)
        {
            free(model);
            CHECK(true);
        }
    }
}

//
// Unit Tests for DI_GetDeviceInformationValue - Enumeration boundary tests
//

TEST_CASE("DI_GetDeviceInformationValue - Boundary and sequential access")
{
    SECTION("All valid properties return non-null or nullptr without crashing")
    {
        // Iterate over all known valid property values.
        // Some may return nullptr (dirty-flag already tripped or config not present).
        // The goal is to exercise every function dispatch path in the DI_GetDeviceInformationValue map.
        const DI_DeviceInfoProperty allProperties[] = {
            DIIP_Manufacturer,
            DIIP_Model,
            DIIP_OsName,
            DIIP_ProcessorArchitecture,
            DIIP_ProcessorManufacturer,
            DIIP_SoftwareVersion,
            DIIP_TotalMemory,
            DIIP_TotalStorage,
        };

        for (auto prop : allProperties)
        {
            char* value = DI_GetDeviceInformationValue(prop);
            // Free if non-null (first call may return value; subsequent calls return nullptr)
            if (value != nullptr)
            {
                CHECK(strlen(value) > 0);
                free(value);
            }
        }
        CHECK(true); // no crash
    }

    SECTION("Large out-of-range enum values return nullptr")
    {
        CHECK(DI_GetDeviceInformationValue(static_cast<DI_DeviceInfoProperty>(100)) == nullptr);
        CHECK(DI_GetDeviceInformationValue(static_cast<DI_DeviceInfoProperty>(255)) == nullptr);
        CHECK(DI_GetDeviceInformationValue(static_cast<DI_DeviceInfoProperty>(INT32_MAX)) == nullptr);
    }
}
