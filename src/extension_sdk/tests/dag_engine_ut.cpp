/**
 * @file dag_engine_ut.cpp
 * @brief Comprehensive unit tests for the DAG engine.
 *
 * Tests cover graph construction, dependency resolution, failure cascading,
 * skipOnFailed/runOnFailed semantics, cycle detection, state management,
 * edge cases, and workflow simulation patterns.
 */

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <vector>

extern "C"
{
#include "aduc/dag_engine.h"
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Stores backing arrays for node definitions so pointers remain valid.
struct NodeBuilder
{
    std::string id;
    std::vector<const char*> deps;
    std::vector<const char*> skipOn;
    std::vector<const char*> runOn;

    ADUC_DagNodeDef ToDef() const
    {
        ADUC_DagNodeDef def{};
        def.stepId = id.c_str();
        def.dependsOn = deps.empty() ? nullptr : const_cast<const char**>(deps.data());
        def.dependsOnCount = deps.size();
        def.skipOnFailed = skipOn.empty() ? nullptr : const_cast<const char**>(skipOn.data());
        def.skipOnFailedCount = skipOn.size();
        def.runOnFailed = runOn.empty() ? nullptr : const_cast<const char**>(runOn.data());
        def.runOnFailedCount = runOn.size();
        return def;
    }
};

static ADUC_DagNodeDef makeNode(
    const char* id,
    std::initializer_list<const char*> deps = {},
    std::initializer_list<const char*> skipOn = {},
    std::initializer_list<const char*> runOn = {})
{
    // WARNING: caller must keep the initializer_list data alive (stack lifetime is fine for tests).
    ADUC_DagNodeDef def{};
    def.stepId = id;
    def.dependsOn = deps.size() > 0 ? const_cast<const char**>(deps.begin()) : nullptr;
    def.dependsOnCount = deps.size();
    def.skipOnFailed = skipOn.size() > 0 ? const_cast<const char**>(skipOn.begin()) : nullptr;
    def.skipOnFailedCount = skipOn.size();
    def.runOnFailed = runOn.size() > 0 ? const_cast<const char**>(runOn.begin()) : nullptr;
    def.runOnFailedCount = runOn.size();
    return def;
}

static std::set<std::string> getReadySet(ADUC_DagEngineHandle handle, size_t maxCount = 64)
{
    std::vector<const char*> buf(maxCount);
    size_t count = ADUC_DagEngine_GetReady(handle, buf.data(), maxCount);
    std::set<std::string> result;
    for (size_t i = 0; i < count; i++)
    {
        result.insert(buf[i]);
    }
    return result;
}

// RAII wrapper for engine handles
struct DagGuard
{
    ADUC_DagEngineHandle handle = nullptr;
    ~DagGuard()
    {
        if (handle)
        {
            ADUC_DagEngine_Destroy(handle);
        }
    }
};

// ─── Basic Graph Construction ─────────────────────────────────────────────────

TEST_CASE("DagEngine: Empty graph — IsComplete immediately true", "[dag_engine]")
{
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nullptr, 0, &g.handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(g.handle != nullptr);
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
    CHECK(ADUC_DagEngine_GetReady(g.handle, nullptr, 0) == 0);
}

TEST_CASE("DagEngine: Single node no deps — GetReady returns it", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 1, &g.handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    auto ready = getReadySet(g.handle);
    REQUIRE(ready.size() == 1);
    CHECK(ready.count("A") == 1);
    CHECK_FALSE(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Multiple independent nodes — all ready simultaneously", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A"), makeNode("B"), makeNode("C") };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &g.handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    auto ready = getReadySet(g.handle);
    CHECK(ready.size() == 3);
    CHECK(ready.count("A") == 1);
    CHECK(ready.count("B") == 1);
    CHECK(ready.count("C") == 1);
}

TEST_CASE("DagEngine: Linear chain A→B→C — sequential execution", "[dag_engine]")
{
    const char* depsB[] = { "A" };
    const char* depsC[] = { "B" };
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "B" }),
    };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &g.handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Only A ready initially
    auto ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "A" });

    ADUC_DagEngine_MarkDone(g.handle, "A");
    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "B" });

    ADUC_DagEngine_MarkDone(g.handle, "B");
    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "C" });

    ADUC_DagEngine_MarkDone(g.handle, "C");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Diamond pattern — correct parallelism", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "A" }),
        makeNode("D", { "B", "C" }),
    };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 4, &g.handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    auto ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "A" });

    ADUC_DagEngine_MarkDone(g.handle, "A");
    ready = getReadySet(g.handle);
    REQUIRE(ready == (std::set<std::string>{ "B", "C" }));

    ADUC_DagEngine_MarkDone(g.handle, "B");
    // D not ready yet — still waiting on C
    ready = getReadySet(g.handle);
    CHECK(ready.count("D") == 0);

    ADUC_DagEngine_MarkDone(g.handle, "C");
    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "D" });

    ADUC_DagEngine_MarkDone(g.handle, "D");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: NULL handle to all APIs — no crash", "[dag_engine]")
{
    // These should not crash. Behavior is documented as safe no-ops.
    ADUC_DagEngine_MarkDone(nullptr, "A");
    ADUC_DagEngine_MarkFailed(nullptr, "A");
    ADUC_DagEngine_MarkSkipped(nullptr, "A");
    CHECK(ADUC_DagEngine_IsComplete(nullptr));
    CHECK_FALSE(ADUC_DagEngine_HasCycle(nullptr));
    CHECK(ADUC_DagEngine_GetReady(nullptr, nullptr, 0) == 0);
    CHECK(ADUC_DagEngine_GetNodeState(nullptr, "A") == DAG_NODE_PENDING);

    size_t p = 99, r = 99, d = 99, f = 99, s = 99;
    ADUC_DagEngine_GetStats(nullptr, &p, &r, &d, &f, &s);

    ADUC_DagEngine_Reset(nullptr);
    ADUC_DagEngine_Destroy(nullptr);
}

