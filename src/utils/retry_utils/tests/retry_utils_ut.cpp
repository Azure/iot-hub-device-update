/**
 * @file retry_utils_ut.cpp
 * @brief Unit Tests for retry_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <aduc/retry_utils.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

TEST_CASE("ADUC_Retry_Delay_Calculator - Basic functionality")
{
    SECTION("Zero retries with no additional delay")
    {
        // With 0 retries, delay should be (2^0 * initialDelayUnit) = 1 * initialDelayUnit
        // initialDelayUnit = 1000ms = 1 second
        // Expected base delay: 1 second
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            0,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent (no jitter for predictable test)
        );
        
        // Result should be approximately now + 1 second (allowing for execution time)
        CHECK(result >= now);
        CHECK(result <= now + 2);  // Should be within 2 seconds
    }

    SECTION("One retry with no additional delay")
    {
        // With 1 retry, delay should be (2^1 * initialDelayUnit) = 2 * 1000ms = 2 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            1,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now + 2 seconds
        CHECK(result >= now + 1);
        CHECK(result <= now + 3);
    }

    SECTION("Multiple retries with exponential backoff")
    {
        // With 3 retries, delay should be (2^3 * 1000ms) = 8 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            3,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now + 8 seconds
        CHECK(result >= now + 7);
        CHECK(result <= now + 9);
    }

    SECTION("Additional delay is correctly added")
    {
        // With 0 retries, base delay = 1 second
        // Additional delay = 5 seconds
        // Total = 6 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            5,      // additionalDelaySecs
            0,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now + 6 seconds
        CHECK(result >= now + 5);
        CHECK(result <= now + 7);
    }
}

TEST_CASE("ADUC_Retry_Delay_Calculator - Max delay capping")
{
    SECTION("Delay exceeds max delay and is capped")
    {
        // With 10 retries, delay would be (2^10 * 1000ms) = 1024 seconds
        // But maxDelaySecs = 30, so it should be capped at 30 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            10,     // retries
            1000,   // initialDelayUnitMilliSecs
            30,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now + 30 seconds (max delay)
        CHECK(result >= now + 29);
        CHECK(result <= now + 31);
    }

    SECTION("Max retry exponent is respected")
    {
        // With retries > ADUC_RETRY_MAX_RETRY_EXPONENT (9)
        // delay = (2^9 * 1000ms) = 512 seconds, but capped by maxDelaySecs = 60
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            15,     // retries (> MAX_RETRY_EXPONENT)
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be capped at maxDelaySecs = 60
        CHECK(result >= now + 59);
        CHECK(result <= now + 61);
    }

    SECTION("Small max delay caps early")
    {
        // With 5 retries, delay would be (2^5 * 1000ms) = 32 seconds
        // But maxDelaySecs = 10, so it should be capped at 10 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            5,      // retries
            1000,   // initialDelayUnitMilliSecs
            10,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be capped at 10 seconds
        CHECK(result >= now + 9);
        CHECK(result <= now + 11);
    }
}

TEST_CASE("ADUC_Retry_Delay_Calculator - Jitter behavior")
{
    SECTION("Jitter increases delay within expected range")
    {
        // With jitter, the delay should be between base delay and base delay * (1 + maxJitterPercent/100)
        // Base delay = 1 second, maxJitterPercent = 10
        // Range: 1 to 1.1 seconds
        
        srand(12345);  // Set seed for reproducibility
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            0,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            10.0    // maxJitterPercent
        );
        
        // Result should be approximately now + 1 to 1.1 seconds
        CHECK(result >= now);
        CHECK(result <= now + 2);  // Upper bound allowing for jitter
    }

    SECTION("High jitter percentage")
    {
        // With high jitter (50%), delay variance is significant
        // Base delay = 2 seconds (2^1 * 1000ms)
        // Range: 2 to 3 seconds with 50% jitter
        
        srand(54321);
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            1,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            50.0    // maxJitterPercent
        );
        
        // Result should be within expected jitter range
        CHECK(result >= now + 1);
        CHECK(result <= now + 4);
    }

    SECTION("Zero jitter gives deterministic results")
    {
        // Multiple calls with same parameters and zero jitter should give consistent results
        
        time_t result1 = ADUC_Retry_Delay_Calculator(0, 2, 1000, 60, 0.0);
        time_t result2 = ADUC_Retry_Delay_Calculator(0, 2, 1000, 60, 0.0);
        
        // Results should be very close (within 1 second due to time progression)
        CHECK(std::abs(result1 - result2) <= 1);
    }
}

TEST_CASE("ADUC_Retry_Delay_Calculator - Different initial delay units")
{
    SECTION("Small initial delay unit")
    {
        // initialDelayUnit = 100ms = 0.1 second
        // With 0 retries, delay = 2^0 * 0.1 = 0.1 seconds (rounds to 0)
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            0,      // retries
            100,    // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now (delay < 1 second)
        CHECK(result >= now);
        CHECK(result <= now + 1);
    }

    SECTION("Large initial delay unit")
    {
        // initialDelayUnit = 5000ms = 5 seconds
        // With 1 retry, delay = 2^1 * 5 = 10 seconds
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            1,      // retries
            5000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now + 10 seconds
        CHECK(result >= now + 9);
        CHECK(result <= now + 11);
    }
}

TEST_CASE("ADUC_Retry_Delay_Calculator - Edge cases")
{
    SECTION("Very high retry count")
    {
        // Even with very high retry count, should be capped by MAX_RETRY_EXPONENT and maxDelaySecs
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,         // additionalDelaySecs
            1000,      // retries (very high)
            1000,      // initialDelayUnitMilliSecs
            100,       // maxDelaySecs
            0.0        // maxJitterPercent
        );
        
        // Should be capped at maxDelaySecs = 100
        CHECK(result >= now + 99);
        CHECK(result <= now + 101);
    }

    SECTION("Zero max delay")
    {
        // If maxDelaySecs is 0, delay should still be calculated but capped at 0
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,      // additionalDelaySecs
            2,      // retries
            1000,   // initialDelayUnitMilliSecs
            0,      // maxDelaySecs (edge case)
            0.0     // maxJitterPercent
        );
        
        // Result should be approximately now (capped at 0)
        CHECK(result >= now);
        CHECK(result <= now + 1);
    }

    SECTION("Combined additional delay and max cap")
    {
        // additionalDelay = 20, base delay would be 4s, total = 24s
        // But maxDelaySecs = 15, so total should be capped at 15s
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            20,     // additionalDelaySecs
            2,      // retries (2^2 * 1s = 4s base delay)
            1000,   // initialDelayUnitMilliSecs
            15,     // maxDelaySecs
            0.0     // maxJitterPercent
        );
        
        // additionalDelay is added after cap, so result = now + 20 + min(4, 15) = now + 24
        // Actually checking the implementation: delay is capped, then additional is added
        // So: min(4, 15) + 20 = 24
        CHECK(result >= now + 23);
        CHECK(result <= now + 25);
    }
}

TEST_CASE("ADUC_Retry_Delay_Calculator - Realistic scenarios")
{
    SECTION("Default parameters simulation")
    {
        // Using common default values
        // ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS = 1000 (1 second)
        // ADUC_RETRY_DEFAULT_MAX_BACKOFF_TIME_MS = 60000 (60 seconds)
        // ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT = 5
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            0,                                      // additionalDelaySecs
            3,                                      // retries
            ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS,    // 1000ms
            ADUC_RETRY_DEFAULT_MAX_BACKOFF_TIME_MS / 1000,  // 60s
            ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT   // 5%
        );
        
        // With 3 retries: 2^3 * 1s = 8s base, plus up to 5% jitter
        CHECK(result >= now + 7);
        CHECK(result <= now + 10);
    }

    SECTION("HTTP retry with additional delay")
    {
        // Simulating HTTP 429 (Too Many Requests) with Retry-After header
        // Retry-After suggests 10 seconds additional delay
        
        time_t now = time(nullptr);
        time_t result = ADUC_Retry_Delay_Calculator(
            10,     // additionalDelaySecs (from Retry-After header)
            1,      // retries
            1000,   // initialDelayUnitMilliSecs
            60,     // maxDelaySecs
            5.0     // maxJitterPercent
        );
        
        // Base delay = 2^1 * 1s = 2s, plus 10s additional = 12s total
        CHECK(result >= now + 11);
        CHECK(result <= now + 14);
    }
}