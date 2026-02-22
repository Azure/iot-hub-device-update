/**
 * @file auto_dir_ut.cpp
 * @brief Unit Tests for AutoDir (test_utils library)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <aduc/auto_dir.hpp>
#include <aduc/system_utils.h>
#include <fstream>
#include <string>

TEST_CASE("AutoDir - Constructor and GetDir")
{
    const std::string testPath = "/tmp/adu_autodir_test_getdir";
    
    SECTION("Constructor initializes directory path")
    {
        aduc::AutoDir autoDir(testPath.c_str());
        CHECK(autoDir.GetDir() == testPath);
    }
}

TEST_CASE("AutoDir - CreateDir")
{
    const std::string testPath = "/tmp/adu_autodir_test_create";
    
    // Clean up if exists
    ADUC_SystemUtils_RmDirRecursive(testPath.c_str());
    
    SECTION("CreateDir creates new directory")
    {
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Directory should not exist initially
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        // Create the directory
        bool result = autoDir.CreateDir();
        CHECK(result);
        
        // Verify directory was created
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
    }
    
    SECTION("CreateDir returns false if directory already exists")
    {
        // First create the directory
        ADUC_SystemUtils_MkDirRecursiveDefault(testPath.c_str());
        
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Try to create again - should return false
        bool result = autoDir.CreateDir();
        CHECK_FALSE(result);
        
        // Directory should still exist
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
    }
    
    // Cleanup
    ADUC_SystemUtils_RmDirRecursive(testPath.c_str());
}

TEST_CASE("AutoDir - RemoveDir")
{
    const std::string testPath = "/tmp/adu_autodir_test_remove";
    
    SECTION("RemoveDir removes existing directory")
    {
        // Create the directory first
        ADUC_SystemUtils_MkDirRecursiveDefault(testPath.c_str());
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Remove the directory
        bool result = autoDir.RemoveDir();
        CHECK(result);
        
        // Verify directory was removed
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
    }
    
    SECTION("RemoveDir returns true if directory doesn't exist")
    {
        // Ensure directory doesn't exist
        ADUC_SystemUtils_RmDirRecursive(testPath.c_str());
        
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Try to remove non-existent directory - should return true
        bool result = autoDir.RemoveDir();
        CHECK(result);
    }
    
    SECTION("RemoveDir removes directory with files")
    {
        // Create directory with files
        ADUC_SystemUtils_MkDirRecursiveDefault(testPath.c_str());
        std::string filePath = testPath + "/test_file.txt";
        std::ofstream file(filePath);
        file << "test content";
        file.close();
        
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Remove directory with content
        bool result = autoDir.RemoveDir();
        CHECK(result);
        
        // Verify directory was removed
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
    }
}

TEST_CASE("AutoDir - Destructor auto-cleanup")
{
    const std::string testPath = "/tmp/adu_autodir_test_destructor";
    
    SECTION("Destructor removes directory when object goes out of scope")
    {
        // Create the directory
        ADUC_SystemUtils_MkDirRecursiveDefault(testPath.c_str());
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        {
            aduc::AutoDir autoDir(testPath.c_str());
            // Directory exists within scope
            CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
        } // autoDir destructor called here
        
        // Directory should be removed after scope exit
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
    }
    
    SECTION("Destructor handles non-existent directory gracefully")
    {
        // Ensure directory doesn't exist
        ADUC_SystemUtils_RmDirRecursive(testPath.c_str());
        
        {
            aduc::AutoDir autoDir(testPath.c_str());
            // Directory doesn't exist
            CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
        } // autoDir destructor called - should not crash
        
        // No exception should be thrown
        CHECK(true);
    }
}

TEST_CASE("AutoDir - Combined operations")
{
    const std::string testPath = "/tmp/adu_autodir_test_combined";
    
    SECTION("Create, verify, remove cycle")
    {
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Initially doesn't exist
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        // Create directory
        CHECK(autoDir.CreateDir());
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        // Try to create again (should fail)
        CHECK_FALSE(autoDir.CreateDir());
        
        // Remove directory
        CHECK(autoDir.RemoveDir());
        CHECK_FALSE(SystemUtils_IsDir(testPath.c_str(), nullptr));
        
        // Remove again (should still return true)
        CHECK(autoDir.RemoveDir());
    }
}

TEST_CASE("AutoDir - Nested directory paths")
{
    const std::string testPath = "/tmp/adu_autodir_test/nested/deep/path";
    
    SECTION("Creates nested directory structure")
    {
        // Clean up parent
        ADUC_SystemUtils_RmDirRecursive("/tmp/adu_autodir_test");
        
        aduc::AutoDir autoDir(testPath.c_str());
        
        // Create nested directories
        bool result = autoDir.CreateDir();
        CHECK(result);
        
        // Verify all levels exist
        CHECK(SystemUtils_IsDir(testPath.c_str(), nullptr));
        CHECK(SystemUtils_IsDir("/tmp/adu_autodir_test/nested/deep", nullptr));
        CHECK(SystemUtils_IsDir("/tmp/adu_autodir_test/nested", nullptr));
    }
    
    // Cleanup parent directory
    ADUC_SystemUtils_RmDirRecursive("/tmp/adu_autodir_test");
}