/**
 * @file mock_handler_create_deps.cpp
 * @brief Mock dependencies for handler_create.cpp exception-path testing.
 *
 * Provides a controllable StepsHandlerImpl::CreateContentHandler() and
 * stub implementations for all symbols referenced by handler_create.cpp.
 */

#include <aduc/steps_handler.hpp>
#include <stdexcept>

// ===== Controllable mock behavior =====

enum class MockCreateBehavior
{
    ReturnValid,
    ThrowStdException,
    ThrowNonStd
};

static MockCreateBehavior g_mock_create_behavior = MockCreateBehavior::ReturnValid;

void mock_handler_create_set_behavior(MockCreateBehavior behavior)
{
    g_mock_create_behavior = behavior;
}

// ===== StepsHandlerImpl implementation stubs =====

ContentHandler* StepsHandlerImpl::CreateContentHandler()
{
    switch (g_mock_create_behavior)
    {
    case MockCreateBehavior::ThrowStdException:
        throw std::runtime_error("mock std exception from CreateContentHandler");
    case MockCreateBehavior::ThrowNonStd:
        throw 42; // non-std exception
    case MockCreateBehavior::ReturnValid:
    default:
        return new StepsHandlerImpl();
    }
}

StepsHandlerImpl::~StepsHandlerImpl() = default;

ADUC_Result StepsHandlerImpl::Download(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::Backup(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::Install(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::Apply(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::Restore(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::Cancel(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

ADUC_Result StepsHandlerImpl::IsInstalled(const tagADUC_WorkflowData*)
{
    return ADUC_Result{ 0, 0 };
}

// ===== Logging stubs =====

extern "C" {

void ADUC_Logging_Init(int, const char*)
{
}

void ADUC_Logging_Uninit()
{
}

} // extern "C"
