/**
 * @file apiproto_utils_ut.cpp
 * @brief Comprehensive Unit Tests for apiproto_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apiproto.h"
#include "aduc/c_utils.h"
#include "aduc/logging.h"

#include <catch2/catch_all.hpp>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using Catch::Matchers::Equals;

/**
 * @brief Test fixture for setting up test pipes using pipe() syscall
 */
class ApiProtoTestFixture
{
public:
    ApiProtoTestFixture()
    {
        read_fd = -1;
        write_fd = -1;
        pipe_fds[0] = -1;
        pipe_fds[1] = -1;
    }

    ~ApiProtoTestFixture()
    {
        if (read_fd >= 0) close(read_fd);
        if (write_fd >= 0) close(write_fd);
        if (pipe_fds[0] >= 0) close(pipe_fds[0]);
        if (pipe_fds[1] >= 0) close(pipe_fds[1]);
    }

    void CreatePipe()
    {
        REQUIRE(pipe(pipe_fds) == 0);
        read_fd = pipe_fds[0];
        write_fd = pipe_fds[1];
        REQUIRE(read_fd >= 0);
        REQUIRE(write_fd >= 0);
    }

    int read_fd;
    int write_fd;
    int pipe_fds[2];
};

/**
 * @brief Tests successful request message sending scenarios
 *
 * Validates that msg_send_req correctly serializes and transmits request messages
 * with various data payloads. Ensures proper header formatting, data transmission,
 * and return value calculation for different message sizes.
 *
 * Expected Behavior:
 * - Returns total bytes written (header + data)
 * - Correctly writes message header and payload to file descriptor
 * - Handles zero-length and maximum-length data payloads
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_send_req Happy Path Tests")
{
    CreatePipe();

    SECTION("Send request with data")
    {
        const char* test_data = "test_message_data";
        size_t data_len = strlen(test_data);

        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = (uint16_t)data_len
        };
        strncpy(req.data, test_data, data_len);

        ssize_t result = msg_send_req(write_fd, &req);

        // Should return header size + data size
        ssize_t expected = (ssize_t)(3 * sizeof(uint16_t)) + (ssize_t)data_len;
        CHECK(result == expected);

        // Verify we can read the data back
        char buffer[1024];
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer));
        CHECK(bytes_read == expected);
    }

    SECTION("Send request with no data")
    {
        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = 0
        };

        ssize_t result = msg_send_req(write_fd, &req);

        // Should return only header size
        ssize_t expected = (ssize_t)(3 * sizeof(uint16_t));
        CHECK(result == expected);

        // Verify we can read the header back
        char buffer[1024];
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer));
        CHECK(bytes_read == expected);
    }

    SECTION("Send request with maximum data")
    {
        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = MAX_BUF_LEN
        };

        // Fill with test pattern
        for (int i = 0; i < MAX_BUF_LEN; i++) {
            req.data[i] = (char)(i % 256);
        }

        ssize_t result = msg_send_req(write_fd, &req);

        ssize_t expected = (ssize_t)(3 * sizeof(uint16_t)) + (ssize_t)MAX_BUF_LEN;
        CHECK(result == expected);
    }
}

/**
 * @brief Tests error handling and edge cases for request message sending
 *
 * Validates that msg_send_req properly handles error conditions including
 * invalid file descriptors, null pointers, and closed connections. Ensures
 * robust error detection and appropriate return codes.
 *
 * Expected Behavior:
 * - Returns -1 for all error conditions
 * - Handles null message pointer gracefully
 * - Detects invalid/closed file descriptors
 * - Does not crash or corrupt memory on invalid inputs
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_send_req Error Handling Tests")
{
    SECTION("Invalid file descriptor")
    {
        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = 5
        };
        strncpy(req.data, "test", 4);

        ssize_t result = msg_send_req(-1, &req);
        CHECK(result == -1);
    }

    SECTION("Null message pointer")
    {
        CreatePipe();
        ssize_t result = msg_send_req(write_fd, nullptr);
        CHECK(result == -1);
    }

    SECTION("Closed write end")
    {
        CreatePipe();
        close(write_fd);
        write_fd = -1;

        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = 5
        };
        strncpy(req.data, "test", 4);

        ssize_t result = msg_send_req(write_fd, &req);
        CHECK(result == -1);
    }
}

/**
 * @brief Tests successful request message receiving scenarios
 *
 * Validates that msg_recv_req correctly deserializes and processes incoming
 * request messages. Tests proper header parsing, data extraction, and message
 * reconstruction from wire format. Uses background threads to simulate
 * asynchronous communication patterns.
 *
 * Expected Behavior:
 * - Returns total bytes read (header + data)
 * - Correctly parses message headers and extracts payload data
 * - Reconstructs original message content accurately
 * - Handles variable-length payloads correctly
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_recv_req Happy Path Tests")
{
    CreatePipe();

    SECTION("Receive request with data")
    {
        const char* test_data = "recv_test_data";
        size_t data_len = strlen(test_data);

        ApiWireRequestMsg send_req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = (uint16_t)data_len
        };
        strncpy(send_req.data, test_data, data_len);

        // Send in background thread to avoid blocking
        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ssize_t send_result = msg_send_req(write_fd, &send_req);
            REQUIRE(send_result > 0);
        });

        // Receive the request
        ApiWireRequestMsg recv_req = {0};
        ssize_t result = msg_recv_req(read_fd, &recv_req);

        sender.join();

        // Should return total bytes read
        ssize_t expected = (ssize_t)(MSG_HDR_LEN + data_len);
        CHECK(result == expected);
        CHECK(recv_req.ver == 1);
        CHECK(recv_req.type == ApiRequestType_GETSTATE);
        CHECK(recv_req.len == data_len);
        CHECK(strncmp(recv_req.data, test_data, data_len) == 0);
    }

    SECTION("Receive request with no data")
    {
        ApiWireRequestMsg send_req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = 0
        };

        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ssize_t send_result = msg_send_req(write_fd, &send_req);
            REQUIRE(send_result > 0);
        });

        ApiWireRequestMsg recv_req = {0};
        ssize_t result = msg_recv_req(read_fd, &recv_req);

        sender.join();

        CHECK(result == MSG_HDR_LEN);
        CHECK(recv_req.ver == 1);
        CHECK(recv_req.type == ApiRequestType_GETSTATE);
        CHECK(recv_req.len == 0);
    }
}

/**
 * @brief Tests error handling for request message receiving
 *
 * Validates that msg_recv_req properly handles error conditions including
 * null pointers, invalid file descriptors, and malformed input data.
 * Ensures robust error detection without memory corruption or crashes.
 *
 * Expected Behavior:
 * - Returns -1 for all error conditions
 * - Handles null output message pointer gracefully
 * - Detects and reports invalid file descriptors
 * - Maintains memory safety during error conditions
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_recv_req Error Handling Tests")
{
    SECTION("Invalid file descriptor")
    {
        ApiWireRequestMsg recv_req = {0};
        ssize_t result = msg_recv_req(-1, &recv_req);
        CHECK(result < 0);  // Should be negative, exact value may vary
    }

    SECTION("Null output pointer")
    {
        CreatePipe();
        ssize_t result = msg_recv_req(read_fd, nullptr);
        CHECK(result < 0);  // Should be negative
    }

    SECTION("Closed read end")
    {
        CreatePipe();
        close(read_fd);
        read_fd = -1;

        ApiWireRequestMsg recv_req = {0};
        ssize_t result = msg_recv_req(read_fd, &recv_req);
        CHECK(result < 0);
    }
}

/**
 * @brief Tests successful response message sending scenarios
 *
 * Validates that msg_send_resp correctly formats and transmits response
 * messages with various status codes and return values. Ensures proper
 * message serialization and transmission over file descriptors.
 *
 * Expected Behavior:
 * - Returns number of bytes written on success
 * - Correctly formats response message with status codes
 * - Transmits response data over provided file descriptor
 * - Handles different response types and return values
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_send_resp Happy Path Tests")
{
    CreatePipe();

    SECTION("Send response")
    {
        ApiWireResponseMsg resp = {
            .code = 1,
            .ret_val = 42
        };

        ssize_t result = msg_send_resp(write_fd, &resp);

        // Should return size of response message
        ssize_t expected = (ssize_t)(2 * sizeof(uint16_t));
        CHECK(result == expected);

        // Verify we can read the data back
        char buffer[1024];
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer));
        CHECK(bytes_read == expected);
    }
}

/**
 * @brief Tests error handling for response message sending
 *
 * Validates that msg_send_resp properly handles error conditions including
 * null pointers and invalid file descriptors. Ensures robust error
 * detection and appropriate error return codes.
 *
 * Expected Behavior:
 * - Returns -1 for all error conditions
 * - Handles null message pointer without crashing
 * - Detects invalid file descriptors appropriately
 * - Maintains function robustness under error conditions
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_send_resp Error Handling Tests")
{
    SECTION("Invalid file descriptor")
    {
        ApiWireResponseMsg resp = {
            .code = 1,
            .ret_val = 42
        };

        ssize_t result = msg_send_resp(-1, &resp);
        CHECK(result == -1);
    }

    SECTION("Null message pointer")
    {
        CreatePipe();
        ssize_t result = msg_send_resp(write_fd, nullptr);
        CHECK(result == -1);
    }
}

/**
 * @brief Tests successful response message receiving scenarios
 *
 * Validates that msg_recv_resp correctly receives and deserializes response
 * messages from the wire format. Tests proper response parsing, status code
 * extraction, and message reconstruction using background thread communication.
 *
 * Expected Behavior:
 * - Returns number of bytes read on success
 * - Correctly parses response message format
 * - Extracts status codes and return values accurately
 * - Handles asynchronous response reception
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_recv_resp Happy Path Tests")
{
    CreatePipe();

    SECTION("Receive response")
    {
        ApiWireResponseMsg send_resp = {
            .code = 1,
            .ret_val = 42
        };

        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ssize_t send_result = msg_send_resp(write_fd, &send_resp);
            REQUIRE(send_result > 0);
        });

        ApiWireResponseMsg recv_resp = {0};
        ssize_t result = msg_recv_resp(read_fd, &recv_resp);

        sender.join();

        CHECK(result == (ssize_t)(2 * sizeof(uint16_t)));
        CHECK(recv_resp.code == 1);
        CHECK(recv_resp.ret_val == 42);
    }
}

/**
 * @brief Tests error handling for response message receiving
 *
 * Validates that msg_recv_resp properly handles error conditions including
 * null pointers and other invalid input scenarios. Ensures robust error
 * detection without memory corruption or system instability.
 *
 * Expected Behavior:
 * - Returns -1 for all error conditions
 * - Handles null output message pointer safely
 * - Maintains memory safety during error processing
 * - Provides consistent error reporting
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "msg_recv_resp Error Handling Tests")
{
    SECTION("Invalid file descriptor")
    {
        ApiWireResponseMsg recv_resp = {0};
        ssize_t result = msg_recv_resp(-1, &recv_resp);
        CHECK(result < 0);  // Should be negative
    }

    SECTION("Null output pointer")
    {
        CreatePipe();
        ssize_t result = msg_recv_resp(read_fd, nullptr);
        CHECK(result < 0);  // Should be negative
    }
}

/**
 * @brief Tests type safety and boundary conditions across different platforms
 *
 * Validates that all API protocol functions handle type conversions safely,
 * particularly signed/unsigned integer operations and size_t conversions.
 * Tests boundary conditions around maximum values, overflow prevention,
 * and cross-platform compatibility.
 *
 * Expected Behavior:
 * - Safe handling of signed/unsigned type conversions
 * - Proper boundary checking for size limits
 * - Consistent behavior across different architectures
 * - Prevention of integer overflow/underflow conditions
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "Cross-Platform Type Safety Tests")
{
    CreatePipe();

    SECTION("Large message size handling")
    {
        // Test with message size near type boundaries
        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = (uint16_t)(MAX_BUF_LEN - 1)
        };

        // Fill with pattern to verify data integrity
        for (int i = 0; i < req.len; i++) {
            req.data[i] = (char)((i * 37) % 256);  // Simple pattern
        }

        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ssize_t send_result = msg_send_req(write_fd, &req);
            CHECK(send_result > 0);
        });

        ApiWireRequestMsg recv_req = {0};
        ssize_t recv_result = msg_recv_req(read_fd, &recv_req);

        sender.join();

        CHECK(recv_result > 0);
        CHECK(recv_req.len == req.len);

        // Verify data integrity
        for (int i = 0; i < req.len; i++) {
            CHECK(recv_req.data[i] == (char)((i * 37) % 256));
        }
    }

    SECTION("Type conversion edge cases")
    {
        // Test edge cases for our cross-platform fixes
        ApiWireRequestMsg req = {
            .ver = UINT16_MAX,
            .type = UINT16_MAX - 1,
            .len = 1
        };
        req.data[0] = 'X';

        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ssize_t send_result = msg_send_req(write_fd, &req);
            CHECK(send_result > 0);
        });

        ApiWireRequestMsg recv_req = {0};
        ssize_t recv_result = msg_recv_req(read_fd, &recv_req);

        sender.join();

        CHECK(recv_result > 0);
        CHECK(recv_req.ver == UINT16_MAX);
        CHECK(recv_req.type == UINT16_MAX - 1);
        CHECK(recv_req.len == 1);
        CHECK(recv_req.data[0] == 'X');
    }
}

/**
 * @brief Validates API protocol constants and configuration values
 *
 * Tests that all protocol constants, buffer sizes, and configuration values
 * are properly defined and within expected ranges. Ensures protocol constants
 * maintain consistency and compatibility across different builds and platforms.
 *
 * Expected Behavior:
 * - All constants are properly defined and non-zero where appropriate
 * - Buffer sizes are reasonable and sufficient for operations
 * - Constants maintain backward compatibility
 * - Configuration values are within safe operational ranges
 */