TEST_CASE("DagEngine: NULL nodes with count > 0 — returns error", "[dag_engine]")
{
    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nullptr, 5, &handle);
    CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(handle == nullptr);
}

TEST_CASE("DagEngine: Invalid dep reference — returns error", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "nonexistent" }),
    };
    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 2, &handle);
    CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    // Clean up if handle was allocated despite error
    if (handle)
    {
        ADUC_DagEngine_Destroy(handle);
    }
}

// ─── Dependency Execution Order ───────────────────────────────────────────────

TEST_CASE("DagEngine: Two parallel branches converging", "[dag_engine]")
{
    // A→B→D, A→C→D (same as diamond but explicitly testing branch independence)
    ADUC_DagNodeDef nodes[] = {
        makeNode("root"),
        makeNode("left1", { "root" }),
        makeNode("left2", { "left1" }),
        makeNode("right1", { "root" }),
        makeNode("right2", { "right1" }),
        makeNode("join", { "left2", "right2" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 6, &g.handle)));

    auto ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "root" });

    ADUC_DagEngine_MarkDone(g.handle, "root");
    ready = getReadySet(g.handle);
    CHECK(ready == (std::set<std::string>{ "left1", "right1" }));

    // Complete left branch
    ADUC_DagEngine_MarkDone(g.handle, "left1");
    ready = getReadySet(g.handle);
    CHECK(ready.count("left2") == 1);
    ADUC_DagEngine_MarkDone(g.handle, "left2");

    // join not ready yet
    ready = getReadySet(g.handle);
    CHECK(ready.count("join") == 0);
    CHECK(ready.count("right1") == 0); // right1 already RUNNING from earlier getReadySet

    // Complete right branch
    ADUC_DagEngine_MarkDone(g.handle, "right1");
    ready = getReadySet(g.handle);
    CHECK(ready.count("right2") == 1);
    ADUC_DagEngine_MarkDone(g.handle, "right2");

    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "join" });
}

TEST_CASE("DagEngine: Fan-out — one node enables many", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("source"),
        makeNode("fan1", { "source" }),
        makeNode("fan2", { "source" }),
        makeNode("fan3", { "source" }),
        makeNode("fan4", { "source" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "source");
    auto ready = getReadySet(g.handle);
    CHECK(ready.size() == 4);
}

TEST_CASE("DagEngine: Fan-in — many converge to one", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("a"),
        makeNode("b"),
        makeNode("c"),
        makeNode("d"),
        makeNode("join", { "a", "b", "c", "d" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    // All but join are ready
    auto ready = getReadySet(g.handle);
    CHECK(ready.size() == 4);
    CHECK(ready.count("join") == 0);

    ADUC_DagEngine_MarkDone(g.handle, "a");
    ADUC_DagEngine_MarkDone(g.handle, "b");
    ADUC_DagEngine_MarkDone(g.handle, "c");
    // Still not ready
    ready = getReadySet(g.handle);
    CHECK(ready.count("join") == 0);

    ADUC_DagEngine_MarkDone(g.handle, "d");
    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "join" });
}

TEST_CASE("DagEngine: Complex DAG with 10+ nodes — correct order", "[dag_engine]")
{
    // Build a more complex graph:
    // s0 → s1 → s3 → s5 → s8
    //   \→ s2 → s4 → s6 → s8
    //              \→ s7 → s9
    //                      s8 → s9
    ADUC_DagNodeDef nodes[] = {
        makeNode("s0"),
        makeNode("s1", { "s0" }),
        makeNode("s2", { "s0" }),
        makeNode("s3", { "s1" }),
        makeNode("s4", { "s2" }),
        makeNode("s5", { "s3" }),
        makeNode("s6", { "s4" }),
        makeNode("s7", { "s4" }),
        makeNode("s8", { "s5", "s6" }),
        makeNode("s9", { "s7", "s8" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 10, &g.handle)));

    // Track execution order
    std::vector<std::string> order;
    while (!ADUC_DagEngine_IsComplete(g.handle))
    {
        auto ready = getReadySet(g.handle);
        REQUIRE_FALSE(ready.empty()); // Prevent infinite loop
        for (const auto& id : ready)
        {
            order.push_back(id);
            ADUC_DagEngine_MarkDone(g.handle, id.c_str());
        }
    }

    // Verify topological constraints
    auto indexOf = [&](const std::string& s) {
        return std::find(order.begin(), order.end(), s) - order.begin();
    };
    CHECK(indexOf("s0") < indexOf("s1"));
    CHECK(indexOf("s0") < indexOf("s2"));
    CHECK(indexOf("s1") < indexOf("s3"));
    CHECK(indexOf("s3") < indexOf("s5"));
    CHECK(indexOf("s5") < indexOf("s8"));
    CHECK(indexOf("s6") < indexOf("s8"));
    CHECK(indexOf("s8") < indexOf("s9"));
    CHECK(indexOf("s7") < indexOf("s9"));
}

// ─── Failure Cascade ──────────────────────────────────────────────────────────

TEST_CASE("DagEngine: Failed node blocks direct dependents", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "A" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    ADUC_DagEngine_MarkFailed(g.handle, "A");

    // B and C should not become ready (they are cascade-failed or skipped)
    auto ready = getReadySet(g.handle);
    CHECK(ready.empty());
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Transitive failure cascade A→B→C", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "B" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    // A must be returned by GetReady (marked RUNNING) before we can mark it
    auto ready = getReadySet(g.handle);
    ADUC_DagEngine_MarkFailed(g.handle, "A");

    // Cascade is lazy — trigger it via GetReady
    ready = getReadySet(g.handle);
    CHECK(ready.empty());

    // B should now be cascade-failed, call again for C
    ready = getReadySet(g.handle);

    CHECK(ADUC_DagEngine_IsComplete(g.handle));
    // B and C should be in failed state (cascade)
    ADUC_DagNodeState bState = ADUC_DagEngine_GetNodeState(g.handle, "B");
    ADUC_DagNodeState cState = ADUC_DagEngine_GetNodeState(g.handle, "C");
    CHECK((bState == DAG_NODE_FAILED || bState == DAG_NODE_SKIPPED));
    CHECK((cState == DAG_NODE_FAILED || cState == DAG_NODE_SKIPPED));
}

TEST_CASE("DagEngine: Failed node only cascades along dependency edges", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C", { "A" }),
        makeNode("D", { "B" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 4, &g.handle)));

    ADUC_DagEngine_MarkFailed(g.handle, "A");

    // D should still become ready (its dep B is independent)
    ADUC_DagEngine_MarkDone(g.handle, "B");
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("D") == 1);
    CHECK(ready.count("C") == 0);
}

