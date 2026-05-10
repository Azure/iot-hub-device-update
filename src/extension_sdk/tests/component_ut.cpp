/**
 * @file component_ut.cpp
 * @brief Unit tests for the ADUC_Component model, property matching,
 *        targeting wildcards, and static enumerator property parsing.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>

extern "C"
{
#include "aduc/component.h"
#include "aduc/component_targeting.h"
#include "aduc/component_enumerator_vtable.h"
#include "aduc/component_registry.h"
}

/* ───────── helpers ───────── */

static ADUC_Component MakeFirmwareComponent()
{
    ADUC_Component c{};
    strncpy(c.id, "host-firmware", sizeof(c.id) - 1);
    strncpy(c.name, "Host Firmware", sizeof(c.name) - 1);
    strncpy(c.group, "firmware", sizeof(c.group) - 1);
    strncpy(c.manufacturer, "contoso", sizeof(c.manufacturer) - 1);
    strncpy(c.model, "toaster", sizeof(c.model) - 1);
    strncpy(c.installedVersion, "1.0.0", sizeof(c.installedVersion) - 1);
    c.isUpdatable = true;

    strncpy(c.properties[0].key, "type", sizeof(c.properties[0].key) - 1);
    strncpy(c.properties[0].value, "firmware", sizeof(c.properties[0].value) - 1);
    strncpy(c.properties[1].key, "partition", sizeof(c.properties[1].key) - 1);
    strncpy(c.properties[1].value, "A", sizeof(c.properties[1].value) - 1);
    c.propertyCount = 2;
    return c;
}

static ADUC_Component MakeModemComponent()
{
    ADUC_Component c{};
    strncpy(c.id, "modem-firmware", sizeof(c.id) - 1);
    strncpy(c.name, "Modem Firmware", sizeof(c.name) - 1);
    strncpy(c.group, "firmware", sizeof(c.group) - 1);
    strncpy(c.manufacturer, "contoso", sizeof(c.manufacturer) - 1);
    strncpy(c.model, "toaster-modem", sizeof(c.model) - 1);
    strncpy(c.installedVersion, "2.1.0", sizeof(c.installedVersion) - 1);
    c.isUpdatable = true;

    strncpy(c.properties[0].key, "type", sizeof(c.properties[0].key) - 1);
    strncpy(c.properties[0].value, "firmware", sizeof(c.properties[0].value) - 1);
    strncpy(c.properties[1].key, "interface", sizeof(c.properties[1].key) - 1);
    strncpy(c.properties[1].value, "lte", sizeof(c.properties[1].value) - 1);
    c.propertyCount = 2;
    return c;
}

static ADUC_Component MakeAppComponent()
{
    ADUC_Component c{};
    strncpy(c.id, "user-app", sizeof(c.id) - 1);
    strncpy(c.name, "User Application", sizeof(c.name) - 1);
    strncpy(c.group, "apps", sizeof(c.group) - 1);
    strncpy(c.manufacturer, "contoso", sizeof(c.manufacturer) - 1);
    strncpy(c.model, "toaster", sizeof(c.model) - 1);
    strncpy(c.installedVersion, "4.0.0", sizeof(c.installedVersion) - 1);
    c.isUpdatable = false;
    c.propertyCount = 0;
    return c;
}

static ADUC_ComponentList MakeTestList()
{
    ADUC_ComponentList list{};
    list.components[0] = MakeFirmwareComponent();
    list.components[1] = MakeModemComponent();
    list.components[2] = MakeAppComponent();
    list.count = 3;
    return list;
}

/* ───────── ADUC_Component_MatchesProperties ───────── */

TEST_CASE("MatchesProperties: exact match on built-in field", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "firmware", sizeof(q[0].value) - 1);
    CHECK(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: exact match on custom property", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "partition", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "A", sizeof(q[0].value) - 1);
    CHECK(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: mismatch returns false", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "apps", sizeof(q[0].value) - 1);
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: unknown key returns false", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "nonexistent", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "x", sizeof(q[0].value) - 1);
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: wildcard match", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "partition", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "*", sizeof(q[0].value) - 1);
    CHECK(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: wildcard on empty field returns false", "[component]")
{
    ADUC_Component c{};
    /* id is empty string */
    c.propertyCount = 0;
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "id", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "*", sizeof(q[0].value) - 1);
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, q, 1));
}

