/**
 * @file linux_adu_core_impl.hpp
 * @brief Implements the ADU Core interface functionality for linux platform.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef LINUX_ADU_CORE_IMPL_HPP
#define LINUX_ADU_CORE_IMPL_HPP

#include <atomic>
#include <exception>
#include <thread>

#include <sys/time.h> // for gettimeofday
#include <time.h>

#include "aduc/adu_core_exports.h"
#include "aduc/exception_utils.hpp"
#include "aduc/logging.h"
#include "aduc/result.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_utils.h"

namespace ADUC
{
/**
 * @brief Implementation class for UpdateAction handlers.
 */
class LinuxPlatformLayer
{
public:
    static std::unique_ptr<LinuxPlatformLayer> Create();

    ADUC_Result SetUpdateActionCallbacks(ADUC_UpdateActionCallbacks* data);

private:
    static std::string g_componentsInfo;
    static time_t g_lastComponentsCheckTime;

    //
    // Static callbacks.
    //

    /**
     * @brief Implements Idle callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     * @return ADUC_Result
     */
    static void IdleCallback(ADUC_Token token, const char* workflowId) noexcept
    {
        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [&token, &workflowId]() -> void { static_cast<LinuxPlatformLayer*>(token)->Idle(workflowId); });
    }

    /**
     * @brief Implements Download callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info ADUC_WorkflowDataToken with information on how to download.
     * @return ADUC_Result
     */
    static ADUC_Result DownloadCallback(
        ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);

        try
        {
            Log_Info("Download thread started.");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Download(workflowData);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(
                    workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_Result_Download_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_Result_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        };
    }

    /**
     * @brief Implements Backup callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info #ADUC_WorkflowData with information on how to backup.
     * @return ADUC_Result
     */
    static ADUC_Result BackupCallback(
        ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken info) noexcept
    {
        ADUC_Result result;
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Backup thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Backup(workflowData);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(
                    workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            result = { ADUC_Result_Backup_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            result = { ADUC_Result_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            result = ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            result = ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }

        return result;
    }

    /**
     * @brief Implements Install callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info #ADUC_WorkflowData with information on how to install.
     * @return ADUC_Result
     */
    static ADUC_Result InstallCallback(
        ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken info) noexcept
    {
        ADUC_Result result;
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Install thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Install(workflowData);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(
                    workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            result = { ADUC_Result_Install_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            result = { ADUC_Result_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            result = ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            result = ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }

        return result;
    }

    /**
     * @brief Implements Apply callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info #ADUC_WorkflowData with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result ApplyCallback(
        ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Apply thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Apply(workflowData);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(
                    workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_Result_Apply_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_Result_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    /**
     * @brief Implements Restore callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info #ADUC_WorkflowData with information on how to restore.
     * @return ADUC_Result
     */
    static ADUC_Result RestoreCallback(
        ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Restore thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Restore(workflowData);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(
                    workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_Result_Restore_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_Result_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    /**
     * @brief Implements Cancel callback.
     *
     * @param token Opaque token.
     * @param info #ADUC_WorkflowData to cancel.
     */
    static void CancelCallback(ADUC_Token token, ADUC_WorkflowDataToken info) noexcept
    {
        Log_Info("CancelCallback called");
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);

        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [&token, &workflowData]() -> void { static_cast<LinuxPlatformLayer*>(token)->Cancel(workflowData); });
    }

    static ADUC_Result IsInstalledCallback(ADUC_Token token, ADUC_WorkflowDataToken info) noexcept
    {
        Log_Info("IsInstalledCallback called");
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);

        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_Result_Failure, [&token, &workflowData]() -> ADUC_Result {
                return static_cast<LinuxPlatformLayer*>(token)->IsInstalled(workflowData);
            });
    }

    /**
     * @brief Implements SandboxCreate callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workflowId Unique workflow identifier.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     *
     * @return ADUC_Result
     */
    static ADUC_Result SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder) noexcept
    {
        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_Result_Failure, [&token, &workflowId, &workFolder]() -> ADUC_Result {
                return static_cast<LinuxPlatformLayer*>(token)->SandboxCreate(workflowId, workFolder);
            });
    }

    /**
     * @brief Implements SandboxDestroy callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workflowId Unique workflow identifier.
     * @param workFolder[in] Sandbox path.
     */
    static void SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder) noexcept
    {
        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions([&token, &workflowId, &workFolder]() -> void {
            static_cast<LinuxPlatformLayer*>(token)->SandboxDestroy(workflowId, workFolder);
        });
    }

    /**
     * @brief Implements DoWork callback.
     *
     * @param token Opaque token.
     * @param workflowData Current workflow data object.
     */
    static void DoWorkCallback(ADUC_Token token, ADUC_WorkflowDataToken workflowData) noexcept
    {
        UNREFERENCED_PARAMETER(token);
        UNREFERENCED_PARAMETER(workflowData);
    }

    //
    // Implementation.
    //

    // Private constructor, must call Create factory method.
    LinuxPlatformLayer() = default;

    void Idle(const char* workflowId);
    ADUC_Result Download(const ADUC_WorkflowData* workflowData);
    ADUC_Result Backup(const ADUC_WorkflowData* workflowData);
    ADUC_Result Install(const ADUC_WorkflowData* workflowData);
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData);
    ADUC_Result Restore(const ADUC_WorkflowData* workflowData);
    void Cancel(const ADUC_WorkflowData* workflowData);

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData);

    /**
     * @brief Class implementation of SandboxCreate method.
     *
     * @param workflowId Unique workflow identifier.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     * @return ADUC_Result
     */
    ADUC_Result SandboxCreate(const char* workflowId, char* workFolder);

    /**
     * @brief Class implementation of SandboxDestroy method.
     *
     * @param workflowId Unique workflow identifier.
     * @param workFolder Sandbox path.
     */
    void SandboxDestroy(const char* workflowId, const char* workFolder);

    /**
     * @brief Was Cancel called?
     */
    std::atomic_bool _IsCancellationRequested{ false };
};
} // namespace ADUC

#endif // LINUX_ADU_CORE_IMPL_HPP
