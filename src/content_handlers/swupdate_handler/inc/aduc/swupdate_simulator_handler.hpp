/**
 * @file swupdate_simulator_handler.hpp
 * @brief Defines SWUpdateSimulatorHandlerImpl.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_SWUPDATE_SIMULATOR_HANDLER_HPP
#define ADUC_SWUPDATE_SIMULATOR_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include "aduc/swupdate_handler.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_swupdate_Simulator_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class SWUpdateSimulatorHandlerImpl
 * @brief The swupdate specific simulator implementation.
 */
class SWUpdateSimulatorHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& logFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateSimulatorHandlerImpl(const SWUpdateSimulatorHandlerImpl&) = delete;
    SWUpdateSimulatorHandlerImpl& operator=(const SWUpdateSimulatorHandlerImpl&) = delete;
    SWUpdateSimulatorHandlerImpl(SWUpdateSimulatorHandlerImpl&&) = delete;
    SWUpdateSimulatorHandlerImpl& operator=(SWUpdateSimulatorHandlerImpl&&) = delete;

    ~SWUpdateSimulatorHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    SWUpdateSimulatorHandlerImpl(
        const std::string& workFolder, const std::string& logFolder, const std::string& filename)
    {
    }

    bool _isInstalled{ false };
};

#endif // ADUC_SWUPDATE_SIMULATOR_HANDLER_HPP
