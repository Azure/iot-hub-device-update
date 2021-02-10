/**
 * @file simulator_adu_core_impl.hpp
 * @brief Implements an ADUC "simulator" mode.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef SIMULATOR_ADU_CORE_IMPL_HPP
#define SIMULATOR_ADU_CORE_IMPL_HPP

#include <exception>
#include <memory>
#include <thread>

#include <aduc/adu_core_exports.h>
#include <aduc/content_handler.hpp>
#include <aduc/content_handler_factory.hpp>
#include <aduc/exception_utils.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>

/**
 * @brief Simulation type to run.
 */
enum class SimulationType
{
    DownloadFailed, /**< Simulate a download failure. */
    InstallationFailed, /**< Simulate an install failure. */
    ApplyFailed, /**< Simulate an apply failure. */
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
    Create(SimulationType type = SimulationType::AllSuccessful, bool performDownload = false);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SimulatorPlatformLayer(const SimulatorPlatformLayer&) = delete;
    SimulatorPlatformLayer& operator=(const SimulatorPlatformLayer&) = delete;
    SimulatorPlatformLayer(SimulatorPlatformLayer&&) = delete;
    SimulatorPlatformLayer& operator=(SimulatorPlatformLayer&&) = delete;

    ~SimulatorPlatformLayer() = default;