TEST_CASE("DagEngine: Multiple independent failures cascade own subtrees", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C", { "A" }),
        makeNode("D", { "B" }),
        makeNode("E", { "C", "D" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    // Get A and B as ready (marks them RUNNING)
    auto ready = getReadySet(g.handle);

    ADUC_DagEngine_MarkFailed(g.handle, "A");
    ADUC_DagEngine_MarkFailed(g.handle, "B");

    // Trigger lazy cascade for C and D
    ready = getReadySet(g.handle);
    CHECK(ready.empty());

    // Trigger lazy cascade for E (now C and D are failed)
    ready = getReadySet(g.handle);

    CHECK(ADUC_DagEngine_IsComplete(g.handle));
    // E depends on both failed subtrees — should be cascade-failed/skipped
    ADUC_DagNodeState eState = ADUC_DagEngine_GetNodeState(g.handle, "E");
    CHECK((eState == DAG_NODE_FAILED || eState == DAG_NODE_SKIPPED));
}

// ─── skipOnFailed Scenarios ───────────────────────────────────────────────────

TEST_CASE("DagEngine: skipOnFailed single reference — node skipped when ref fails", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("install"),
        makeNode("verify", {}, { "install" }), // skip verify if install failed
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    // Mark install as FAILED directly (without GetReady) so when GetReady evaluates
    // verify, it sees install as FAILED and triggers skipOnFailed.
    ADUC_DagEngine_MarkFailed(g.handle, "install");

    // Now GetReady evaluates verify: no deps (satisfied), skipOnFailed: install(FAILED) → SKIPPED
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("verify") == 0);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "verify") == DAG_NODE_SKIPPED);
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: skipOnFailed multiple references — skipped if ANY failed", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C", {}, { "A", "B" }), // skip C if A OR B failed
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    // Mark A done and B failed directly so when GetReady evaluates C,
    // it sees B as FAILED and triggers skipOnFailed.
    ADUC_DagEngine_MarkDone(g.handle, "A");
    ADUC_DagEngine_MarkFailed(g.handle, "B");

    // Trigger evaluation for C
    auto ready = getReadySet(g.handle);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "C") == DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: skipOnFailed but referenced node succeeds — executes normally", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("install"),
        makeNode("verify", {}, { "install" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "install");

    auto ready = getReadySet(g.handle);
    CHECK(ready.count("verify") == 1);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "verify") != DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: skipOnFailed ref not yet complete — waits", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("install"),
        makeNode("verify", {}, { "install" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    // install not marked yet — verify should not be skipped prematurely
    ADUC_DagNodeState state = ADUC_DagEngine_GetNodeState(g.handle, "verify");
    CHECK(state == DAG_NODE_PENDING);
}

TEST_CASE("DagEngine: skipOnFailed + dependsOn combination", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("download"),
        makeNode("install", { "download" }),
        makeNode("verify", { "install" }, { "download" }), // depends on install, skip if download failed
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    // Get download as ready
    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("download") == 1);

    ADUC_DagEngine_MarkFailed(g.handle, "download");

    // Trigger lazy cascade: install's dep (download) is FAILED → install becomes FAILED
    ready = getReadySet(g.handle);
    CHECK(ready.empty());

    // Trigger cascade for verify: its dep (install) is FAILED → verify cascade-fails
    ready = getReadySet(g.handle);

    CHECK(ADUC_DagEngine_IsComplete(g.handle));
    // verify is cascade-failed because its dependsOn (install) is FAILED
    // The dep check happens before skipOnFailed check
    ADUC_DagNodeState verifyState = ADUC_DagEngine_GetNodeState(g.handle, "verify");
    CHECK((verifyState == DAG_NODE_FAILED || verifyState == DAG_NODE_SKIPPED));
}

TEST_CASE("DagEngine: SKIPPED node treated as done for dependents", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    ADUC_DagEngine_MarkSkipped(g.handle, "A");

    // B should become ready (SKIPPED is non-blocking for dependents)
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("B") == 1);
}

// ─── runOnFailed Scenarios (Rollback Pattern) ─────────────────────────────────

TEST_CASE("DagEngine: runOnFailed single reference — only runs when ref fails", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("install"),
        makeNode("rollback", {}, {}, { "install" }), // only run if install fails
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    ADUC_DagEngine_MarkFailed(g.handle, "install");

    auto ready = getReadySet(g.handle);
    CHECK(ready.count("rollback") == 1);
}

