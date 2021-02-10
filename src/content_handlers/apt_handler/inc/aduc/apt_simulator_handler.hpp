/**
 * @file apt_simulator_handler.hpp
 * @brief Defines AptSimulatorHandlerImpl.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_APT_SIMULATOR_HANDLER_HPP
#define ADUC_APT_SIMULATOR_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include "aduc/apt_handler.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_apt_Simulator_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class AptSimulatorHandlerImpl
 * @brief The apt specific simulator implementation.
 */
class AptSimulatorHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    AptSimulatorHandlerImpl(const AptSimulatorHandlerImpl&) = delete;
    AptSimulatorHandlerImpl& operator=(const AptSimulatorHandlerImpl&) = delete;
    AptSimulatorHandlerImpl(AptSimulatorHandlerImpl&&) = delete;
    AptSimulatorHandlerImpl& operator=(AptSimulatorHandlerImpl&&) = delete;

    ~AptSimulatorHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    AptSimulatorHandlerImpl(const std::string& workFolder, const std::string& filename)
    {
    }

    bool _isInstalled{ false };
};

#endif // ADUC_APT_SIMULATOR_HANDLER_HPP