    /**
     * @brief Set the #ADUC_RegisterData object
     *
     * @param data Object to set.
     * @return ADUC_Result ResultCode.
     */
    ADUC_Result SetRegisterData(ADUC_RegisterData* data);

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
     * @param info #ADUC_DownloadInfo with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result DownloadCallback(
        ADUC_Token token,
        const char* workflowId,
        const char* updateType,
        const ADUC_WorkCompletionData* workCompletionData,
        const ADUC_DownloadInfo* info)
    {
        try
        {
            Log_Info("Download thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, updateType, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_DownloadResult_Failure,
                    [&token, &workflowId, &updateType, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Download(workflowId, updateType, info);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_DownloadResult_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_DownloadResult_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_DownloadResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_DownloadResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    /**
     * @brief Implements Install callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     * @param info #ADUC_InstallInfo with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result InstallCallback(
        ADUC_Token token,
        const char* workflowId,
        const ADUC_WorkCompletionData* workCompletionData,
        const ADUC_InstallInfo* info)
    {
        try
        {
            Log_Info("Install thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_InstallResult_Failure, [&token, &workflowId, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Install(workflowId, info);
                    }) };

                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_InstallResult_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_InstallResult_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    /**
     * @brief Implements Apply callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     * @param info #ADUC_ApplyInfo with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result ApplyCallback(
        ADUC_Token token,
        const char* workflowId,
        const ADUC_WorkCompletionData* workCompletionData,
        const ADUC_ApplyInfo* info)
    {
        try
        {
            Log_Info("Apply thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_ApplyResult_Failure, [&token, &workflowId, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<SimulatorPlatformLayer*>(token)->Apply(workflowId, info);
                    }) };
                // Report result to main thread.
                workCompletionData->WorkCompletionCallback(workCompletionData->WorkCompletionToken, result);
            } };

            // Allow the thread to work independently of this main thread.
            worker.detach();

            // Indicate that we've spun off a thread to do the actual work.
            return ADUC_Result{ ADUC_ApplyResult_InProgress };
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
            return ADUC_Result{ ADUC_InstallResult_Failure, e.Code() };
        }
        catch (const std::exception& e)
        {
            Log_Error("Unhandled std exception: %s", e.what());
            return ADUC_Result{ ADUC_ApplyResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            return ADUC_Result{ ADUC_ApplyResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    /**
     * @brief Implements Cancel callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     */
    static void CancelCallback(ADUC_Token token, const char* workflowId)
    {
        Log_Info("CancelCallback called");

        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [&token, &workflowId]() -> void { static_cast<SimulatorPlatformLayer*>(token)->Cancel(workflowId); });
    }

    static ADUC_Result IsInstalledCallback(
        ADUC_Token token, const char* workflowId, const char* updateType, const char* installedCriteria) noexcept
    {
        Log_Info("IsInstalledCallback called");

        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_IsInstalledResult_Failure, [&token, &workflowId, &updateType, &installedCriteria]() -> ADUC_Result {
                return static_cast<SimulatorPlatformLayer*>(token)->IsInstalled(
                    workflowId, updateType, installedCriteria);
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
    static ADUC_Result SandboxCreateCallback(ADUC_Token token, const char* workflowId, char** workFolder)
    {
        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_SandboxCreateResult_Failure, [&token, &workflowId, &workFolder]() -> ADUC_Result {
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
     * @brief Implements Prepare callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workflowId Unique workflow identifier.
     * @param prepareInfo.
     *
     * @return ADUC_Result
     */
    static ADUC_Result
    PrepareCallback(ADUC_Token token, const char* workflowId, const ADUC_PrepareInfo* prepareInfo) noexcept
    {
        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_PrepareResult_Failure, [&token, &workflowId, &prepareInfo]() -> ADUC_Result {
                return static_cast<SimulatorPlatformLayer*>(token)->Prepare(workflowId, prepareInfo);
            });
    }

    /**
     * @brief Implements DoWork callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     */
    static void DoWorkCallback(ADUC_Token token, const char* workflowId)
    {
        // Not used in this example.
        // Not used in this code.
        UNREFERENCED_PARAMETER(token);
        UNREFERENCED_PARAMETER(workflowId);
    }

    //
    // Implementation.
    //

    // Private constructor, must use Create factory method to creat an object.
    SimulatorPlatformLayer(SimulationType type, bool performDownload);

    /**
     * @brief Class implementation of Idle method.
     */
    void Idle(const char* workflowId);

    /**
     * @brief Class implementation of Download method.
     * @return ADUC_Result
     */
    ADUC_Result Download(const char* workflowId, const char* updateType, const ADUC_DownloadInfo* info);

    /**
     * @brief Class implementation of Install method.
     * @return ADUC_Result
     */
    ADUC_Result Install(const char* workflowId, const ADUC_InstallInfo* info);

    /**
     * @brief Class implementation of Apply method.
     * @return ADUC_Result
     */
    ADUC_Result Apply(const char* workflowId, const ADUC_ApplyInfo* info);

    /**
     * @brief Class implementation of Cancel method.
     */
    void Cancel(const char* workflowId);

    ADUC_Result IsInstalled(const char* workflowId, const char* updateType, const char* installedCriteria);

    /**
     * @brief Class implementation of SandboxCreate method.
     * @param workflowId Unique workflow identifier.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     */
    ADUC_Result SandboxCreate(const char* workflowId, char** workFolder);

    /**
     * @brief Class implementation of SandboxDestroy method.
     *
     * @param workflowId Unique workflow identifier.
     * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
     */
    void SandboxDestroy(const char* workflowId, const char* workFolder);

    /**
     * @brief Class implementation of Prepare method.
     *
     * @param workflowId Unique workflow identifier.
     * @param metadata
     */
    ADUC_Result Prepare(const char* workflowId, const ADUC_PrepareInfo* prepareInfo);

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
     * @brief Determins if a real download should occur.
     *
     * @return bool True if download should occur.
     */
    bool ShouldPerformDownload() const
    {
        return _shouldPerformDownload;
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
     * @brief Should a download actually occur? (Useful for testing endpoints)
     */
    bool _shouldPerformDownload;

    /**
     * @brief Was Cancel called?
     */
    bool _cancellationRequested;

    std::unique_ptr<ContentHandler> _contentHandler;
};
} // namespace ADUC

#endif // SIMULATOR_ADU_CORE_IMPL_HPP