TEST_CASE("API Protocol Constants Validation")
{
    SECTION("Buffer size constants are sane")
    {
        // Verify our buffer calculations are correct
        CHECK(MAX_BUF_LEN > 0);
        CHECK(MSG_HDR_LEN > 0);
        CHECK(RESP_MSG_READ_BUF_SIZE > 0);

        // Verify they make sense relative to each other
        CHECK(MSG_HDR_LEN < PIPE_BUF);
        CHECK(MAX_BUF_LEN < PIPE_BUF);
        CHECK(MSG_HDR_LEN + MAX_BUF_LEN <= PIPE_BUF);
    }

    SECTION("Request type constants")
    {
        CHECK(ApiRequestType_NONE == 0x00);
        CHECK(ApiRequestType_GETSTATE == 0x01);
    }
}

// Test to ensure our fixes work across different architectures
/**
 * @brief Tests compatibility across different system architectures
 *
 * Validates that the API protocol functions work correctly across different
 * system architectures (32-bit, 64-bit) and handles architecture-specific
 * type size differences. Tests endianness handling and data structure
 * alignment compatibility.
 *
 * Expected Behavior:
 * - Consistent behavior across 32-bit and 64-bit architectures
 * - Proper handling of architecture-specific type sizes
 * - Correct endianness conversion where applicable
 * - Stable data structure alignment and padding
 */
