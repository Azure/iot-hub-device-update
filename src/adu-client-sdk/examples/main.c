/**
 * @file main.c
 * @brief Example demonstrating Azure Device Update Core SDK usage
 * 
 * This example shows how to use the Core SDK result types and utilities.
 */

#include <stdio.h>
#include <aduc/result.h>
#include <aduc/types.h>

int main()
{
    printf("Azure Device Update Core SDK Example\n");
    printf("=====================================\n\n");

    // Test result creation and handling
    ADUC_ResultDetails result;
    
    // Test success result
    printf("1. Testing success result:\n");
    ADUC_Result_SetSuccess(&result, "Operation completed successfully");
    printf("   Result code: %d (%s)\n", result.resultCode, ADUC_Result_ToString(result.resultCode));
    printf("   Is success: %s\n", ADUC_Result_IsSuccess(&result) ? "true" : "false");
    printf("   Is failure: %s\n", ADUC_Result_IsFailure(&result) ? "true" : "false");
    printf("   Details: %s\n", result.resultDetails ? result.resultDetails : "NULL");
    ADUC_Result_Free(&result);
    
    printf("\n");
    
    // Test failure result
    printf("2. Testing failure result:\n");
    ADUC_Result_SetFailure(&result, ADUC_Result_Failure_FileNotFound, "Configuration file not found");
    printf("   Result code: %d (%s)\n", result.resultCode, ADUC_Result_ToString(result.resultCode));
    printf("   Is success: %s\n", ADUC_Result_IsSuccess(&result) ? "true" : "false");
    printf("   Is failure: %s\n", ADUC_Result_IsFailure(&result) ? "true" : "false");
    printf("   Details: %s\n", result.resultDetails ? result.resultDetails : "NULL");
    ADUC_Result_Free(&result);
    
    printf("\n");
    
    // Test some enum values
    printf("3. Testing result code enumeration:\n");
    printf("   ADUC_Result_Success = %d\n", ADUC_Result_Success);
    printf("   ADUC_Result_Failure = %d\n", ADUC_Result_Failure);
    printf("   ADUC_Result_Failure_NetworkError = %d\n", ADUC_Result_Failure_NetworkError);
    printf("   ADUC_Result_Failure_InstallFailed = %d\n", ADUC_Result_Failure_InstallFailed);
    
    printf("\n");
    
    // Test log levels
    printf("4. Testing log levels:\n");
    printf("   ADUC_LOG_ERROR = %d\n", ADUC_LOG_ERROR);
    printf("   ADUC_LOG_WARN = %d\n", ADUC_LOG_WARN);
    printf("   ADUC_LOG_INFO = %d\n", ADUC_LOG_INFO);
    printf("   ADUC_LOG_DEBUG = %d\n", ADUC_LOG_DEBUG);
    
    printf("\n✅ Core SDK example completed successfully!\n");
    
    return 0;
}