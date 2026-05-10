/**
 * @file component_registry_ut.cpp
 * @brief Unit tests for component registry and targeting.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_test_macros.hpp>

#include <cstring>

extern "C"
{
#include "aduc/component_enumerator_vtable.h"
#include "aduc/component_registry.h"
#include "aduc/component_targeting.h"
}

// --- Mock enumerator ---

static ADUC_ComponentInfo s_mockComponents[2];
static bool s_mockInitialized = false;

static void InitMockComponents()
{
    if (!s_mockInitialized)
    {
        memset(s_mockComponents, 0, sizeof(s_mockComponents));

        strncpy(s_mockComponents[0].id, "fw-main", sizeof(s_mockComponents[0].id) - 1);
        strncpy(s_mockComponents[0].name, "Main Firmware", sizeof(s_mockComponents[0].name) - 1);
        strncpy(s_mockComponents[0].group, "firmware", sizeof(s_mockComponents[0].group) - 1);
        strncpy(s_mockComponents[0].manufacturer, "Contoso", sizeof(s_mockComponents[0].manufacturer) - 1);
        strncpy(s_mockComponents[0].model, "Widget-1000", sizeof(s_mockComponents[0].model) - 1);
        strncpy(s_mockComponents[0].version, "1.2.3", sizeof(s_mockComponents[0].version) - 1);

        strncpy(s_mockComponents[1].id, "app-pkg", sizeof(s_mockComponents[1].id) - 1);
        strncpy(s_mockComponents[1].name, "Application Package", sizeof(s_mockComponents[1].name) - 1);
        strncpy(s_mockComponents[1].group, "software", sizeof(s_mockComponents[1].group) - 1);
        strncpy(s_mockComponents[1].manufacturer, "Contoso", sizeof(s_mockComponents[1].manufacturer) - 1);
        strncpy(s_mockComponents[1].model, "Widget-1000", sizeof(s_mockComponents[1].model) - 1);
        strncpy(s_mockComponents[1].version, "3.4.5", sizeof(s_mockComponents[1].version) - 1);

        s_mockInitialized = true;
    }
}

static ADUC_Result2 MockEnumerate(ADUC_ComponentInfo** outComponents, size_t* outCount)
{
    InitMockComponents();

    auto* copy = static_cast<ADUC_ComponentInfo*>(malloc(sizeof(s_mockComponents)));
    memcpy(copy, s_mockComponents, sizeof(s_mockComponents));

    *outComponents = copy;
    *outCount = 2;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 MockGetById(const char* componentId, ADUC_ComponentInfo* outComponent)
{
    InitMockComponents();
    for (int i = 0; i < 2; i++)
    {
        if (strcmp(s_mockComponents[i].id, componentId) == 0)
        {
            *outComponent = s_mockComponents[i];
            return ADUC_RESULT2_SUCCESS;
        }
    }
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 1);
}

static bool MockMatchesCriteria(const ADUC_ComponentInfo* component, const char* criteria)
{
    return (strcmp(component->group, criteria) == 0);
}

static ADUC_Result2 MockGetInstalledVersion(const char* componentId, char* versionBuf, size_t bufLen)
{
    (void)componentId;
    (void)versionBuf;
    (void)bufLen;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 MockRefresh(void)
{
    return ADUC_RESULT2_SUCCESS;
}

static void MockFreeComponents(ADUC_ComponentInfo* components, size_t count)
{
    (void)count;
    free(components);
}

static const ADUC_ComponentEnumeratorVtable s_mockVtable = {
    MockEnumerate,
    MockGetById,
    MockMatchesCriteria,
    MockGetInstalledVersion,
    MockRefresh,
    MockFreeComponents,
};

// --- Tests ---

TEST_CASE("ComponentRegistry: empty registry has count 0", "[component_registry]")
{
    ADUC_ComponentRegistryHandle handle = nullptr;
    ADUC_Result2 res = ADUC_ComponentRegistry_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    REQUIRE(handle != nullptr);

    CHECK(ADUC_ComponentRegistry_GetCount(handle) == 0);

    ADUC_ComponentRegistry_Destroy(handle);
}

TEST_CASE("ComponentRegistry: add enumerator and refresh", "[component_registry]")
{
    ADUC_ComponentRegistryHandle handle = nullptr;
    ADUC_Result2 res = ADUC_ComponentRegistry_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));

    res = ADUC_ComponentRegistry_AddEnumerator(handle, &s_mockVtable);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));

    res = ADUC_ComponentRegistry_Refresh(handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));

    CHECK(ADUC_ComponentRegistry_GetCount(handle) == 2);

    ADUC_ComponentInfo* components = nullptr;
    size_t count = 0;
    res = ADUC_ComponentRegistry_GetAll(handle, &components, &count);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    CHECK(count == 2);
    CHECK(strcmp(components[0].id, "fw-main") == 0);
    CHECK(strcmp(components[1].id, "app-pkg") == 0);

    ADUC_ComponentRegistry_FreeComponents(components, count);
    ADUC_ComponentRegistry_Destroy(handle);
}

TEST_CASE("ComponentRegistry: FindByGroup returns correct subset", "[component_registry]")
{
    ADUC_ComponentRegistryHandle handle = nullptr;
    ADUC_ComponentRegistry_Create(&handle);
    ADUC_ComponentRegistry_AddEnumerator(handle, &s_mockVtable);
    ADUC_ComponentRegistry_Refresh(handle);

    ADUC_ComponentInfo* components = nullptr;
    size_t count = 0;

    ADUC_Result2 res = ADUC_ComponentRegistry_FindByGroup(handle, "firmware", &components, &count);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    CHECK(count == 1);
    CHECK(strcmp(components[0].id, "fw-main") == 0);
    ADUC_ComponentRegistry_FreeComponents(components, count);

    res = ADUC_ComponentRegistry_FindByGroup(handle, "software", &components, &count);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    CHECK(count == 1);
    CHECK(strcmp(components[0].id, "app-pkg") == 0);
    ADUC_ComponentRegistry_FreeComponents(components, count);

    res = ADUC_ComponentRegistry_FindByGroup(handle, "nonexistent", &components, &count);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    CHECK(count == 0);

    ADUC_ComponentRegistry_Destroy(handle);
}

TEST_CASE("ComponentRegistry: FindById works", "[component_registry]")
{
    ADUC_ComponentRegistryHandle handle = nullptr;
    ADUC_ComponentRegistry_Create(&handle);
    ADUC_ComponentRegistry_AddEnumerator(handle, &s_mockVtable);
    ADUC_ComponentRegistry_Refresh(handle);

    ADUC_ComponentInfo comp;
    memset(&comp, 0, sizeof(comp));

    ADUC_Result2 res = ADUC_ComponentRegistry_FindById(handle, "fw-main", &comp);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));
    CHECK(strcmp(comp.name, "Main Firmware") == 0);

    res = ADUC_ComponentRegistry_FindById(handle, "nonexistent", &comp);
    CHECK(ADUC_RESULT2_IS_FAILURE(res));

    ADUC_ComponentRegistry_Destroy(handle);
}

TEST_CASE("Targeting: simple match", "[component_targeting]")
{
    InitMockComponents();
    CHECK(ADUC_Targeting_Matches(&s_mockComponents[0], "group=firmware"));
    CHECK(ADUC_Targeting_Matches(&s_mockComponents[1], "group=software"));
    CHECK_FALSE(ADUC_Targeting_Matches(&s_mockComponents[0], "group=software"));
}

TEST_CASE("Targeting: AND match", "[component_targeting]")
{
    InitMockComponents();
    CHECK(ADUC_Targeting_Matches(&s_mockComponents[0], "manufacturer=Contoso AND model=Widget-1000"));
    CHECK(ADUC_Targeting_Matches(&s_mockComponents[0], "group=firmware AND manufacturer=Contoso"));
    CHECK_FALSE(ADUC_Targeting_Matches(&s_mockComponents[0], "group=firmware AND model=Other"));
}

TEST_CASE("Targeting: no match", "[component_targeting]")
{
    InitMockComponents();
    CHECK_FALSE(ADUC_Targeting_Matches(&s_mockComponents[0], "group=nonexistent"));
    CHECK_FALSE(ADUC_Targeting_Matches(&s_mockComponents[0], "manufacturer=Unknown"));
}

TEST_CASE("Targeting: invalid expression", "[component_targeting]")
{
    CHECK_FALSE(ADUC_Targeting_IsValid(""));
    CHECK_FALSE(ADUC_Targeting_IsValid(nullptr));
    CHECK_FALSE(ADUC_Targeting_IsValid("noequals"));
    CHECK_FALSE(ADUC_Targeting_IsValid("=nokey"));
    CHECK(ADUC_Targeting_IsValid("group=firmware"));
    CHECK(ADUC_Targeting_IsValid("group=firmware AND model=Widget-1000"));
}
