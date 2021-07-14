/**
 * @file pvcontrol_simulator_handler.hpp
 * @brief Defines PVControlSimulatorHandlerImpl.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 * @copyright Copyright (c) 2021, Pantacor Ltd.
 */
#ifndef ADUC_PVCONTROL_SIMULATOR_HANDLER_HPP
#define ADUC_PVCONTROL_SIMULATOR_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include "aduc/pvcontrol_handler.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> pantacor_pvcontrol_Simulator_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class PVControlSimulatorHandlerImpl
 * @brief The pvcontrol specific simulator implementation.
 */
class PVControlSimulatorHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& logFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    PVControlSimulatorHandlerImpl(const PVControlSimulatorHandlerImpl&) = delete;
    PVControlSimulatorHandlerImpl& operator=(const PVControlSimulatorHandlerImpl&) = delete;
    PVControlSimulatorHandlerImpl(PVControlSimulatorHandlerImpl&&) = delete;
    PVControlSimulatorHandlerImpl& operator=(PVControlSimulatorHandlerImpl&&) = delete;

    ~PVControlSimulatorHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    PVControlSimulatorHandlerImpl(
        const std::string& workFolder, const std::string& logFolder, const std::string& filename)
    {
    }

    bool _isInstalled{ false };
};

#endif // ADUC_PVCONTROL_SIMULATOR_HANDLER_HPP
