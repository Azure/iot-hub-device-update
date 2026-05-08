/**
 * @file workflow_engine_ut.cpp
 * @brief Unit tests for the workflow engine, step lifecycle, and workflow state.
 *
 * Tests cover:
 *   - Workflow execution with empty/null inputs
 *   - Step lifecycle through mock handlers (success, failure, skip, abort, defer-reboot)
 *   - Workflow state transitions (IDLE → RUNNING → COMPLETE/FAILED/CANCELLED)
 *   - Multi-step DAG workflows with mock handlers
 *   - MiniManifest/file reference patterns
 *   - Progress and completion callbacks
 *   - WorkflowPersist round-trip
 */

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <vector>

extern "C"
{
#include "aduc/workflow_engine.h"
#include "aduc/dag_engine.h"
#include "aduc/extension_loader.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"
#include "aduc/step_handler_vtable.h"
#include "aduc/step_result.h"
#include "aduc/workflow_persist.h"
}

// ═══════════════════════════════════════════════════════════════════════════════
// Mock Step Handler Infrastructure
// ═══════════════════════════════════════════════════════════════════════════════

// Internal registry layout (mirrors extension_loader.c for test injection)
struct TestLoadedExtension
{
    void* libHandle;
    const ADUC_ExtensionDescriptor* descriptor;
    bool initialized;
};

struct TestExtensionRegistry
{
    TestLoadedExtension extensions[64];
    size_t count;
};

// Per-step mock state
struct MockStepState
{
    int evaluateCalls;
    int acquireCalls;
    int preprocessCalls;
    int executeCalls;
    int validateCalls;
    int postprocessCalls;
    int reportCalls;
    int signalCalls;
    int cancelCalls;
    int releaseCalls;
    bool postprocessRollback;
    ADUC_Result2 evaluateResult;
    ADUC_Result2 acquireResult;
    ADUC_Result2 preprocessResult;
    ADUC_Result2 executeResult;
    ADUC_Result2 validateResult;
    ADUC_StepResult getResultValue;
};

// Global mock configuration
static MockStepState g_mockState;
static ADUC_Result2 g_failAtPhase;  // Which phase should fail (0 = none)
static ADUC_OrchestratorSignal g_signalOnSuccess;

static void reset_mock()
{
    memset(&g_mockState, 0, sizeof(g_mockState));
    g_mockState.evaluateResult = ADUC_RESULT2_SUCCESS;
    g_mockState.acquireResult = ADUC_RESULT2_SUCCESS;
    g_mockState.preprocessResult = ADUC_RESULT2_SUCCESS;
    g_mockState.executeResult = ADUC_RESULT2_SUCCESS;
    g_mockState.validateResult = ADUC_RESULT2_SUCCESS;
    memset(&g_mockState.getResultValue, 0, sizeof(g_mockState.getResultValue));
    g_mockState.getResultValue.result = ADUC_RESULT2_SUCCESS;
    g_mockState.getResultValue.signal = ADUC_SIGNAL_CONTINUE;
    g_failAtPhase = ADUC_RESULT2_SUCCESS;
    g_signalOnSuccess = ADUC_SIGNAL_CONTINUE;
}

static ADUC_Result2 mock_evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    (void)ctx;
    g_mockState.evaluateCalls++;
    *handle = (ADUC_StepHandle)&g_mockState;
    return g_mockState.evaluateResult;
}

static ADUC_Result2 mock_acquire(ADUC_StepHandle handle) { (void)handle; g_mockState.acquireCalls++; return g_mockState.acquireResult; }
static ADUC_Result2 mock_preprocess(ADUC_StepHandle handle) { (void)handle; g_mockState.preprocessCalls++; return g_mockState.preprocessResult; }
static ADUC_Result2 mock_execute(ADUC_StepHandle handle, ADUC_StepProgressFn fn, void* ctx) { (void)handle; (void)fn; (void)ctx; g_mockState.executeCalls++; return g_mockState.executeResult; }
static ADUC_Result2 mock_validate(ADUC_StepHandle handle) { (void)handle; g_mockState.validateCalls++; return g_mockState.validateResult; }
static ADUC_Result2 mock_postprocess(ADUC_StepHandle handle, bool rollback) { (void)handle; g_mockState.postprocessCalls++; g_mockState.postprocessRollback = rollback; return ADUC_RESULT2_SUCCESS; }

static ADUC_StepResult mock_getresult(ADUC_StepHandle handle)
{
    (void)handle;
    return g_mockState.getResultValue;
}

static ADUC_Result2 mock_cancel(ADUC_StepHandle handle) { (void)handle; g_mockState.cancelCalls++; return ADUC_RESULT2_SUCCESS; }
static void mock_release(ADUC_StepHandle handle) { (void)handle; g_mockState.releaseCalls++; }

static ADUC_StepResultDetail mock_report(ADUC_StepHandle handle)
{
    (void)handle;
    g_mockState.reportCalls++;
    return ADUC_StepResult_Success(ADUC_STEP_PHASE_REPORT);
}

static ADUC_StepResultDetail mock_signal(ADUC_StepHandle handle)
{
    (void)handle;
    g_mockState.signalCalls++;
    return ADUC_StepResult_Success(ADUC_STEP_PHASE_SIGNAL);
}