TEST_CASE("MatchesProperties: multi-property all must match", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[2]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "firmware", sizeof(q[0].value) - 1);
    strncpy(q[1].key, "manufacturer", sizeof(q[1].key) - 1);
    strncpy(q[1].value, "contoso", sizeof(q[1].value) - 1);
    CHECK(ADUC_Component_MatchesProperties(&c, q, 2));
}

TEST_CASE("MatchesProperties: multi-property partial mismatch returns false", "[component]")
{
    ADUC_Component c = MakeFirmwareComponent();
    ADUC_ComponentProperty q[2]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "firmware", sizeof(q[0].value) - 1);
    strncpy(q[1].key, "manufacturer", sizeof(q[1].key) - 1);
    strncpy(q[1].value, "fabrikam", sizeof(q[1].value) - 1);
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, q, 2));
}

TEST_CASE("MatchesProperties: null args return false", "[component]")
{
    ADUC_ComponentProperty q[1]{};
    CHECK_FALSE(ADUC_Component_MatchesProperties(nullptr, q, 1));

    ADUC_Component c = MakeFirmwareComponent();
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, nullptr, 1));
    CHECK_FALSE(ADUC_Component_MatchesProperties(&c, q, 0));
}

/* ───────── ADUC_Component_FindMatching ───────── */

TEST_CASE("FindMatching: match by group returns correct components", "[component]")
{
    ADUC_ComponentList list = MakeTestList();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "firmware", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[ADUC_MAX_COMPONENTS]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, ADUC_MAX_COMPONENTS);

    CHECK(count == 2);
    CHECK(strcmp(matches[0]->id, "host-firmware") == 0);
    CHECK(strcmp(matches[1]->id, "modem-firmware") == 0);
}

TEST_CASE("FindMatching: match by custom property", "[component]")
{
    ADUC_ComponentList list = MakeTestList();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "interface", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "lte", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[ADUC_MAX_COMPONENTS]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, ADUC_MAX_COMPONENTS);

    CHECK(count == 1);
    CHECK(strcmp(matches[0]->id, "modem-firmware") == 0);
}

TEST_CASE("FindMatching: no matches", "[component]")
{
    ADUC_ComponentList list = MakeTestList();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "nonexistent", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[ADUC_MAX_COMPONENTS]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, ADUC_MAX_COMPONENTS);
    CHECK(count == 0);
}

TEST_CASE("FindMatching: empty list returns 0", "[component]")
{
    ADUC_ComponentList list{};
    list.count = 0;

    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "firmware", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[ADUC_MAX_COMPONENTS]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, ADUC_MAX_COMPONENTS);
    CHECK(count == 0);
}

TEST_CASE("FindMatching: maxMatches limits output", "[component]")
{
    ADUC_ComponentList list = MakeTestList();
    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "manufacturer", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "contoso", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[1]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, 1);
    CHECK(count == 1);
}

TEST_CASE("FindMatching: null args return 0", "[component]")
{
    ADUC_ComponentProperty q[1]{};
    const ADUC_Component* matches[1]{};

    CHECK(ADUC_Component_FindMatching(nullptr, q, 1, matches, 1) == 0);

    ADUC_ComponentList list = MakeTestList();
    CHECK(ADUC_Component_FindMatching(&list, q, 1, nullptr, 1) == 0);
    CHECK(ADUC_Component_FindMatching(&list, q, 1, matches, 0) == 0);
}

/* ───────── Targeting wildcard support ───────── */