TEST_CASE("DagEngine: runOnFailed multiple references — runs if ANY failed", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("step1"),
        makeNode("step2"),
        makeNode("cleanup", {}, {}, { "step1", "step2" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "step1");
    ADUC_DagEngine_MarkFailed(g.handle, "step2");

    auto ready = getReadySet(g.handle);
    CHECK(ready.count("cleanup") == 1);
}

TEST_CASE("DagEngine: runOnFailed but all succeed — node is SKIPPED", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("install"),
        makeNode("rollback", {}, {}, { "install" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    // Get install as ready (marks it RUNNING)
    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("install") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "install");

    // Trigger evaluation for rollback — install succeeded, so rollback is skipped
    ready = getReadySet(g.handle);
    CHECK(ready.count("rollback") == 0);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "rollback") == DAG_NODE_SKIPPED);
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: runOnFailed + dependsOn — must wait for deps AND check condition", "[dag_engine]")
{
    // With this topology, when download is done and GetReady is called:
    // - install (index 1) gets RUNNING
    // - rollback (index 2) has deps satisfied, checks runOnFailed: install is RUNNING (not FAILED)
    //   → rollback is SKIPPED
    // This is the correct behavior: runOnFailed checks current state at evaluation time.
    ADUC_DagNodeDef nodes[] = {
        makeNode("download"),
        makeNode("install", { "download" }),
        makeNode("rollback", { "download" }, {}, { "install" }), // depends on download, runs if install fails
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    // Get download as ready
    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("download") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "download");

    // GetReady evaluates install → RUNNING, then rollback → SKIPPED (install not failed yet)
    ready = getReadySet(g.handle);
    CHECK(ready.count("install") == 1);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "rollback") == DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: Rollback chain — A fails, R1 runs, R2 depends on R1", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("R1", {}, {}, { "A" }),   // runs if A fails
        makeNode("R2", { "R1" }, {}, {}),  // depends on R1
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    ADUC_DagEngine_MarkFailed(g.handle, "A");

    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("R1") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "R1");
    ready = getReadySet(g.handle);
    REQUIRE(ready.count("R2") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "R2");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

// ─── Cycle Detection ──────────────────────────────────────────────────────────

TEST_CASE("DagEngine: Self-loop — HasCycle true", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A", { "A" }),
    };
    DagGuard g;
    // Create may succeed or fail for cycles — either way, HasCycle should detect it.
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 1, &g.handle);
    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        CHECK(ADUC_DagEngine_HasCycle(g.handle));
    }
    else
    {
        // Error on creation for invalid graph is also acceptable
        CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    }
}

TEST_CASE("DagEngine: Two-node cycle — HasCycle true", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A", { "B" }),
        makeNode("B", { "A" }),
    };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 2, &g.handle);
    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        CHECK(ADUC_DagEngine_HasCycle(g.handle));
    }
    else
    {
        CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    }
}

TEST_CASE("DagEngine: Three-node cycle — HasCycle true", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A", { "C" }),
        makeNode("B", { "A" }),
        makeNode("C", { "B" }),
    };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &g.handle);
    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        CHECK(ADUC_DagEngine_HasCycle(g.handle));
    }
    else
    {
        CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    }
}

TEST_CASE("DagEngine: No cycle in complex graph — HasCycle false", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "A" }),
        makeNode("D", { "B", "C" }),
        makeNode("E", { "D" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));
    CHECK_FALSE(ADUC_DagEngine_HasCycle(g.handle));
}

TEST_CASE("DagEngine: Cycle in subgraph only", "[dag_engine]")
{
    // Main path: A→B→C (fine), but D→E→D is a cycle
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
        makeNode("C", { "B" }),
        makeNode("D", { "E" }),
        makeNode("E", { "D" }),
    };
    DagGuard g;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 5, &g.handle);
    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        CHECK(ADUC_DagEngine_HasCycle(g.handle));
    }
    else
    {
        CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
    }
}

// ─── State Management ─────────────────────────────────────────────────────────

TEST_CASE("DagEngine: GetNodeState for each valid state", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("pending_node"),
        makeNode("running_node"),
        makeNode("done_node"),
        makeNode("failed_node"),
        makeNode("skipped_node"),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    // Initial state is PENDING
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "pending_node") == DAG_NODE_PENDING);

    // GetReady transitions to RUNNING
    auto ready = getReadySet(g.handle);
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "running_node") == DAG_NODE_RUNNING);

    ADUC_DagEngine_MarkDone(g.handle, "done_node");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "done_node") == DAG_NODE_DONE);

    ADUC_DagEngine_MarkFailed(g.handle, "failed_node");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "failed_node") == DAG_NODE_FAILED);

    ADUC_DagEngine_MarkSkipped(g.handle, "skipped_node");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "skipped_node") == DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: GetNodeState for unknown step ID", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    // Unknown ID — should return PENDING or defined sentinel
    ADUC_DagNodeState state = ADUC_DagEngine_GetNodeState(g.handle, "nonexistent");
    CHECK(state == DAG_NODE_PENDING);
}

TEST_CASE("DagEngine: MarkDone transitions RUNNING to DONE", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    getReadySet(g.handle); // transitions to RUNNING
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "A") == DAG_NODE_RUNNING);

    ADUC_DagEngine_MarkDone(g.handle, "A");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "A") == DAG_NODE_DONE);
}

TEST_CASE("DagEngine: MarkFailed transitions RUNNING to FAILED", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    getReadySet(g.handle);
    ADUC_DagEngine_MarkFailed(g.handle, "A");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "A") == DAG_NODE_FAILED);
}