static ADUC_Result2 mock_is_installed(const ADUC_StepContext* ctx, bool* out)
{
    (void)ctx;
    *out = false;
    return ADUC_RESULT2_SUCCESS;
}

static const char* mock_caps_arr[] = { "mock/handler:1", nullptr };
static const char** mock_get_caps() { return mock_caps_arr; }

static ADUC_StepHandlerVtable g_mockVtable = {
    1,                  // structVersion
    mock_evaluate,
    mock_acquire,
    mock_preprocess,
    mock_execute,
    mock_validate,
    mock_postprocess,
    mock_getresult,
    mock_cancel,
    mock_release,
    mock_report,
    mock_signal,
    mock_is_installed,
    mock_get_caps,
};

static const char* g_mockCaps[] = { "mock/handler:1", nullptr };

static ADUC_ExtensionDescriptor g_mockDescriptor = {
    1,                              // structVersion
    "com.test.mock-handler",        // id
    "Mock Step Handler",            // name
    "1.0.0",                        // version
    ADUC_EXT_TYPE_STEP_HANDLER,     // type
    1,                              // minHostApiVersion
    nullptr,                        // Initialize (function pointer)
    nullptr,                        // Uninitialize (function pointer)
    (const void*)&g_mockVtable,     // vtable
    g_mockCaps,                     // capabilities
};

// Inject mock handler into registry
static void inject_mock_handler(ADUC_ExtensionRegistryHandle registry)
{
    auto* reg = reinterpret_cast<TestExtensionRegistry*>(registry);
    reg->extensions[reg->count].libHandle = nullptr;
    reg->extensions[reg->count].descriptor = &g_mockDescriptor;
    reg->extensions[reg->count].initialized = true;
    reg->count++;
}

// Helper: create a single-step deployment
static void make_single_step_deployment(
    ADUC_Deployment& deployment,
    ADUC_DeploymentStep& step,
    const char* wfId = "wf-test",
    const char* handlerType = "mock/handler:1")
{
    memset(&deployment, 0, sizeof(deployment));
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = handlerType;
    deployment.workflowId = wfId;
    deployment.updateId = "test.mock.1.0";
    deployment.steps = &step;
    deployment.stepCount = 1;
}