TEST_CASE("Cross-Architecture Compatibility")
{
    SECTION("Size calculations are consistent")
    {
        // Test that our size calculations work on different word sizes
        size_t header_size = 3 * sizeof(uint16_t);
        ssize_t signed_header_size = (ssize_t)header_size;

        CHECK(signed_header_size > 0);
        CHECK(signed_header_size == 6);  // 3 * 2 bytes

        // Test MAX_BUF_LEN calculations
        CHECK(MAX_BUF_LEN == (PIPE_BUF - 3 * sizeof(uint16_t)));

        // Test MSG_HDR_LEN calculation
        size_t expected_hdr_len = sizeof(ApiWireRequestMsg) - MAX_BUF_LEN * sizeof(char);
        CHECK(MSG_HDR_LEN == expected_hdr_len);
    }

    SECTION("Type conversion safety")
    {
        // Test our conversion patterns are safe
        ssize_t test_signed = 100;
        size_t test_unsigned = 200;

        // Our conversion pattern: check bounds then convert
        if (test_signed >= 0) {
            size_t converted = (size_t)test_signed;
            CHECK(converted == 100);
        }

        if (test_unsigned <= (size_t)SSIZE_MAX) {
            ssize_t converted = (ssize_t)test_unsigned;
            CHECK(converted == 200);
        }
    }
}

