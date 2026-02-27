/**
 * @file mock_simulator_handler_deps.h
 * @brief Configurable mock state for simulator_handler.cpp dependencies.
 */
#ifndef MOCK_SIMULATOR_HANDLER_DEPS_H
#define MOCK_SIMULATOR_HANDLER_DEPS_H

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* --- workflow_* mocks --- */
    extern size_t mock_update_files_count;
    extern bool mock_get_update_file_return;
    extern const char* mock_target_filename;
    extern const char* mock_installed_criteria;

    /* --- Reset all mocks --- */
    void mock_simulator_handler_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_SIMULATOR_HANDLER_DEPS_H */