// RAII guard for workflow handles
struct WorkflowGuard
{
    ADUC_WorkflowEngineHandle handle = nullptr;
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ~WorkflowGuard()
    {
        if (handle) ADUC_Workflow_Destroy(handle);
        if (registry) ADUC_ExtensionRegistry_Destroy(registry);
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Basic Workflow Engine Tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("WorkflowEngine: Execute with empty deployment succeeds", "[workflow_engine]")
{
    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-empty";
    deployment.updateId = "test.empty.1.0.0";

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(g.handle != nullptr);
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
}

TEST_CASE("WorkflowEngine: GetState on NULL handle returns IDLE", "[workflow_engine]")
{
    CHECK(ADUC_Workflow_GetState(nullptr) == ADUC_WF_STATE_IDLE);
}

TEST_CASE("WorkflowEngine: Destroy NULL handle is safe", "[workflow_engine]")
{
    ADUC_Workflow_Destroy(nullptr);
}

TEST_CASE("WorkflowEngine: Cancel NULL handle is safe", "[workflow_engine]")
{
    ADUC_Workflow_Cancel(nullptr);
}

TEST_CASE("WorkflowEngine: Execute with NULL deployment fails", "[workflow_engine]")
{
    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_Workflow_Execute(nullptr, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("WorkflowEngine: Execute with NULL registry fails", "[workflow_engine]")
{
    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-test";

    ADUC_WorkflowEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Workflow_Execute(&deployment, nullptr, nullptr, nullptr, nullptr, &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("WorkflowEngine: Execute with NULL outHandle fails", "[workflow_engine]")
{
    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-test";

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Completion Callback Tests
// ═══════════════════════════════════════════════════════════════════════════════

static bool s_completionCalled = false;
static ADUC_Result2 s_completionResult;
static size_t s_completionStepCount = 0;

static void test_completion_callback(
    const char* workflowId,
    ADUC_Result2 overallResult,
    const ADUC_StepResult* stepResults,
    size_t stepCount,
    void* ctx)
{
    (void)workflowId;
    (void)stepResults;
    (void)ctx;
    s_completionCalled = true;
    s_completionResult = overallResult;
    s_completionStepCount = stepCount;
}

TEST_CASE("WorkflowEngine: Completion callback is invoked", "[workflow_engine]")
{
    s_completionCalled = false;
    s_completionResult.code = 0xFFFFFFFF;

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-callback";
    deployment.updateId = "test.callback.1.0";

    result = ADUC_Workflow_Execute(
        &deployment, g.registry, nullptr, test_completion_callback, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(s_completionCalled == true);
    CHECK(ADUC_RESULT2_IS_SUCCESS(s_completionResult));
}

TEST_CASE("WorkflowEngine: Step with no handler fails gracefully", "[workflow_engine]")
{
    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_DeploymentStep step;
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = "nonexistent/handler:1";

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-no-handler";
    deployment.updateId = "test.nohandler.1.0";
    deployment.steps = &step;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    REQUIRE(g.handle != nullptr);
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Step Lifecycle Tests (with mock handlers)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("StepLifecycle: Step succeeds through all phases", "[workflow_engine][step_lifecycle]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(g.handle != nullptr);
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);

    // Verify all lifecycle phases were called
    CHECK(g_mockState.evaluateCalls == 1);
    CHECK(g_mockState.acquireCalls == 1);
    CHECK(g_mockState.preprocessCalls == 1);
    CHECK(g_mockState.executeCalls == 1);
    CHECK(g_mockState.validateCalls == 1);
    CHECK(g_mockState.postprocessCalls == 1);
    CHECK(g_mockState.releaseCalls == 1);
    // Postprocess with rollback=false on success
    CHECK(g_mockState.postprocessRollback == false);
}

TEST_CASE("StepLifecycle: Step fails at EXECUTE — transitions to POSTPROCESS with rollback", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    g_mockState.executeResult = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);

    // Evaluate, Acquire, Preprocess, Execute all called
    CHECK(g_mockState.evaluateCalls == 1);
    CHECK(g_mockState.acquireCalls == 1);
    CHECK(g_mockState.preprocessCalls == 1);
    CHECK(g_mockState.executeCalls == 1);
    // Validate NOT called (failure before it)
    CHECK(g_mockState.validateCalls == 0);
    // Postprocess called with rollback=true
    CHECK(g_mockState.postprocessCalls == 1);
    CHECK(g_mockState.postprocessRollback == true);
    CHECK(g_mockState.releaseCalls == 1);
}

TEST_CASE("StepLifecycle: Step fails at ACQUIRE — rollback triggered", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    g_mockState.acquireResult = ADUC_RESULT2_MAKE(0x02, 0x03, 0x0001);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);

    CHECK(g_mockState.evaluateCalls == 1);
    CHECK(g_mockState.acquireCalls == 1);
    CHECK(g_mockState.preprocessCalls == 0);
    CHECK(g_mockState.executeCalls == 0);
    CHECK(g_mockState.validateCalls == 0);
    CHECK(g_mockState.postprocessCalls == 1);
    CHECK(g_mockState.postprocessRollback == true);
}

TEST_CASE("StepLifecycle: Step fails at EVALUATE — skip step, signal continue", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    g_mockState.evaluateResult = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0001);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    // Evaluate failure with CONTINUE signal → step skipped, workflow overall fails
    // because the step result itself is a failure
    REQUIRE(g.handle != nullptr);

    CHECK(g_mockState.evaluateCalls == 1);
    // No further phases called after evaluate failure
    CHECK(g_mockState.acquireCalls == 0);
    CHECK(g_mockState.preprocessCalls == 0);
    CHECK(g_mockState.executeCalls == 0);
}

TEST_CASE("StepLifecycle: Step fails at VALIDATE — rollback triggered", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    g_mockState.validateResult = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    CHECK(g_mockState.evaluateCalls == 1);
    CHECK(g_mockState.acquireCalls == 1);
    CHECK(g_mockState.preprocessCalls == 1);
    CHECK(g_mockState.executeCalls == 1);
    CHECK(g_mockState.validateCalls == 1);
    CHECK(g_mockState.postprocessCalls == 1);
    CHECK(g_mockState.postprocessRollback == true);
}

TEST_CASE("StepLifecycle: Step signals ABORT — stops remaining steps", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    // Configure mock to return ABORT signal with a failure result
    g_mockState.getResultValue.signal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
    g_mockState.getResultValue.result = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // Two-step deployment: step_0 aborts, step_1 should not run
    ADUC_DeploymentStep steps[2];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "step_0";
    steps[0].handlerType = "mock/handler:1";
    steps[1].stepId = "step_1";
    steps[1].handlerType = "mock/handler:1";

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-abort";
    deployment.updateId = "test.abort.1.0";
    deployment.steps = steps;
    deployment.stepCount = 2;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    REQUIRE(g.handle != nullptr);
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);

    // Only one step should have been evaluated (the first one)
    CHECK(g_mockState.evaluateCalls == 1);
}

TEST_CASE("StepLifecycle: Step signals DEFER_REBOOT — recorded correctly", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    g_mockState.getResultValue.signal = ADUC_SIGNAL_DEFER_REBOOT;

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
    // All phases completed
    CHECK(g_mockState.postprocessCalls == 1);
    CHECK(g_mockState.postprocessRollback == false);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Progress Callback Tests
// ═══════════════════════════════════════════════════════════════════════════════

static std::vector<std::string> s_progressPhases;
static std::vector<uint32_t> s_progressPercents;

static void test_progress_callback(
    const char* workflowId,
    const char* stepId,
    const char* phase,
    uint32_t percentComplete,
    void* ctx)
{
    (void)workflowId;
    (void)stepId;
    (void)ctx;
    s_progressPhases.push_back(phase);
    s_progressPercents.push_back(percentComplete);
}

TEST_CASE("StepLifecycle: Progress tracking reports all phases", "[workflow_engine][step_lifecycle]")
{
    reset_mock();
    s_progressPhases.clear();
    s_progressPercents.clear();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(
        &deployment, g.registry, test_progress_callback, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Should have progress reports for each lifecycle phase
    REQUIRE(s_progressPhases.size() >= 6);
    CHECK(s_progressPhases[0] == "Evaluate");
    CHECK(s_progressPhases[1] == "Acquire");
    CHECK(s_progressPhases[2] == "Preprocess");
    CHECK(s_progressPhases[3] == "Execute");
    CHECK(s_progressPhases[4] == "Validate");
    CHECK(s_progressPhases[5] == "Postprocess");

    // Percent should be monotonically non-decreasing
    for (size_t i = 1; i < s_progressPercents.size(); i++)
    {
        CHECK(s_progressPercents[i] >= s_progressPercents[i - 1]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Workflow State Transition Tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("WorkflowState: Starts in IDLE (NULL handle)", "[workflow_engine][workflow_state]")
{
    CHECK(ADUC_Workflow_GetState(nullptr) == ADUC_WF_STATE_IDLE);
}

TEST_CASE("WorkflowState: Transitions to COMPLETE on success", "[workflow_engine][workflow_state]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
}

TEST_CASE("WorkflowState: Transitions to FAILED on step failure", "[workflow_engine][workflow_state]")
{
    reset_mock();
    g_mockState.executeResult = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);
}

TEST_CASE("WorkflowState: Transitions to COMPLETE with 0-step deployment", "[workflow_engine][workflow_state]")
{
    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-empty-state";

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
}

TEST_CASE("WorkflowState: Reports correct result on handler-not-found failure", "[workflow_engine][workflow_state]")
{
    s_completionCalled = false;

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_DeploymentStep step;
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = "missing/handler:1";

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-missing";
    deployment.steps = &step;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(
        &deployment, g.registry, nullptr, test_completion_callback, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);
    CHECK(s_completionCalled == true);
    CHECK(ADUC_RESULT2_IS_FAILURE(s_completionResult));
}

TEST_CASE("WorkflowState: Completion callback reports step count", "[workflow_engine][workflow_state]")
{
    reset_mock();
    s_completionCalled = false;
    s_completionStepCount = 999;

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step);

    result = ADUC_Workflow_Execute(
        &deployment, g.registry, nullptr, test_completion_callback, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(s_completionCalled);
    CHECK(s_completionStepCount == 1);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Integration-Style Tests: Full Workflow with Mock Handlers
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("Integration: Full workflow single step success", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_Deployment deployment;
    ADUC_DeploymentStep step;
    make_single_step_deployment(deployment, step, "wf-integration-1");

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
    CHECK(g_mockState.evaluateCalls == 1);
    CHECK(g_mockState.releaseCalls == 1);
}

TEST_CASE("Integration: Full workflow multi-step linear DAG all succeed", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // A → B → C (linear chain via requires)
    const char* bReqs[] = { "step_a" };
    const char* cReqs[] = { "step_b" };

    ADUC_DeploymentStep steps[3];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "step_a";
    steps[0].handlerType = "mock/handler:1";
    steps[1].stepId = "step_b";
    steps[1].handlerType = "mock/handler:1";
    steps[1].requires = bReqs;
    steps[1].requiresCount = 1;
    steps[2].stepId = "step_c";
    steps[2].handlerType = "mock/handler:1";
    steps[2].requires = cReqs;
    steps[2].requiresCount = 1;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-multi-linear";
    deployment.updateId = "test.multi.1.0";
    deployment.steps = steps;
    deployment.stepCount = 3;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);

    // All 3 steps executed
    CHECK(g_mockState.evaluateCalls == 3);
    CHECK(g_mockState.executeCalls == 3);
    CHECK(g_mockState.releaseCalls == 3);
}

TEST_CASE("Integration: Full workflow multi-step middle step fails", "[workflow_engine][integration]")
{
    // We need per-step control. Use a counter to fail the second step.
    reset_mock();

    // Fail on 2nd execute call
    static int s_executeCallCount = 0;
    s_executeCallCount = 0;
    static auto original_execute = g_mockVtable.Execute;
    g_mockVtable.Execute = [](ADUC_StepHandle handle, ADUC_StepProgressFn fn, void* ctx) -> ADUC_Result2 {
        (void)handle; (void)fn; (void)ctx;
        g_mockState.executeCalls++;
        s_executeCallCount++;
        if (s_executeCallCount == 2)
        {
            return ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);
        }
        return ADUC_RESULT2_SUCCESS;
    };

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    const char* bReqs[] = { "step_a" };
    const char* cReqs[] = { "step_b" };

    ADUC_DeploymentStep steps[3];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "step_a";
    steps[0].handlerType = "mock/handler:1";
    steps[1].stepId = "step_b";
    steps[1].handlerType = "mock/handler:1";
    steps[1].requires = bReqs;
    steps[1].requiresCount = 1;
    steps[2].stepId = "step_c";
    steps[2].handlerType = "mock/handler:1";
    steps[2].requires = cReqs;
    steps[2].requiresCount = 1;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-middle-fail";
    deployment.updateId = "test.midfail.1.0";
    deployment.steps = steps;
    deployment.stepCount = 3;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_FAILED);

    // step_a succeeded (execute called once), step_b failed (execute #2), step_c never ran
    CHECK(g_mockState.executeCalls == 2);

    // Restore original
    g_mockVtable.Execute = original_execute;
}

TEST_CASE("Integration: Workflow with cycle in DAG returns error", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // A→B→A cycle
    const char* aReqs[] = { "step_b" };
    const char* bReqs[] = { "step_a" };

    ADUC_DeploymentStep steps[2];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "step_a";
    steps[0].handlerType = "mock/handler:1";
    steps[0].requires = aReqs;
    steps[0].requiresCount = 1;
    steps[1].stepId = "step_b";
    steps[1].handlerType = "mock/handler:1";
    steps[1].requires = bReqs;
    steps[1].requiresCount = 1;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-cycle";
    deployment.steps = steps;
    deployment.stepCount = 2;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    // No steps should have been executed
    CHECK(g_mockState.evaluateCalls == 0);
}

TEST_CASE("Integration: Workflow with missing dependency reference", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    const char* badReqs[] = { "nonexistent_step" };

    ADUC_DeploymentStep steps[1];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "step_a";
    steps[0].handlerType = "mock/handler:1";
    steps[0].requires = badReqs;
    steps[0].requiresCount = 1;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-bad-dep";
    deployment.steps = steps;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(g_mockState.evaluateCalls == 0);
}

TEST_CASE("Integration: Workflow with diamond DAG succeeds", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // A → B, A → C, B+C → D
    const char* bReqs[] = { "A" };
    const char* cReqs[] = { "A" };
    const char* dReqs[] = { "B", "C" };

    ADUC_DeploymentStep steps[4];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "A";
    steps[0].handlerType = "mock/handler:1";
    steps[1].stepId = "B";
    steps[1].handlerType = "mock/handler:1";
    steps[1].requires = bReqs;
    steps[1].requiresCount = 1;
    steps[2].stepId = "C";
    steps[2].handlerType = "mock/handler:1";
    steps[2].requires = cReqs;
    steps[2].requiresCount = 1;
    steps[3].stepId = "D";
    steps[3].handlerType = "mock/handler:1";
    steps[3].requires = dReqs;
    steps[3].requiresCount = 2;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-diamond";
    deployment.updateId = "test.diamond.1.0";
    deployment.steps = steps;
    deployment.stepCount = 4;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
    CHECK(g_mockState.evaluateCalls == 4);
    CHECK(g_mockState.executeCalls == 4);
}

TEST_CASE("Integration: Workflow with fan-out succeeds", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // A → B, A → C, A → D, A → E
    const char* rootReq[] = { "root" };

    ADUC_DeploymentStep steps[5];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "root";
    steps[0].handlerType = "mock/handler:1";
    for (int i = 1; i < 5; i++)
    {
        static const char* ids[] = { nullptr, "fan1", "fan2", "fan3", "fan4" };
        steps[i].stepId = ids[i];
        steps[i].handlerType = "mock/handler:1";
        steps[i].requires = rootReq;
        steps[i].requiresCount = 1;
    }

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-fanout";
    deployment.steps = steps;
    deployment.stepCount = 5;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
    CHECK(g_mockState.evaluateCalls == 5);
}

TEST_CASE("Integration: Workflow with fan-in succeeds", "[workflow_engine][integration]")
{
    reset_mock();

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    // A, B, C → D
    const char* dReqs[] = { "A", "B", "C" };

    ADUC_DeploymentStep steps[4];
    memset(steps, 0, sizeof(steps));
    steps[0].stepId = "A";
    steps[0].handlerType = "mock/handler:1";
    steps[1].stepId = "B";
    steps[1].handlerType = "mock/handler:1";
    steps[2].stepId = "C";
    steps[2].handlerType = "mock/handler:1";
    steps[3].stepId = "D";
    steps[3].handlerType = "mock/handler:1";
    steps[3].requires = dReqs;
    steps[3].requiresCount = 3;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-fanin";
    deployment.steps = steps;
    deployment.stepCount = 4;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_Workflow_GetState(g.handle) == ADUC_WF_STATE_COMPLETE);
    CHECK(g_mockState.evaluateCalls == 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MiniManifest / File Reference Pattern Tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("MiniManifest: Step with file references — passed in context", "[workflow_engine][minimanifest]")
{
    reset_mock();

    // Track what files the step handler sees
    static size_t s_seenFileCount = 0;
    static const char* s_seenFileName = nullptr;
    g_mockVtable.Evaluate = [](const ADUC_StepContext* ctx, ADUC_StepHandle* handle) -> ADUC_Result2 {
        g_mockState.evaluateCalls++;
        s_seenFileCount = ctx->fileCount;
        if (ctx->fileCount > 0 && ctx->files != nullptr)
        {
            s_seenFileName = ctx->files[0].name;
        }
        *handle = (ADUC_StepHandle)&g_mockState;
        return ADUC_RESULT2_SUCCESS;
    };

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_StepFile files[2];
    memset(files, 0, sizeof(files));
    files[0].id = "file-001";
    files[0].name = "firmware.bin";
    files[0].path = "/downloads/firmware.bin";
    files[0].size = 1024000;
    files[0].sha256 = "abc123def456";
    files[1].id = "file-002";
    files[1].name = "config.json";
    files[1].path = "/downloads/config.json";
    files[1].size = 256;
    files[1].sha256 = "789xyz";

    ADUC_DeploymentStep step;
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = "mock/handler:1";
    step.files = files;
    step.fileCount = 2;

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-files";
    deployment.steps = &step;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(s_seenFileCount == 2);
    CHECK(std::string(s_seenFileName) == "firmware.bin");

    // Restore original
    g_mockVtable.Evaluate = mock_evaluate;
}

TEST_CASE("MiniManifest: Step with handler config JSON — passed through", "[workflow_engine][minimanifest]")
{
    reset_mock();

    static const char* s_seenConfigJson = nullptr;
    g_mockVtable.Evaluate = [](const ADUC_StepContext* ctx, ADUC_StepHandle* handle) -> ADUC_Result2 {
        g_mockState.evaluateCalls++;
        s_seenConfigJson = ctx->handlerConfigJson;
        *handle = (ADUC_StepHandle)&g_mockState;
        return ADUC_RESULT2_SUCCESS;
    };

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_DeploymentStep step;
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = "mock/handler:1";
    step.handlerConfigJson = R"({"rebootRequired":true,"timeout":300})";

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-config";
    deployment.steps = &step;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(s_seenConfigJson != nullptr);
    CHECK(std::string(s_seenConfigJson).find("rebootRequired") != std::string::npos);

    g_mockVtable.Evaluate = mock_evaluate;
}

TEST_CASE("MiniManifest: Step with installed criteria — passed through", "[workflow_engine][minimanifest]")
{
    reset_mock();

    static const char* s_seenCriteria = nullptr;
    g_mockVtable.Evaluate = [](const ADUC_StepContext* ctx, ADUC_StepHandle* handle) -> ADUC_Result2 {
        g_mockState.evaluateCalls++;
        s_seenCriteria = ctx->installedCriteria;
        *handle = (ADUC_StepHandle)&g_mockState;
        return ADUC_RESULT2_SUCCESS;
    };

    WorkflowGuard g;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&g.registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    inject_mock_handler(g.registry);

    ADUC_DeploymentStep step;
    memset(&step, 0, sizeof(step));
    step.stepId = "step_0";
    step.handlerType = "mock/handler:1";
    step.installedCriteria = "fw_version>=2.0.0";

    ADUC_Deployment deployment;
    memset(&deployment, 0, sizeof(deployment));
    deployment.workflowId = "wf-criteria";
    deployment.steps = &step;
    deployment.stepCount = 1;

    result = ADUC_Workflow_Execute(&deployment, g.registry, nullptr, nullptr, nullptr, &g.handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(s_seenCriteria != nullptr);
    CHECK(std::string(s_seenCriteria) == "fw_version>=2.0.0");

    g_mockVtable.Evaluate = mock_evaluate;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Step Result Helper Tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("StepResult: Success helper creates valid result", "[step_result]")
{
    ADUC_StepResultDetail r = ADUC_StepResult_Success(ADUC_STEP_PHASE_EXECUTE);
    CHECK(ADUC_StepResult_IsSuccess(&r));
    CHECK(r.phase == ADUC_STEP_PHASE_EXECUTE);
    CHECK(r.signal == ADUC_SIGNAL_CONTINUE);
    CHECK_FALSE(ADUC_StepResult_ShouldStop(&r));
    CHECK(r.startTimeMs > 0);
    ADUC_StepResult_Free(&r);
}

TEST_CASE("StepResult: SuccessWithSignal sets signal correctly", "[step_result]")
{
    ADUC_StepResultDetail r = ADUC_StepResult_SuccessWithSignal(
        ADUC_STEP_PHASE_EXECUTE, ADUC_SIGNAL_DEFER_REBOOT);
    CHECK(ADUC_StepResult_IsSuccess(&r));
    CHECK(r.signal == ADUC_SIGNAL_DEFER_REBOOT);
    CHECK_FALSE(ADUC_StepResult_ShouldStop(&r));
    ADUC_StepResult_Free(&r);
}

TEST_CASE("StepResult: Failure helper sets error fields", "[step_result]")
{
    ADUC_Result2 errCode = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0006);
    ADUC_StepResultDetail r = ADUC_StepResult_Failure(
        ADUC_STEP_PHASE_EXECUTE, errCode, "Execute failed", "test_handler");
    CHECK_FALSE(ADUC_StepResult_IsSuccess(&r));
    CHECK(r.phase == ADUC_STEP_PHASE_EXECUTE);
    CHECK(ADUC_StepResult_ShouldStop(&r));
    CHECK(r.outcome == ADUC_Outcome_Failed);
    CHECK(r.origin == ADUC_Origin_AgentCore);
    CHECK(std::string(r.resultDetails) == "Execute failed");
    CHECK(std::string(r.errorSource) == "test_handler");
    ADUC_StepResult_Free(&r);
    // After free, fields should be zeroed
    CHECK(r.resultDetails == nullptr);
}

TEST_CASE("StepResult: Abort helper sets ABORT signal", "[step_result]")
{
    ADUC_Result2 errCode = ADUC_RESULT2_MAKE(0x03, 0x06, 0x0001);
    ADUC_StepResultDetail r = ADUC_StepResult_Abort(
        ADUC_STEP_PHASE_PREPROCESS, errCode, "Abort requested");
    CHECK_FALSE(ADUC_StepResult_IsSuccess(&r));
    CHECK(ADUC_StepResult_ShouldStop(&r));
    CHECK(r.signal == ADUC_SIGNAL_ABORT_DEPLOYMENT);
    ADUC_StepResult_Free(&r);
}

TEST_CASE("StepResult: Retry helper sets retry metadata", "[step_result]")
{
    ADUC_StepResultDetail r = ADUC_StepResult_Retry(
        ADUC_STEP_PHASE_EXECUTE, 2, 5, 1000);
    CHECK(ADUC_StepResult_IsSuccess(&r));
    CHECK(r.signal == ADUC_SIGNAL_RETRY_PHASE);
    CHECK(r.retryCount == 2);
    CHECK(r.maxRetries == 5);
    CHECK(r.retryDelayMs == 1000);
    ADUC_StepResult_Free(&r);
}

TEST_CASE("StepResult: IsSuccess with NULL returns false", "[step_result]")
{
    CHECK_FALSE(ADUC_StepResult_IsSuccess(nullptr));
}

TEST_CASE("StepResult: ShouldStop with NULL returns false", "[step_result]")
{
    CHECK_FALSE(ADUC_StepResult_ShouldStop(nullptr));
}

TEST_CASE("StepResult: ElapsedMs computes correctly", "[step_result]")
{
    ADUC_StepResultDetail r;
    memset(&r, 0, sizeof(r));
    r.startTimeMs = 1000;
    r.endTimeMs = 1500;
    CHECK(ADUC_StepResult_ElapsedMs(&r) == 500);
}

TEST_CASE("StepResult: ElapsedMs with NULL returns 0", "[step_result]")
{
    CHECK(ADUC_StepResult_ElapsedMs(nullptr) == 0);
}

TEST_CASE("StepResult: Free with NULL is safe", "[step_result]")
{
    ADUC_StepResult_Free(nullptr); // Should not crash
}

TEST_CASE("StepResult: Phase ToString covers all phases", "[step_result]")
{
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_EVALUATE)) == "Evaluate");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_ACQUIRE)) == "Acquire");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_PREPROCESS)) == "Preprocess");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_EXECUTE)) == "Execute");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_VALIDATE)) == "Validate");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_POSTPROCESS)) == "Postprocess");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_REPORT)) == "Report");
    CHECK(std::string(ADUC_StepPhase_ToString(ADUC_STEP_PHASE_SIGNAL)) == "Signal");
    CHECK(std::string(ADUC_StepPhase_ToString((ADUC_StepPhase)99)) == "Unknown");
}