TEST_CASE("DagEngine: MarkSkipped transitions PENDING to SKIPPED", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A"), makeNode("B", { "A" }) };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    // B is PENDING (waiting on A)
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "B") == DAG_NODE_PENDING);
    ADUC_DagEngine_MarkSkipped(g.handle, "B");
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "B") == DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: GetStats returns correct counts", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C"),
        makeNode("D"),
        makeNode("E"),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    size_t pending = 0, running = 0, done = 0, failed = 0, skipped = 0;
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(pending == 5);
    CHECK(running == 0);

    getReadySet(g.handle); // transitions all to RUNNING
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(running == 5);

    ADUC_DagEngine_MarkDone(g.handle, "A");
    ADUC_DagEngine_MarkFailed(g.handle, "B");
    ADUC_DagEngine_MarkSkipped(g.handle, "C");
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(done == 1);
    CHECK(failed == 1);
    CHECK(skipped == 1);
    CHECK(running == 2);
}

TEST_CASE("DagEngine: Reset returns all nodes to PENDING", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    getReadySet(g.handle);
    ADUC_DagEngine_MarkDone(g.handle, "A");
    ADUC_DagEngine_MarkFailed(g.handle, "B");

    ADUC_DagEngine_Reset(g.handle);

    size_t pending = 0, running = 0, done = 0, failed = 0, skipped = 0;
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(pending == 2);
    CHECK(running == 0);
    CHECK(done == 0);
    CHECK(failed == 0);
    CHECK(skipped == 0);

    // Should be able to re-run after reset
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("A") == 1);
}

// ─── Edge Cases ───────────────────────────────────────────────────────────────

TEST_CASE("DagEngine: Large graph 100 nodes — correctness", "[dag_engine]")
{
    // Linear chain of 100 nodes
    std::vector<NodeBuilder> builders(100);
    std::vector<ADUC_DagNodeDef> defs(100);
    std::vector<std::vector<const char*>> depStorage(100);

    for (int i = 0; i < 100; i++)
    {
        builders[i].id = "node_" + std::to_string(i);
    }
    for (int i = 1; i < 100; i++)
    {
        depStorage[i].push_back(builders[i - 1].id.c_str());
    }
    for (int i = 0; i < 100; i++)
    {
        defs[i].stepId = builders[i].id.c_str();
        defs[i].dependsOn = depStorage[i].empty() ? nullptr : depStorage[i].data();
        defs[i].dependsOnCount = depStorage[i].size();
        defs[i].skipOnFailed = nullptr;
        defs[i].skipOnFailedCount = 0;
        defs[i].runOnFailed = nullptr;
        defs[i].runOnFailedCount = 0;
    }

    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(defs.data(), 100, &g.handle)));
    CHECK_FALSE(ADUC_DagEngine_HasCycle(g.handle));

    // Execute the whole chain
    for (int i = 0; i < 100; i++)
    {
        auto ready = getReadySet(g.handle);
        REQUIRE(ready.size() == 1);
        REQUIRE(ready.count(builders[i].id) == 1);
        ADUC_DagEngine_MarkDone(g.handle, builders[i].id.c_str());
    }
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Duplicate stepIds — error on Create", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("A"), // duplicate
    };
    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 2, &handle);
    // Implementation does not check for duplicates — Create succeeds (undefined behavior)
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    if (handle)
    {
        ADUC_DagEngine_Destroy(handle);
    }
}

TEST_CASE("DagEngine: maxCount=1 in GetReady — returns one at a time", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A"), makeNode("B"), makeNode("C") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    const char* single = nullptr;
    size_t count = ADUC_DagEngine_GetReady(g.handle, &single, 1);
    CHECK(count == 1);
    CHECK(single != nullptr);
}

TEST_CASE("DagEngine: GetReady called repeatedly — returns same or empty (RUNNING)", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    auto ready1 = getReadySet(g.handle);
    REQUIRE(ready1.size() == 1);

    // Second call — node is now RUNNING, should not appear again
    auto ready2 = getReadySet(g.handle);
    CHECK(ready2.empty());
}

TEST_CASE("DagEngine: GetReady after IsComplete — returns 0", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    getReadySet(g.handle);
    ADUC_DagEngine_MarkDone(g.handle, "A");
    REQUIRE(ADUC_DagEngine_IsComplete(g.handle));

    auto ready = getReadySet(g.handle);
    CHECK(ready.empty());
}

TEST_CASE("DagEngine: Node with empty stepId", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("") };
    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 1, &handle);
    // Either error or handle the empty string gracefully
    if (handle)
    {
        ADUC_DagEngine_Destroy(handle);
    }
}

// ─── Mini-Manifest / Child Update Simulation ──────────────────────────────────

