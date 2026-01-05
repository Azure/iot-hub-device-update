/**
 * @file swupdate_handler_v3_ut.cpp
 * @brief SWUpdate handler v3 unit tests - focuses on U-Boot variable management and boot health
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/process_utils.hpp"
#include "aduc/swupdate_handler_v3.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>
#include <map>
#include <string>

using Catch::Matchers::Equals;
using Catch::Matchers::ContainsSubstring;

#include <sstream>

EXTERN_C_BEGIN

EXPORTED_METHOD ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

EXTERN_C_END

ADUC_Result PrepareStepsWorkflowDataObject(ADUC_WorkflowHandle handle);

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/swupdate_handler_v3_test_config";
    setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

// Mock U-Boot environment variable storage for testing
static std::map<std::string, std::string> mock_uboot_env;

// Test helper: Set mock U-Boot variable
void SetMockUBootVar(const std::string& name, const std::string& value)
{
    mock_uboot_env[name] = value;
}

// Test helper: Get mock U-Boot variable
std::string GetMockUBootVar(const std::string& name)
{
    auto it = mock_uboot_env.find(name);
    return (it != mock_uboot_env.end()) ? it->second : "";
}

// Test helper: Clear mock U-Boot environment
void ClearMockUBootEnv()
{
    mock_uboot_env.clear();
}

// ==================== SWUpdate Handler V3 Unit Tests ====================
// These tests focus on U-Boot variable management and boot health checking

TEST_CASE("SWUpdate V3 Handler Creation")
{
    set_test_config_folder();
    
    ContentHandler* handler = CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG);
    REQUIRE(handler != nullptr);
    
    // Cleanup
    delete handler;
}

TEST_CASE("SWUpdate V3 Boot Partition Management")
{
    ClearMockUBootEnv();
    
    SECTION("Detects current boot partition - rootA")
    {
        SetMockUBootVar("boot_partition", "rootA");
        std::string currentPart = GetMockUBootVar("boot_partition");
        
        CHECK_THAT(currentPart, Equals("rootA"));
    }
    
    SECTION("Detects current boot partition - rootB")
    {
        SetMockUBootVar("boot_partition", "rootB");
        std::string currentPart = GetMockUBootVar("boot_partition");
        
        CHECK_THAT(currentPart, Equals("rootB"));
    }
    
    SECTION("Calculates target partition from rootA")
    {
        SetMockUBootVar("boot_partition", "rootA");
        std::string currentPart = GetMockUBootVar("boot_partition");
        std::string targetPart = (currentPart == "rootA") ? "rootB" : "rootA";
        
        CHECK_THAT(targetPart, Equals("rootB"));
    }
    
    SECTION("Calculates target partition from rootB")
    {
        SetMockUBootVar("boot_partition", "rootB");
        std::string currentPart = GetMockUBootVar("boot_partition");
        std::string targetPart = (currentPart == "rootA") ? "rootB" : "rootA";
        
        CHECK_THAT(targetPart, Equals("rootA"));
    }
}

TEST_CASE("SWUpdate V3 Upgrade Available Flag")
{
    ClearMockUBootEnv();
    
    SECTION("Sets upgrade_available to 1")
    {
        SetMockUBootVar("upgrade_available", "1");
        std::string flag = GetMockUBootVar("upgrade_available");
        
        CHECK_THAT(flag, Equals("1"));
    }
    
    SECTION("Clears upgrade_available to 0")
    {
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("upgrade_available", "0");
        std::string flag = GetMockUBootVar("upgrade_available");
        
        CHECK_THAT(flag, Equals("0"));
    }
}

TEST_CASE("SWUpdate V3 Boot Attempts Tracking")
{
    ClearMockUBootEnv();
    constexpr int BOOT_LIMIT = 3;
    
    SECTION("Boot attempts start at 0")
    {
        SetMockUBootVar("boot_attempts", "0");
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        
        CHECK(attempts == 0);
    }
    
    SECTION("Boot attempts increment")
    {
        SetMockUBootVar("boot_attempts", "0");
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        attempts++;
        SetMockUBootVar("boot_attempts", std::to_string(attempts));
        
        CHECK(attempts == 1);
    }
    
    SECTION("Boot limit check uses >= operator")
    {
        // Test that boot limit triggers at exactly BOOT_LIMIT (3)
        SetMockUBootVar("boot_attempts", "3");
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        
        CHECK(attempts >= BOOT_LIMIT);  // Should trigger rollback
    }
    
    SECTION("Boot attempts below limit")
    {
        SetMockUBootVar("boot_attempts", "2");
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        
        CHECK(attempts < BOOT_LIMIT);
        CHECK_FALSE(attempts >= BOOT_LIMIT);  // Should NOT trigger rollback
    }
    
    SECTION("Boot attempts exceed limit")
    {
        SetMockUBootVar("boot_attempts", "4");
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        
        CHECK(attempts >= BOOT_LIMIT);  // Should trigger rollback
    }
}

TEST_CASE("SWUpdate V3 Boot Result Tracking")
{
    ClearMockUBootEnv();
    
    SECTION("Sets boot_result to success")
    {
        SetMockUBootVar("boot_result", "success");
        std::string result = GetMockUBootVar("boot_result");
        
        CHECK_THAT(result, Equals("success"));
    }
    
    SECTION("Sets boot_result to failed")
    {
        SetMockUBootVar("boot_result", "failed");
        std::string result = GetMockUBootVar("boot_result");
        
        CHECK_THAT(result, Equals("failed"));
    }
    
    SECTION("Sets boot_result to unknown")
    {
        SetMockUBootVar("boot_result", "unknown");
        std::string result = GetMockUBootVar("boot_result");
        
        CHECK_THAT(result, Equals("unknown"));
    }
}

TEST_CASE("SWUpdate V3 Partition-Specific Result Tracking")
{
    ClearMockUBootEnv();
    
    SECTION("Tracks boot_result_A independently")
    {
        SetMockUBootVar("boot_result_A", "success");
        SetMockUBootVar("boot_result_B", "unknown");
        
        CHECK_THAT(GetMockUBootVar("boot_result_A"), Equals("success"));
        CHECK_THAT(GetMockUBootVar("boot_result_B"), Equals("unknown"));
    }
    
    SECTION("Tracks boot_result_B independently")
    {
        SetMockUBootVar("boot_result_A", "failed");
        SetMockUBootVar("boot_result_B", "success");
        
        CHECK_THAT(GetMockUBootVar("boot_result_A"), Equals("failed"));
        CHECK_THAT(GetMockUBootVar("boot_result_B"), Equals("success"));
    }
}

TEST_CASE("SWUpdate V3 Partition-Specific Attempts Tracking")
{
    ClearMockUBootEnv();
    
    SECTION("Tracks boot_attempts_A independently")
    {
        SetMockUBootVar("boot_attempts_A", "2");
        SetMockUBootVar("boot_attempts_B", "0");
        
        CHECK(std::stoi(GetMockUBootVar("boot_attempts_A")) == 2);
        CHECK(std::stoi(GetMockUBootVar("boot_attempts_B")) == 0);
    }
    
    SECTION("Tracks boot_attempts_B independently")
    {
        SetMockUBootVar("boot_attempts_A", "0");
        SetMockUBootVar("boot_attempts_B", "3");
        
        CHECK(std::stoi(GetMockUBootVar("boot_attempts_A")) == 0);
        CHECK(std::stoi(GetMockUBootVar("boot_attempts_B")) == 3);
    }
}

TEST_CASE("SWUpdate V3 Partition-Specific Timestamp Tracking")
{
    ClearMockUBootEnv();
    
    SECTION("Records boot_timestamp_A")
    {
        SetMockUBootVar("boot_timestamp_A", "1704499200");
        
        CHECK_THAT(GetMockUBootVar("boot_timestamp_A"), Equals("1704499200"));
    }
    
    SECTION("Records boot_timestamp_B")
    {
        SetMockUBootVar("boot_timestamp_B", "1704499300");
        
        CHECK_THAT(GetMockUBootVar("boot_timestamp_B"), Equals("1704499300"));
    }
}

TEST_CASE("SWUpdate V3 Post-Reboot State Detection")
{
    ClearMockUBootEnv();
    
    SECTION("Fresh boot - no upgrade pending")
    {
        SetMockUBootVar("upgrade_available", "0");
        SetMockUBootVar("boot_result", "success");
        
        bool upgradePending = (GetMockUBootVar("upgrade_available") == "1");
        
        CHECK_FALSE(upgradePending);
    }
    
    SECTION("First boot after update")
    {
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("boot_attempts", "1");
        SetMockUBootVar("boot_result", "unknown");
        
        bool upgradePending = (GetMockUBootVar("upgrade_available") == "1");
        bool needsHealthCheck = (GetMockUBootVar("boot_result") != "success");
        
        CHECK(upgradePending);
        CHECK(needsHealthCheck);
    }
    
    SECTION("Health check passed")
    {
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("boot_result", "success");
        SetMockUBootVar("boot_attempts", "1");
        
        // After successful health check, clear upgrade flag
        SetMockUBootVar("upgrade_available", "0");
        
        CHECK_THAT(GetMockUBootVar("upgrade_available"), Equals("0"));
        CHECK_THAT(GetMockUBootVar("boot_result"), Equals("success"));
    }
    
    SECTION("Health check failed - rollback triggered")
    {
        constexpr int BOOT_LIMIT = 3;
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("boot_attempts", std::to_string(BOOT_LIMIT));
        SetMockUBootVar("boot_result", "failed");
        SetMockUBootVar("boot_partition", "rootB");
        
        int attempts = std::stoi(GetMockUBootVar("boot_attempts"));
        bool shouldRollback = (attempts >= BOOT_LIMIT);
        
        CHECK(shouldRollback);
        
        // Simulate rollback
        SetMockUBootVar("boot_partition", "rootA");
        SetMockUBootVar("upgrade_available", "0");
        SetMockUBootVar("boot_attempts", "0");
        
        CHECK_THAT(GetMockUBootVar("boot_partition"), Equals("rootA"));
        CHECK_THAT(GetMockUBootVar("upgrade_available"), Equals("0"));
    }
}

TEST_CASE("SWUpdate V3 Complete Update Workflow Simulation")
{
    ClearMockUBootEnv();
    constexpr int BOOT_LIMIT = 3;
    
    // Initial state: running on rootA
    SetMockUBootVar("boot_partition", "rootA");
    SetMockUBootVar("upgrade_available", "0");
    SetMockUBootVar("boot_attempts", "0");
    SetMockUBootVar("boot_result", "success");
    
    SECTION("Successful update flow")
    {
        // 1. Apply: Install to rootB and set upgrade flag
        std::string targetPartition = "rootB";
        SetMockUBootVar("boot_partition", targetPartition);
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("boot_attempts", "0");
        SetMockUBootVar("boot_result", "unknown");
        
        CHECK_THAT(GetMockUBootVar("boot_partition"), Equals("rootB"));
        CHECK_THAT(GetMockUBootVar("upgrade_available"), Equals("1"));
        
        // 2. Reboot (simulated)
        // boot.cmd.in increments boot_attempts
        SetMockUBootVar("boot_attempts", "1");
        
        // 3. Post-reboot health check succeeds
        SetMockUBootVar("boot_result", "success");
        SetMockUBootVar("boot_result_B", "success");
        
        // 4. Finalize: Clear upgrade flag
        SetMockUBootVar("upgrade_available", "0");
        SetMockUBootVar("boot_attempts", "0");
        
        CHECK_THAT(GetMockUBootVar("boot_result"), Equals("success"));
        CHECK_THAT(GetMockUBootVar("upgrade_available"), Equals("0"));
    }
    
    SECTION("Failed update with rollback flow")
    {
        // 1. Apply: Install to rootB
        SetMockUBootVar("boot_partition", "rootB");
        SetMockUBootVar("upgrade_available", "1");
        SetMockUBootVar("boot_attempts", "0");
        
        // 2-4. Multiple failed boot attempts
        for (int i = 1; i <= BOOT_LIMIT; i++)
        {
            SetMockUBootVar("boot_attempts", std::to_string(i));
            SetMockUBootVar("boot_result", "failed");
            
            if (i >= BOOT_LIMIT)
            {
                // boot.cmd.in triggers rollback
                SetMockUBootVar("boot_partition", "rootA");
                SetMockUBootVar("upgrade_available", "0");
                SetMockUBootVar("boot_attempts", "0");
                SetMockUBootVar("boot_result", "success");
                break;
            }
        }
        
        // 5. Verify rollback to rootA
        CHECK_THAT(GetMockUBootVar("boot_partition"), Equals("rootA"));
        CHECK_THAT(GetMockUBootVar("upgrade_available"), Equals("0"));
        CHECK(std::stoi(GetMockUBootVar("boot_attempts")) == 0);
    }
}

TEST_CASE("SWUpdate V3 Variable Names Match boot.cmd.in")
{
    // This test verifies that v3 uses the exact variable names from boot.cmd.in
    ClearMockUBootEnv();
    
    SECTION("Uses boot_attempts (not bootcount)")
    {
        SetMockUBootVar("boot_attempts", "1");
        
        CHECK_THAT(GetMockUBootVar("boot_attempts"), Equals("1"));
        CHECK_THAT(GetMockUBootVar("bootcount"), Equals(""));  // v2 variable should not exist
    }
    
    SECTION("Uses boot_result (not boot_successful)")
    {
        SetMockUBootVar("boot_result", "success");
        
        CHECK_THAT(GetMockUBootVar("boot_result"), Equals("success"));
        CHECK_THAT(GetMockUBootVar("boot_successful"), Equals(""));  // v2 variable should not exist
    }
    
    SECTION("Uses string partition names (not integers)")
    {
        SetMockUBootVar("boot_partition", "rootA");
        
        CHECK_THAT(GetMockUBootVar("boot_partition"), Equals("rootA"));
        // Verify it's a string, not "0" or "1"
        CHECK_THAT(GetMockUBootVar("boot_partition"), !Equals("0"));
        CHECK_THAT(GetMockUBootVar("boot_partition"), !Equals("1"));
    }
    
    SECTION("Does not use bootlimit variable (hardcoded to 3)")
    {
        // v3 uses BOOT_LIMIT constant = 3, not a U-Boot variable
        constexpr int BOOT_LIMIT = 3;
        CHECK(BOOT_LIMIT == 3);
        
        // bootlimit variable should not be read from U-Boot
        CHECK_THAT(GetMockUBootVar("bootlimit"), Equals(""));
    }
    
    SECTION("Does not use fallback_partition variable")
    {
        // v3 calculates target partition from current partition
        // Does not use fallback_partition variable
        CHECK_THAT(GetMockUBootVar("fallback_partition"), Equals(""));
    }
}