TEST_CASE("StepResult: Signal ToString covers all signals", "[step_result]")
{
    CHECK(std::string(ADUC_OrchestratorSignal_ToString(ADUC_SIGNAL_CONTINUE)) == "Continue");
    CHECK(std::string(ADUC_OrchestratorSignal_ToString(ADUC_SIGNAL_ABORT_DEPLOYMENT)) == "AbortDeployment");
    CHECK(std::string(ADUC_OrchestratorSignal_ToString(ADUC_SIGNAL_DEFER_REBOOT)) == "DeferReboot");
    CHECK(std::string(ADUC_OrchestratorSignal_ToString(ADUC_SIGNAL_IMMEDIATE_REBOOT)) == "ImmediateReboot");
    CHECK(std::string(ADUC_OrchestratorSignal_ToString(ADUC_SIGNAL_ROLLBACK)) == "Rollback");
    CHECK(std::string(ADUC_OrchestratorSignal_ToString((ADUC_OrchestratorSignal)99)) == "Unknown");
}

// ═══════════════════════════════════════════════════════════════════════════════
// DAG Engine Tests (kept from original — additional ones in dag_engine_ut.cpp)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("DagEngine: Linear chain A→B→C executes in order", "[dag_engine]")
{
    const char* bDeps[] = {"A", NULL};
    const char* cDeps[] = {"B", NULL};

    ADUC_DagNodeDef nodes[3] = {
        {"A", NULL, 0},
        {"B", bDeps, 1},
        {"C", cDeps, 1},
    };

    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(handle != nullptr);
    REQUIRE_FALSE(ADUC_DagEngine_HasCycle(handle));

    const char* ready[3] = {};
    size_t count = ADUC_DagEngine_GetReady(handle, ready, 3);
    REQUIRE(count == 1);
    CHECK(std::string(ready[0]) == "A");

    ADUC_DagEngine_MarkDone(handle, "A");
    count = ADUC_DagEngine_GetReady(handle, ready, 3);
    REQUIRE(count == 1);
    CHECK(std::string(ready[0]) == "B");

    ADUC_DagEngine_MarkDone(handle, "B");
    count = ADUC_DagEngine_GetReady(handle, ready, 3);
    REQUIRE(count == 1);
    CHECK(std::string(ready[0]) == "C");

    ADUC_DagEngine_MarkDone(handle, "C");
    CHECK(ADUC_DagEngine_IsComplete(handle));

    ADUC_DagEngine_Destroy(handle);
}