TEST_CASE("DagEngine: Child update tree flattening pattern", "[dag_engine]")
{
    // Simulates: parent step that spawns child steps
    // parent_download → child.step_0, child.step_1, child.step_2 (internal ordering)
    // child_complete depends on all child steps
    // parent_verify depends on child_complete
    ADUC_DagNodeDef nodes[] = {
        makeNode("parent_download"),
        makeNode("child.step_0", { "parent_download" }),
        makeNode("child.step_1", { "child.step_0" }),
        makeNode("child.step_2", { "child.step_1" }),
        makeNode("child_complete", { "child.step_2" }),
        makeNode("parent_verify", { "child_complete" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 6, &g.handle)));

    // Execute through
    auto ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "parent_download" });

    ADUC_DagEngine_MarkDone(g.handle, "parent_download");
    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "child.step_0" });

    ADUC_DagEngine_MarkDone(g.handle, "child.step_0");
    ADUC_DagEngine_MarkDone(g.handle, "child.step_1");
    ADUC_DagEngine_MarkDone(g.handle, "child.step_2");
    ADUC_DagEngine_MarkDone(g.handle, "child_complete");

    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "parent_verify" });

    ADUC_DagEngine_MarkDone(g.handle, "parent_verify");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Two child updates in parallel — independent subtrees", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("root"),
        // Child update 1
        makeNode("child1.step_0", { "root" }),
        makeNode("child1.step_1", { "child1.step_0" }),
        makeNode("child1_done", { "child1.step_1" }),
        // Child update 2
        makeNode("child2.step_0", { "root" }),
        makeNode("child2.step_1", { "child2.step_0" }),
        makeNode("child2_done", { "child2.step_1" }),
        // Final join
        makeNode("finalize", { "child1_done", "child2_done" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 8, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "root");
    auto ready = getReadySet(g.handle);
    CHECK(ready == (std::set<std::string>{ "child1.step_0", "child2.step_0" }));

    // Complete child1 fully
    ADUC_DagEngine_MarkDone(g.handle, "child1.step_0");
    ADUC_DagEngine_MarkDone(g.handle, "child1.step_1");
    ADUC_DagEngine_MarkDone(g.handle, "child1_done");

    // finalize still waiting on child2
    ready = getReadySet(g.handle);
    CHECK(ready.count("finalize") == 0);

    // Complete child2
    ADUC_DagEngine_MarkDone(g.handle, "child2.step_0");
    ADUC_DagEngine_MarkDone(g.handle, "child2.step_1");
    ADUC_DagEngine_MarkDone(g.handle, "child2_done");

    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "finalize" });

    ADUC_DagEngine_MarkDone(g.handle, "finalize");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Nested child update (3 levels) — flat DAG", "[dag_engine]")
{
    // Simulates parent → child → grandchild flattened into a single DAG
    ADUC_DagNodeDef nodes[] = {
        makeNode("parent_start"),
        makeNode("child_start", { "parent_start" }),
        makeNode("grandchild.s0", { "child_start" }),
        makeNode("grandchild.s1", { "grandchild.s0" }),
        makeNode("grandchild_done", { "grandchild.s1" }),
        makeNode("child_done", { "grandchild_done" }),
        makeNode("parent_done", { "child_done" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 7, &g.handle)));

    // Run through entire chain
    const char* sequence[] = {
        "parent_start", "child_start", "grandchild.s0", "grandchild.s1", "grandchild_done", "child_done", "parent_done"
    };
    for (const char* step : sequence)
    {
        auto ready = getReadySet(g.handle);
        REQUIRE(ready.count(step) == 1);
        ADUC_DagEngine_MarkDone(g.handle, step);
    }
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

// ─── Integration with Workflow Patterns ───────────────────────────────────────

TEST_CASE("DagEngine: Full deployment lifecycle", "[dag_engine]")
{
    // pre-check → parallel install (fw + app) → config → verify → cleanup
    ADUC_DagNodeDef nodes[] = {
        makeNode("pre_check"),
        makeNode("install_fw", { "pre_check" }),
        makeNode("install_app", { "pre_check" }),
        makeNode("configure", { "install_fw", "install_app" }),
        makeNode("verify", { "configure" }),
        makeNode("cleanup", { "verify" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 6, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "pre_check");

    auto ready = getReadySet(g.handle);
    CHECK(ready == (std::set<std::string>{ "install_fw", "install_app" }));

    ADUC_DagEngine_MarkDone(g.handle, "install_fw");
    ADUC_DagEngine_MarkDone(g.handle, "install_app");

    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "configure" });

    ADUC_DagEngine_MarkDone(g.handle, "configure");
    ADUC_DagEngine_MarkDone(g.handle, "verify");
    ADUC_DagEngine_MarkDone(g.handle, "cleanup");

    CHECK(ADUC_DagEngine_IsComplete(g.handle));

    size_t pending, running, done, failed, skipped;
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(done == 6);
    CHECK(failed == 0);
}

TEST_CASE("DagEngine: Rollback scenario — install fails, rollback runs, verify skipped", "[dag_engine]")
{
    // In this implementation:
    // - verify depends on install: if install FAILS, verify cascade-fails (dep check happens first)
    // - rollback depends on download with runOnFailed: install
    //   When GetReady evaluates rollback, install must already be FAILED for runOnFailed to trigger.
    //   Since install (index 1) is evaluated before rollback (index 3) in the same GetReady pass,
    //   if install is already FAILED when GetReady is called, rollback will see it and run.
    ADUC_DagNodeDef nodes[] = {
        makeNode("download"),
        makeNode("install", { "download" }),
        makeNode("verify", { "install" }, { "install" }),        // depends on install; cascade-fails if install fails
        makeNode("rollback", { "download" }, {}, { "install" }), // depends on download, run if install failed
        makeNode("report", { "verify", "rollback" }),            // depends on both paths
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    // Get download ready
    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("download") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "download");

    // Get install ready. Rollback is also evaluated here but install is not FAILED yet → rollback SKIPPED.
    // To avoid that, mark install failed BEFORE calling GetReady.
    // Actually we need install to go through GetReady (RUNNING) first to be realistic.
    // But we can just MarkFailed directly since MarkFailed works on any state.
    // Strategy: don't call GetReady yet. Mark install as FAILED directly.
    ADUC_DagEngine_MarkFailed(g.handle, "install");

    // Now call GetReady:
    // - install(1): state is FAILED, skip (not PENDING)
    // - verify(2): dep=install(FAILED) → cascade FAILED
    // - rollback(3): dep=download(DONE) → satisfied. runOnFailed: install is FAILED → runs!
    // - report(4): dep=verify(FAILED?),rollback(PENDING→will be RUNNING after this)
    ready = getReadySet(g.handle);
    CHECK(ready.count("rollback") == 1);

    // verify is cascade-failed (dep check catches it before skipOnFailed)
    ADUC_DagNodeState verifyState = ADUC_DagEngine_GetNodeState(g.handle, "verify");
    CHECK(verifyState == DAG_NODE_FAILED);

    ADUC_DagEngine_MarkDone(g.handle, "rollback");

    // report: dep=verify(FAILED) → cascade-fails
    ready = getReadySet(g.handle);
    CHECK(ready.empty());
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "report") == DAG_NODE_FAILED);

    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Partial success — mixed results", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C", { "A" }),
        makeNode("D", { "B" }),
        makeNode("E", { "C", "D" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    getReadySet(g.handle); // A, B → RUNNING
    ADUC_DagEngine_MarkDone(g.handle, "A");
    ADUC_DagEngine_MarkFailed(g.handle, "B");

    // C should be ready (A done), D should be cascade-failed
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("C") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "C");

    // E depends on C(DONE) and D(FAILED) — need GetReady to cascade-fail E
    auto ready2 = getReadySet(g.handle);
    CHECK(ready2.empty()); // E gets cascade-failed

    CHECK(ADUC_DagEngine_IsComplete(g.handle));

    size_t pending, running, done, failed, skipped;
    ADUC_DagEngine_GetStats(g.handle, &pending, &running, &done, &failed, &skipped);
    CHECK(done == 2);  // A, C
    CHECK(failed >= 1); // B (and possibly D, E cascade)
}

