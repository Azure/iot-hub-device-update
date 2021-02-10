/**
 * @file linux_adu_core_impl.hpp
 * @brief Implements the ADU Core interface functionality for linux platform.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef LINUX_ADU_CORE_IMPL_HPP
#define LINUX_ADU_CORE_IMPL_HPP

#include <atomic>
#include <exception>
#include <thread>

#include <aduc/adu_core_exports.h>
#include <aduc/exception_utils.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>

#include <aduc/content_handler.hpp>
#include <aduc/content_handler_factory.hpp>

namespace ADUC
{
/**
 * @brief Implementation class for UpdateAction handlers.
 */
class LinuxPlatformLayer
{
public:
    static std::unique_ptr<LinuxPlatformLayer> Create();

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
     * @param workflowId Current workflow identifier.
     * @param info #ADUC_DownloadInfo with information on how to apply.
     * @return ADUC_Result
     */
    static ADUC_Result DownloadCallback(
        ADUC_Token token,
        const char* workflowId,
        const char* updateType,
        const ADUC_WorkCompletionData* workCompletionData,
        const ADUC_DownloadInfo* info) noexcept
    {
        try
        {
            Log_Info("Download thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, updateType, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_DownloadResult_Failure,
                    [&token, &workflowId, &updateType, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Download(workflowId, updateType, info);
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
        };
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
        const ADUC_InstallInfo* info) noexcept
    {
        try
        {
            Log_Info("Install thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_InstallResult_Failure, [&token, &workflowId, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Install(workflowId, info);
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
        const ADUC_ApplyInfo* info) noexcept
    {
        try
        {
            Log_Info("Apply thread started");

            // Pointers passed to this method are guaranteed to be valid until WorkCompletionCallback is called.
            std::thread worker{ [token, workflowId, workCompletionData, info] {
                const ADUC_Result result{ ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
                    ADUC_ApplyResult_Failure, [&token, &workflowId, &workCompletionData, &info]() -> ADUC_Result {
                        return static_cast<LinuxPlatformLayer*>(token)->Apply(workflowId, info);
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
            return ADUC_Result{ ADUC_ApplyResult_Failure, e.Code() };
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
    static void CancelCallback(ADUC_Token token, const char* workflowId) noexcept
    {
        Log_Info("CancelCallback called");

        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [&token, &workflowId]() -> void { static_cast<LinuxPlatformLayer*>(token)->Cancel(workflowId); });
    }

    static ADUC_Result IsInstalledCallback(
        ADUC_Token token, const char* workflowId, const char* updateType, const char* installedCriteria) noexcept
    {
        Log_Info("IsInstalledCallback called");

        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_IsInstalledResult_Failure, [&token, &workflowId, &updateType, &installedCriteria]() -> ADUC_Result {
                return static_cast<LinuxPlatformLayer*>(token)->IsInstalled(workflowId, updateType, installedCriteria);
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
    static ADUC_Result SandboxCreateCallback(ADUC_Token token, const char* workflowId, char** workFolder) noexcept
    {
        return ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(
            ADUC_SandboxCreateResult_Failure, [&token, &workflowId, &workFolder]() -> ADUC_Result {
                return static_cast<LinuxPlatformLayer*>(token)->SandboxCreate(workflowId, workFolder);
            });
    }

    /**
     * @brief Implements SandboxDestroy callback.
     *
     * @param token Contains pointer to our class instance.
     * @param workflowId Unique workflow identifier.
     * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
     */
    static void SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder) noexcept
    {
        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions([&token, &workflowId, &workFolder]() -> void {
            static_cast<LinuxPlatformLayer*>(token)->SandboxDestroy(workflowId, workFolder);
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
                return static_cast<LinuxPlatformLayer*>(token)->Prepare(workflowId, prepareInfo);
            });
    }

    /**
     * @brief Implements DoWork callback.
     *
     * @param token Opaque token.
     * @param workflowId Current workflow identifier.
     */
    static void DoWorkCallback(ADUC_Token token, const char* workflowId) noexcept
    {
        // Not used in this code.
        UNREFERENCED_PARAMETER(token);
        UNREFERENCED_PARAMETER(workflowId);
    }

    //
    // Implementation.
    //

    // Private constructor, must call Create factory method.
    LinuxPlatformLayer() = default;

    void Idle(const char* workflowId);
    ADUC_Result Download(const char* workflowId, const char* updateType, const ADUC_DownloadInfo* info);
    ADUC_Result Install(const char* workflowId, const ADUC_InstallInfo* info);
    ADUC_Result Apply(const char* workflowId, const ADUC_ApplyInfo* info);
    void Cancel(const char* workflowId);

    ADUC_Result IsInstalled(const char* workflowId, const char* updateType, const char* installedCriteria);

    /**
     * @brief Class implementation of SandboxCreate method.
     *
     * @param workflowId Unique workflow identifier.
     * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
     * Must be allocated using malloc.
     * @return ADUC_Result
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
     * @param metadata.
     */
    ADUC_Result Prepare(const char* workflowId, const ADUC_PrepareInfo* prepareInfo);

    /**
     * @brief Was Cancel called?
     */
    std::atomic_bool _IsCancellationRequested{ false };

    std::unique_ptr<ContentHandler> _contentHandler;
};
} // namespace ADUC

#endif // LINUX_ADU_CORE_IMPL_HPP