TEST_CASE("DagEngine: Parallel nodes A,B independent, C depends on both", "[dag_engine]")
{
    const char* cDeps[] = {"A", "B", NULL};

    ADUC_DagNodeDef nodes[3] = {
        {"A", NULL, 0},
        {"B", NULL, 0},
        {"C", cDeps, 2},
    };

    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const char* ready[3] = {};
    size_t count = ADUC_DagEngine_GetReady(handle, ready, 3);
    REQUIRE(count == 2);

    ADUC_DagEngine_MarkDone(handle, "A");
    ADUC_DagEngine_MarkDone(handle, "B");
    count = ADUC_DagEngine_GetReady(handle, ready, 3);
    REQUIRE(count == 1);
    CHECK(std::string(ready[0]) == "C");

    ADUC_DagEngine_MarkDone(handle, "C");
    CHECK(ADUC_DagEngine_IsComplete(handle));

    ADUC_DagEngine_Destroy(handle);
}

TEST_CASE("DagEngine: Cycle detection", "[dag_engine]")
{
    const char* aDeps[] = {"C", NULL};
    const char* bDeps[] = {"A", NULL};
    const char* cDeps[] = {"B", NULL};

    ADUC_DagNodeDef nodes[3] = {
        {"A", aDeps, 1},
        {"B", bDeps, 1},
        {"C", cDeps, 1},
    };

    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 3, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_DagEngine_HasCycle(handle));
    ADUC_DagEngine_Destroy(handle);
}