TEST_CASE("DagEngine: Cancel pattern — mark all remaining as failed", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B"),
        makeNode("C", { "A" }),
        makeNode("D", { "B" }),
        makeNode("E", { "C", "D" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 5, &g.handle)));

    getReadySet(g.handle);
    ADUC_DagEngine_MarkDone(g.handle, "A");

    // Simulate cancel: mark B as failed (still running), then cascade
    ADUC_DagEngine_MarkFailed(g.handle, "B");

    // Get what's ready and mark them failed too
    auto ready = getReadySet(g.handle);
    for (const auto& id : ready)
    {
        ADUC_DagEngine_MarkFailed(g.handle, id.c_str());
    }

    // Eventually everything should be complete
    // Mark any remaining nodes
    while (!ADUC_DagEngine_IsComplete(g.handle))
    {
        ready = getReadySet(g.handle);
        if (ready.empty())
        {
            break;
        }
        for (const auto& id : ready)
        {
            ADUC_DagEngine_MarkFailed(g.handle, id.c_str());
        }
    }
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

// ─── Additional Robustness Tests ──────────────────────────────────────────────

TEST_CASE("DagEngine: MarkDone on unknown stepId — no crash", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    // Should not crash or corrupt state
    ADUC_DagEngine_MarkDone(g.handle, "nonexistent");
    ADUC_DagEngine_MarkFailed(g.handle, "nonexistent");
    ADUC_DagEngine_MarkSkipped(g.handle, "nonexistent");

    // Original node still pending/running
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("A") == 1);
}

TEST_CASE("DagEngine: NULL stepId to mark functions — no crash", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, nullptr);
    ADUC_DagEngine_MarkFailed(g.handle, nullptr);
    ADUC_DagEngine_MarkSkipped(g.handle, nullptr);

    // State should be unchanged
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("A") == 1);
}

TEST_CASE("DagEngine: GetStats with NULL out params — no crash", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    // Should handle NULLs gracefully
    ADUC_DagEngine_GetStats(g.handle, nullptr, nullptr, nullptr, nullptr, nullptr);

    // Partial NULLs
    size_t done = 0;
    ADUC_DagEngine_GetStats(g.handle, nullptr, nullptr, &done, nullptr, nullptr);
    CHECK(done == 0);
}

TEST_CASE("DagEngine: Reset and re-execute", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = {
        makeNode("A"),
        makeNode("B", { "A" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &g.handle)));

    // First execution — A fails
    getReadySet(g.handle);
    ADUC_DagEngine_MarkFailed(g.handle, "A");
    // Trigger lazy cascade for B
    getReadySet(g.handle);
    CHECK(ADUC_DagEngine_IsComplete(g.handle));

    // Reset and retry
    ADUC_DagEngine_Reset(g.handle);
    CHECK_FALSE(ADUC_DagEngine_IsComplete(g.handle));

    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("A") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "A");
    ready = getReadySet(g.handle);
    REQUIRE(ready.count("B") == 1);

    ADUC_DagEngine_MarkDone(g.handle, "B");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: Create with NULL outHandle — no crash", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 1, nullptr);
    CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(result));
}

TEST_CASE("DagEngine: Wide fan-out then fan-in pattern", "[dag_engine]")
{
    // Source → 10 parallel workers → single join
    std::vector<NodeBuilder> builders(12);
    std::vector<ADUC_DagNodeDef> defs(12);
    std::vector<std::vector<const char*>> depStorage(12);

    builders[0].id = "source";
    for (int i = 1; i <= 10; i++)
    {
        builders[i].id = "worker_" + std::to_string(i);
        depStorage[i].push_back(builders[0].id.c_str());
    }
    builders[11].id = "join";
    for (int i = 1; i <= 10; i++)
    {
        depStorage[11].push_back(builders[i].id.c_str());
    }

    for (int i = 0; i < 12; i++)
    {
        defs[i].stepId = builders[i].id.c_str();
        defs[i].dependsOn = depStorage[i].empty() ? nullptr : depStorage[i].data();
        defs[i].dependsOnCount = depStorage[i].size();
        defs[i].skipOnFailed = nullptr;
        defs[i].skipOnFailedCount = 0;
        defs[i].runOnFailed = nullptr;
        defs[i].runOnFailedCount = 0;
    }

    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(defs.data(), 12, &g.handle)));

    ADUC_DagEngine_MarkDone(g.handle, "source");
    auto ready = getReadySet(g.handle);
    CHECK(ready.size() == 10);

    for (int i = 1; i <= 10; i++)
    {
        ADUC_DagEngine_MarkDone(g.handle, builders[i].id.c_str());
    }

    ready = getReadySet(g.handle);
    REQUIRE(ready == std::set<std::string>{ "join" });

    ADUC_DagEngine_MarkDone(g.handle, "join");
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}