TEST_CASE("Targeting: wildcard match with ADUC_ComponentInfo", "[component_targeting]")
{
    ADUC_ComponentInfo ci{};
    strncpy(ci.id, "fw-main", sizeof(ci.id) - 1);
    strncpy(ci.name, "Main Firmware", sizeof(ci.name) - 1);
    strncpy(ci.group, "firmware", sizeof(ci.group) - 1);
    strncpy(ci.manufacturer, "Contoso", sizeof(ci.manufacturer) - 1);
    strncpy(ci.model, "Widget-1000", sizeof(ci.model) - 1);
    strncpy(ci.version, "1.2.3", sizeof(ci.version) - 1);
    ci.properties.entries = nullptr;
    ci.properties.count = 0;

    CHECK(ADUC_Targeting_Matches(&ci, "group=*"));
    CHECK(ADUC_Targeting_Matches(&ci, "manufacturer=*"));
    CHECK(ADUC_Targeting_Matches(&ci, "group=firmware AND manufacturer=*"));
}

TEST_CASE("Targeting: wildcard on property key via ADUC_Targeting", "[component_targeting]")
{
    /* Component with a custom property */
    ADUC_PropertyEntry entries[1];
    entries[0].key = "partition";
    entries[0].value = "A";

    ADUC_ComponentInfo ci{};
    strncpy(ci.id, "fw-1", sizeof(ci.id) - 1);
    strncpy(ci.group, "firmware", sizeof(ci.group) - 1);
    ci.properties.entries = entries;
    ci.properties.count = 1;

    CHECK(ADUC_Targeting_Matches(&ci, "partition=*"));
    CHECK(ADUC_Targeting_Matches(&ci, "partition=A"));
    CHECK_FALSE(ADUC_Targeting_Matches(&ci, "partition=B"));
    CHECK_FALSE(ADUC_Targeting_Matches(&ci, "nonexistent=*"));

    /* reset so no double-free */
    ci.properties.entries = nullptr;
    ci.properties.count = 0;
}

/* ───────── Registry with multiple enumerators ───────── */

static ADUC_ComponentInfo s_enumA[1];
static ADUC_ComponentInfo s_enumB[1];