TEST_CASE("DagEngine: Mark failed blocks dependents", "[dag_engine]")
{
    const char* bDeps[] = {"A", NULL};

    ADUC_DagNodeDef nodes[2] = {
        {"A", NULL, 0},
        {"B", bDeps, 1},
    };

    ADUC_DagEngineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_DagEngine_Create(nodes, 2, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const char* ready[2] = {};
    size_t count = ADUC_DagEngine_GetReady(handle, ready, 2);
    REQUIRE(count == 1);
    CHECK(std::string(ready[0]) == "A");

    ADUC_DagEngine_MarkFailed(handle, "A");
    count = ADUC_DagEngine_GetReady(handle, ready, 2);
    CHECK(count == 0);
    CHECK(ADUC_DagEngine_IsComplete(handle));

    size_t pending = 0, running = 0, done = 0, failed = 0;
    ADUC_DagEngine_GetStats(handle, &pending, &running, &done, &failed, NULL);
    CHECK(failed == 2);

    ADUC_DagEngine_Destroy(handle);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Workflow Persist Tests
// ═══════════════════════════════════════════════════════════════════════════════

TEST_CASE("WorkflowPersist: Save and load round-trip", "[workflow_persist]")
{
    const char* testDir = ".";

    ADUC_WorkflowState2 state;
    memset(&state, 0, sizeof(state));
    strncpy(state.deploymentId, "deploy-123", sizeof(state.deploymentId) - 1);
    strncpy(state.workflowId, "wf-456", sizeof(state.workflowId) - 1);
    state.currentStepIndex = 3;
    state.currentPhase = ADUC_STEP_PHASE_EXECUTE;
    state.overallState = ADUC_WF_STATE_RUNNING;
    state.lastUpdateTimeMs = 1700000000000ULL;
    strncpy(state.resumeToken, "token-abc", sizeof(state.resumeToken) - 1);

    ADUC_Result2 result = ADUC_WorkflowPersist_Save(testDir, &state);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_WorkflowState2 loaded;
    result = ADUC_WorkflowPersist_Load(testDir, &loaded);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(std::string(loaded.deploymentId) == "deploy-123");
    CHECK(std::string(loaded.workflowId) == "wf-456");
    CHECK(loaded.currentStepIndex == 3);
    CHECK(loaded.currentPhase == ADUC_STEP_PHASE_EXECUTE);
    CHECK(loaded.overallState == ADUC_WF_STATE_RUNNING);
    CHECK(loaded.lastUpdateTimeMs == 1700000000000ULL);
    CHECK(std::string(loaded.resumeToken) == "token-abc");

    ADUC_WorkflowPersist_Clear(testDir);
}

TEST_CASE("WorkflowPersist: HasState returns false on empty dir", "[workflow_persist]")
{
    const char* emptyDir = "..";
    ADUC_WorkflowPersist_Clear(emptyDir);
    CHECK_FALSE(ADUC_WorkflowPersist_HasState(emptyDir));
}

TEST_CASE("WorkflowPersist: Clear removes state", "[workflow_persist]")
{
    const char* testDir = ".";

    ADUC_WorkflowState2 state;
    memset(&state, 0, sizeof(state));
    strncpy(state.workflowId, "wf-clear-test", sizeof(state.workflowId) - 1);

    ADUC_Result2 result = ADUC_WorkflowPersist_Save(testDir, &state);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_WorkflowPersist_HasState(testDir));

    result = ADUC_WorkflowPersist_Clear(testDir);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK_FALSE(ADUC_WorkflowPersist_HasState(testDir));
}