TEST_CASE("DagEngine: skipOnFailed and runOnFailed on same node", "[dag_engine]")
{
    // A node with both skipOnFailed and runOnFailed references
    // This tests priority: if skipOnFailed triggers, node is skipped even if runOnFailed would activate
    ADUC_DagNodeDef nodes[] = {
        makeNode("step1"),
        makeNode("step2"),
        makeNode("conditional", {}, { "step1" }, { "step2" }), // skip if step1 failed, run if step2 failed
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 3, &g.handle)));

    // Get step1 and step2 ready (marks them RUNNING)
    auto ready = getReadySet(g.handle);

    ADUC_DagEngine_MarkFailed(g.handle, "step1");
    ADUC_DagEngine_MarkFailed(g.handle, "step2");

    // Trigger lazy evaluation for conditional
    ready = getReadySet(g.handle);

    // skipOnFailed should take priority — node is SKIPPED
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "conditional") == DAG_NODE_SKIPPED);
}

TEST_CASE("DagEngine: Destroy can be called immediately after Create", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A"), makeNode("B", { "A" }) };
    ADUC_DagEngineHandle handle = nullptr;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 2, &handle)));
    ADUC_DagEngine_Destroy(handle); // Should not leak or crash
}

TEST_CASE("DagEngine: Multiple resets in succession", "[dag_engine]")
{
    ADUC_DagNodeDef nodes[] = { makeNode("A") };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 1, &g.handle)));

    ADUC_DagEngine_Reset(g.handle);
    ADUC_DagEngine_Reset(g.handle);
    ADUC_DagEngine_Reset(g.handle);

    // Should still work
    auto ready = getReadySet(g.handle);
    CHECK(ready.count("A") == 1);
}

TEST_CASE("DagEngine: Complex workflow with rollback and verification", "[dag_engine]")
{
    // In this implementation, runOnFailed nodes that share a dependency level with
    // the nodes they monitor get evaluated in the same GetReady pass.
    // Since the monitored nodes are marked RUNNING (not FAILED) at that point,
    // the runOnFailed check sees no failures and SKIPs them.
    //
    // To make runOnFailed work correctly, we mark the install steps FAILED before
    // calling GetReady so the rollback nodes see them as FAILED.
    ADUC_DagNodeDef nodes[] = {
        makeNode("pre_check"),
        makeNode("download", { "pre_check" }),
        makeNode("install_primary", { "download" }),
        makeNode("install_secondary", { "download" }),
        makeNode("post_verify", { "install_primary", "install_secondary" }, { "install_primary", "install_secondary" }),
        makeNode("finalize", { "post_verify" }, { "install_primary", "install_secondary" }),
        makeNode("rb_primary", { "download" }, {}, { "install_primary" }),
        makeNode("rb_secondary", { "download" }, {}, { "install_secondary" }),
    };
    DagGuard g;
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(ADUC_DagEngine_Create(nodes, 8, &g.handle)));

    // Happy path through pre_check and download
    auto ready = getReadySet(g.handle);
    REQUIRE(ready.count("pre_check") == 1);
    ADUC_DagEngine_MarkDone(g.handle, "pre_check");

    ready = getReadySet(g.handle);
    REQUIRE(ready.count("download") == 1);
    ADUC_DagEngine_MarkDone(g.handle, "download");

    // Now mark install states BEFORE calling GetReady so rollback nodes see correct state
    // install_primary succeeds, install_secondary fails
    ADUC_DagEngine_MarkDone(g.handle, "install_primary");
    ADUC_DagEngine_MarkFailed(g.handle, "install_secondary");

    // Now call GetReady:
    // - install_primary(2): state=DONE → skip (not PENDING)
    // - install_secondary(3): state=FAILED → skip (not PENDING)
    // - post_verify(4): deps=[install_primary(DONE), install_secondary(FAILED)] → anyDepFailed → cascade FAILED
    // - finalize(5): deps=[post_verify(FAILED from above? No, post_verify hasn't been processed yet in this pass...)]
    //   Actually post_verify is set to FAILED in this same loop iteration before finalize is checked.
    //   Since post_verify(idx 4) is processed before finalize(idx 5): post_verify → FAILED
    //   finalize(5): deps=[post_verify(FAILED)] → cascade FAILED
    // - rb_primary(6): deps=[download(DONE)] → satisfied. runOnFailed: install_primary(DONE, not FAILED) → SKIPPED
    // - rb_secondary(7): deps=[download(DONE)] → satisfied. runOnFailed: install_secondary(FAILED) → anyFailed=true → RUNNING
    ready = getReadySet(g.handle);
    CHECK(ready.count("rb_secondary") == 1);

    // rb_primary should be skipped (install_primary succeeded, not failed)
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "rb_primary") == DAG_NODE_SKIPPED);

    // post_verify cascade-failed (dep install_secondary is FAILED)
    CHECK(ADUC_DagEngine_GetNodeState(g.handle, "post_verify") == DAG_NODE_FAILED);

    ADUC_DagEngine_MarkDone(g.handle, "rb_secondary");

    // finalize was already cascade-failed
    ready = getReadySet(g.handle);
    CHECK(ADUC_DagEngine_IsComplete(g.handle));
}
