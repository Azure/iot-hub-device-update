/**
 * @file agent.h
 * @brief ADU Gen2 agent main entry point - daemon lifecycle management.
 */
#ifndef ADUC_AGENT_H
#define ADUC_AGENT_H

#include "aduc/extension_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_AgentConfig {
    const char* configPath;
    const char* logFilePath;
    ADUC_LogLevel logLevel;
    bool runOnce;           // Process one deployment then exit (for testing)
    uint32_t pollIntervalSec;
} ADUC_AgentConfig;

// Parse command-line args into config
ADUC_Result2 ADUC_Agent_ParseArgs(int argc, char** argv, ADUC_AgentConfig* config);

// Run the agent (blocking - returns on shutdown)
ADUC_Result2 ADUC_Agent_Run(const ADUC_AgentConfig* config);

#ifdef __cplusplus
}
#endif
#endif // ADUC_AGENT_H
