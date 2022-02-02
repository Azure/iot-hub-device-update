/**
 * @file simulator_adu_core_impl.hpp
 * @brief Implements an ADUC "simulator" mode.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef SIMULATOR_ADU_CORE_IMPL_HPP
#define SIMULATOR_ADU_CORE_IMPL_HPP

#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include <aduc/adu_core_exports.h>
#include <aduc/content_handler.hpp>
#include <aduc/exception_utils.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>

/**
 * @brief Simulation type to run.
 */
enum class SimulationType
{
    DownloadFailed, /**< Simulate a download failure. */
    BackupFailed, /**< Simulate a backup failure. */
    InstallationFailed, /**< Simulate an install failure. */
    ApplyFailed, /**< Simulate an apply failure. */
    RestoreFailed, /**< Simulate an restore failure. */
    IsInstalledFailed, /**< Simulate IsInstalled failure. */
    AllSuccessful, /**< Simulate a successful run. */
};

namespace ADUC
{
/**
 * @brief Implementation class for UpdateAction handlers.
 */
class SimulatorPlatformLayer
{
public:
    static std::unique_ptr<SimulatorPlatformLayer>
    Create(SimulationType type = SimulationType::AllSuccessful);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SimulatorPlatformLayer(const SimulatorPlatformLayer&) = delete;
    SimulatorPlatformLayer& operator=(const SimulatorPlatformLayer&) = delete;
    SimulatorPlatformLayer(SimulatorPlatformLayer&&) = delete;
    SimulatorPlatformLayer& operator=(SimulatorPlatformLayer&&) = delete;

    ~SimulatorPlatformLayer() = default;

    /**
     * @brief Set the #ADUC_UpdateActionCallbacks object
     *
     * @param data Object to set.
     * @return ADUC_Result ResultCode.
     */
    ADUC_Result SetUpdateActionCallbacks(ADUC_UpdateActionCallbacks* data);

private:
    //
    // Static callbacks.
    //

    /**
     * @brief Implements Idle callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     */
    static void IdleCallback(ADUC_Token token, const char* workflowId)
    {
        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [&token, &workflowId]() -> void { static_cast<SimulatorPlatformLayer*>(token)->Idle(workflowId); });
    }

    /**
     * @brief Implements Download callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     * @param updateType
     * @param info #ADUC_WrokflowData with information on how to download.
     * @return ADUC_Result
     */

   /**
     * @brief Implements Download callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info ADUC_WorkflowDataToken with information on how to download.
     * @return ADUC_Result
     */
    static ADUC_Result DownloadCallback(
        ADUC_Token token,
        const ADUC_WorkCompletionData* workCompletionData,
        ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Download thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure,
                    [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Download(workflowData);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result, true /* isAsync */);
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
        }
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
        ADUC_Token token,
        const ADUC_WorkCompletionData* workCompletionData,
        ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Install thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Install(workflowData);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_Result_Install_InProgress };
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
     * @brief Implements Apply callback.
     *
     * @param token Opaque token.
     * @param workCompletionData Contains information on what to do when task is completed.
     * @param info #ADUC_WorkflowData with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result ApplyCallback(
        ADUC_Token token,
        const ADUC_WorkCompletionData* workCompletionData,
        ADUC_WorkflowDataToken info) noexcept
    {
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Apply thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Apply(workflowData);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result, true /* isAsync */);
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
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);
        try
        {
            Log_Info("Backup thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workCompletionData, workflowData] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_Result_Failure, [&token, &workCompletionData, &workflowData]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Backup(workflowData);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result, true /* isAsync */);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_Result_Backup_InProgress };
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
                        return static_cast<SimulatorPlatformLayer*>(token)->Restore(workflowData);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result, true /* isAsync */);
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
     * @param info #ADUC_WorkflowData with information on how to apply.
     */
    static void CancelCallback(ADUC_Token token, ADUC_WorkflowDataToken info) noexcept
    {
        Log_Info("CancelCallback called");
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);

        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions([&token, &workflowData]() -> void {
            static_cast<SimulatorPlatformLayer*>(token)->Cancel(workflowData);
        });
    }

    static ADUC_Result IsInstalledCallback(ADUC_Token token, ADUC_WorkflowDataToken info) noexcept
    {
        Log_Info("IsInstalledCallback called");
        const ADUC_WorkflowData* workflowData = static_cast<const ADUC_WorkflowData*>(info);

        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_Result_Failure, [&token, &workflowData]() -> ADUC_Result {
                return static_cast<SimulatorPlatformLayer*>(token)->IsInstalled(workflowData);
            });
    }

    /**
     * @brief Implements SandboxCreate callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     *
     * @return ADUC_Result
     */
    static ADUC_Result SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
    {
        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_Result_Failure, [&token, &workflowId, &workFolder]() -> ADUC_Result {
                return static_cast<SimulatorPlatformLayer*>(token)->SandboxCreate(workflowId, workFolder);
            });
    }

    /**
     * @brief Implements SandboxDestroy callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workflowId Unique workflow identifier.
     * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
     */
    static void SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder)
    {
        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions([&token, &workflowId, &workFolder]() -> void {
            static_cast<SimulatorPlatformLayer*>(token)->SandboxDestroy(workflowId, workFolder);
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
        // Not used in this example.
        // Not used in this code.
        UNREFERENCED_PARAMETER(token);
        UNREFERENCED_PARAMETER(workflowData);
    }

    //
    // Implementation.
    //

    // Private constructor, must use Create factory method to creat an object.
    SimulatorPlatformLayer(SimulationType type);

    /**
     * @brief Class implementation of Idle method.
     */
    void Idle(const char* workflowId);

    /**
     * @brief Class implementation of Download method.
     * @return ADUC_Result
     */
    ADUC_Result Download(const ADUC_WorkflowData* workflowData);

    /**
     * @brief Class implementation of Install method.
     * @return ADUC_Result
     */
    ADUC_Result Install(const ADUC_WorkflowData* workflowData);

    /**
     * @brief Class implementation of Apply method.
     * @return ADUC_Result
     */
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData);

    /**
     * @brief Class implementation of Cancel method.
     */
    void Cancel(const ADUC_WorkflowData* workflowData);

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData);

    /**
     * @brief Class implementation of SandboxCreate method.
     * @param workflowId Unique workflow identifier.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     */
    ADUC_Result SandboxCreate(const char* workflowId, char* workFolder);

    /**
     * @brief Class implementation of SandboxDestroy method.
     *
     * @param workflowId Unique workflow identifier.
     * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
     */
    void SandboxDestroy(const char* workflowId, const char* workFolder);

    //
    // Accessors.
    //

    /**
     * @brief Get the #SimulationType object
     *
     * @return SimulationType
     */
    SimulationType GetSimulationType() const
    {
        return _simulationType;
    }

    /**
     * @brief Determine if cancellation was requested.
     *
     * @return bool True if requested.
     */
    bool CancellationRequested() const
    {
        return _cancellationRequested;
    }

    //
    // Members.
    //

    /**
     * @brief Simulation that's being run.
     */
    SimulationType _simulationType;

    /**
     * @brief Was Cancel called?
     */
    bool _cancellationRequested;
};
} // namespace ADUC

#endif // SIMULATOR_ADU_CORE_IMPL_HPP