static ADUC_Result2 EnumA_Enumerate(ADUC_ComponentInfo** out, size_t* outCount)
{
    memset(s_enumA, 0, sizeof(s_enumA));
    strncpy(s_enumA[0].id, "comp-a", sizeof(s_enumA[0].id) - 1);
    strncpy(s_enumA[0].group, "groupA", sizeof(s_enumA[0].group) - 1);

    auto* copy = static_cast<ADUC_ComponentInfo*>(malloc(sizeof(s_enumA)));
    memcpy(copy, s_enumA, sizeof(s_enumA));
    *out = copy;
    *outCount = 1;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 EnumB_Enumerate(ADUC_ComponentInfo** out, size_t* outCount)
{
    memset(s_enumB, 0, sizeof(s_enumB));
    strncpy(s_enumB[0].id, "comp-b", sizeof(s_enumB[0].id) - 1);
    strncpy(s_enumB[0].group, "groupB", sizeof(s_enumB[0].group) - 1);

    auto* copy = static_cast<ADUC_ComponentInfo*>(malloc(sizeof(s_enumB)));
    memcpy(copy, s_enumB, sizeof(s_enumB));
    *out = copy;
    *outCount = 1;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Stub_GetById(const char*, ADUC_ComponentInfo*) { return ADUC_RESULT2_SUCCESS; }
static bool Stub_Matches(const ADUC_ComponentInfo*, const char*) { return false; }
static ADUC_Result2 Stub_GetVersion(const char*, char*, size_t) { return ADUC_RESULT2_SUCCESS; }
static ADUC_Result2 Stub_Refresh() { return ADUC_RESULT2_SUCCESS; }
static void Stub_Free(ADUC_ComponentInfo* c, size_t) { free(c); }

static const ADUC_ComponentEnumeratorVtable s_vtableA = {
    EnumA_Enumerate, Stub_GetById, Stub_Matches, Stub_GetVersion, Stub_Refresh, Stub_Free
};
static const ADUC_ComponentEnumeratorVtable s_vtableB = {
    EnumB_Enumerate, Stub_GetById, Stub_Matches, Stub_GetVersion, Stub_Refresh, Stub_Free
};

TEST_CASE("Registry: multiple enumerators aggregate components", "[component_registry]")
{
    ADUC_ComponentRegistryHandle handle = nullptr;
    ADUC_Result2 res = ADUC_ComponentRegistry_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(res));

    ADUC_ComponentRegistry_AddEnumerator(handle, &s_vtableA);
    ADUC_ComponentRegistry_AddEnumerator(handle, &s_vtableB);
    ADUC_ComponentRegistry_Refresh(handle);

    CHECK(ADUC_ComponentRegistry_GetCount(handle) == 2);

    ADUC_ComponentInfo* all = nullptr;
    size_t count = 0;
    ADUC_ComponentRegistry_GetAll(handle, &all, &count);
    REQUIRE(count == 2);
    CHECK(strcmp(all[0].id, "comp-a") == 0);
    CHECK(strcmp(all[1].id, "comp-b") == 0);

    ADUC_ComponentRegistry_FreeComponents(all, count);

    /* FindByGroup */
    ADUC_ComponentInfo* ga = nullptr;
    size_t gc = 0;
    ADUC_ComponentRegistry_FindByGroup(handle, "groupA", &ga, &gc);
    CHECK(gc == 1);
    CHECK(strcmp(ga[0].id, "comp-a") == 0);
    ADUC_ComponentRegistry_FreeComponents(ga, gc);

    ADUC_ComponentRegistry_Destroy(handle);
}

/* ───────── Static enumerator with properties ───────── */

#ifdef __has_include
#if __has_include("static_enumerator.h")
#define HAS_STATIC_ENUMERATOR 1
#endif
#endif

/* Always include the header when building the extension_sdk_tests binary,
 * since the static_enumerator source is part of the static_file_enumerator
 * shared lib that may or may not be linked.  We conditionally compile
 * the TOML-based test only when we can write files (non-cross-compile). */

#ifndef CMAKE_CROSSCOMPILING
#include <cstdlib>

static const char* s_toml_with_props =
    "[[components]]\n"
    "id = \"host-fw\"\n"
    "name = \"Host Firmware\"\n"
    "group = \"firmware\"\n"
    "manufacturer = \"contoso\"\n"
    "model = \"toaster\"\n"
    "version = \"1.0.0\"\n"
    "\n"
    "[components.properties]\n"
    "type = \"firmware\"\n"
    "partition = \"A\"\n"
    "\n"
    "[[components]]\n"
    "id = \"modem-fw\"\n"
    "name = \"Modem Firmware\"\n"
    "group = \"firmware\"\n"
    "manufacturer = \"contoso\"\n"
    "model = \"toaster-modem\"\n"
    "version = \"2.1.0\"\n"
    "\n"
    "[components.properties]\n"
    "type = \"firmware\"\n"
    "interface = \"lte\"\n";

/* We use the StaticEnumerator via its vtable exported from the .so.
 * For unit testing without the .so, we include the source directly
 * only if the build links it.  Since the test binary links
 * aduc_extension_sdk (not the enumerator .so), we test the registry
 * and targeting layers here, and leave the TOML parsing to an
 * integration test that loads the .so.  The TOML read path is
 * already exercised by the static_file_enumerator unit when linked. */
#endif /* CMAKE_CROSSCOMPILING */

/* ───────── Edge cases ───────── */

TEST_CASE("FindMatching: max components boundary", "[component]")
{
    ADUC_ComponentList list{};
    list.count = ADUC_MAX_COMPONENTS;
    for (int i = 0; i < ADUC_MAX_COMPONENTS; i++)
    {
        snprintf(list.components[i].id, sizeof(list.components[i].id), "comp-%d", i);
        strncpy(list.components[i].group, "all", sizeof(list.components[i].group) - 1);
        list.components[i].propertyCount = 0;
    }

    ADUC_ComponentProperty q[1]{};
    strncpy(q[0].key, "group", sizeof(q[0].key) - 1);
    strncpy(q[0].value, "all", sizeof(q[0].value) - 1);

    const ADUC_Component* matches[ADUC_MAX_COMPONENTS]{};
    int count = ADUC_Component_FindMatching(&list, q, 1, matches, ADUC_MAX_COMPONENTS);
    CHECK(count == ADUC_MAX_COMPONENTS);
}
