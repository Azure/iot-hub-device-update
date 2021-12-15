/**
 * @file installed_criteria_utils_ut.cpp
 * @brief installed criteria utilities unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_core_exports.h"
#include "aduc/installed_criteria_utils.hpp"
#include <catch2/catch.hpp>

class InstalledCriteriaPersistence  // NOLINT
{
public:
    ~InstalledCriteriaPersistence()
    {
        remove(ADUC_INSTALLEDCRITERIA_FILE_PATH);
    }
};


TEST_CASE("IsInstalled_test")
{
    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();
    // Persist foo
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    ADUC_Result isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    bool persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Persist bar
    const char* installedCriteria_bar = "bar.1.0.1";
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(persisted);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Remove foo
    bool removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    // Remove bar
    removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(removed);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);
}

TEST_CASE("RemoveIsInstalledWhenEmpty")
{
    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();

    // Remove should succeeded despite no elements in json array.
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    bool removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);
}

TEST_CASE("RemoveIsInstalledWhenNoMatch")
{
    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();

    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    bool persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    // Remove should succeed when bar does not exist.
    const char* installedCriteria_bar = "bar.1.0.1";
    bool removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(removed);
}

TEST_CASE("RemoveIsInstalledTwice_test")
{
    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();

    // Ensure foo doesn't exist.
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    ADUC_Result isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    // Persist foo.
    bool persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    // Foo is installed.
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Remove foo should succeeded.
    bool removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);

    // Regression test: 2nd remove should succeeded, with no infinite loop.
    removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);
}

TEST_CASE("RemoveInstalledCriteriaShouldRemoveDuplicates")
{
    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();

    const char* installedCriteria_contoso = "contoso-iot-edge-6.1.0.19";

    ADUC_Result isInstalled =
        GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    // Persist
    bool persisted =
        PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(persisted);

    // Should be installed
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Persist duplicate
    persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(persisted);

    // Should still be installed
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Single remove call should remove all matching
    bool removed =
        RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(removed);

    // Should NOT be installed
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_contoso);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
}

TEST_CASE("RemoveInstalledCriteriaShouldRemoveDuplicatesAndSkipNonMatching")
{
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    const char* installedCriteria_bar = "bar.1.0.1";

    InstalledCriteriaPersistence persistence; // remove installed criteria file on destruction.
    UNREFERENCED_PARAMETER(persistence); // avoid style warning for unused variable.

    RemoveAllInstalledCriteria();

    // Persist foo
    bool persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    ADUC_Result isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Persist dup foo
    persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Persist bar
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode != ADUC_Result_IsInstalled_Installed);

    persisted = PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(persisted);

    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);

    // Single remove foo and duplicates
    bool removed = RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);

    // foo should NOT be installed
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_NotInstalled);

    // but bar should be installed
    isInstalled = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode == ADUC_Result_IsInstalled_Installed);
}