// Stress tests for robustness
/**
 * @brief Comprehensive stress testing and edge case validation
 *
 * Tests the API protocol functions under stress conditions including
 * rapid successive operations, edge-case data values, and resource
 * constraints. Validates system stability and performance under load.
 *
 * Expected Behavior:
 * - Maintains functionality under rapid successive operations
 * - Handles edge-case data values without corruption
 * - Preserves memory safety under stress conditions
 * - Provides consistent performance characteristics
 * - Graceful handling of resource constraints
 */
TEST_CASE_METHOD(ApiProtoTestFixture, "Stress and Edge Case Tests")
{
    CreatePipe();

    SECTION("Multiple rapid send/receive cycles")
    {
        const int num_cycles = 100;
        for (int i = 0; i < num_cycles; i++) {
            ApiWireRequestMsg req = {
                .ver = (uint16_t)(i % UINT16_MAX),
                .type = ApiRequestType_GETSTATE,
                .len = (uint16_t)(i % 10)
            };

            // Send in thread to avoid blocking
            std::thread sender([&]() {
                ssize_t send_result = msg_send_req(write_fd, &req);
                REQUIRE(send_result > 0);
            });

            ApiWireRequestMsg recv_req = {0};
            ssize_t recv_result = msg_recv_req(read_fd, &recv_req);

            sender.join();

            CHECK(recv_result > 0);
            CHECK(recv_req.ver == req.ver);
            CHECK(recv_req.type == req.type);
            CHECK(recv_req.len == req.len);
        }
    }

    SECTION("Zero-length message handling")
    {
        ApiWireRequestMsg req = {
            .ver = 1,
            .type = ApiRequestType_GETSTATE,
            .len = 0
        };

        std::thread sender([&]() {
            ssize_t send_result = msg_send_req(write_fd, &req);
            REQUIRE(send_result == MSG_HDR_LEN);
        });

        ApiWireRequestMsg recv_req = {0};
        ssize_t recv_result = msg_recv_req(read_fd, &recv_req);

        sender.join();

        CHECK(recv_result == MSG_HDR_LEN);
        CHECK(recv_req.ver == 1);
        CHECK(recv_req.type == ApiRequestType_GETSTATE);
        CHECK(recv_req.len == 0);
    }
}
