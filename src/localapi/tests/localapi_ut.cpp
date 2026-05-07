/**
 * @file localapi_ut.cpp
 * @brief Unit tests for the Agent Local API server.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstring>
#include <thread>

extern "C"
{
#include "aduc/ipc_transport.h"
#include "aduc/localapi.h"

// Override weak symbol for testing
int localapi_get_current_status(void)
{
    return 9; // Idle
}
}

#ifdef _WIN32
static const char* TEST_ENDPOINT = "\\\\.\\pipe\\adu-localapi-server-test";
#else
static const char* TEST_ENDPOINT = "/tmp/adu-localapi-server-test.sock";
#endif

TEST_CASE("localapi - init and uninit", "[localapi][server]")
{
    LocalApiConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.timeoutMs = 1000;
    config.maxConnections = 2;
    config.maxRequestsPerSecond = 10;

    REQUIRE(localapi_init(&config));
    REQUIRE(localapi_is_running());

    REQUIRE(localapi_uninit());
    REQUIRE_FALSE(localapi_is_running());
}

TEST_CASE("localapi - double init fails", "[localapi][server]")
{
    LocalApiConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.timeoutMs = 1000;

    REQUIRE(localapi_init(&config));
    REQUIRE_FALSE(localapi_init(&config)); // Second init should fail

    localapi_uninit();
}

TEST_CASE("localapi - client can query status", "[localapi][server]")
{
    LocalApiConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.timeoutMs = 2000;

    REQUIRE(localapi_init(&config));

    // Give server thread time to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect as client
    IpcTransport* client = nullptr;
    IpcTransportResult rc = ipc_client_connect(TEST_ENDPOINT, 2000, &client);
    REQUIRE(rc == IPC_OK);

    // Send GETSTATE request: [ver=1][type=1][len=0]
    uint16_t request[3] = { 1, 1, 0 }; // ver=1, type=GETSTATE=1, len=0
    rc = ipc_send(client, request, sizeof(request));
    REQUIRE(rc == IPC_OK);

    // Receive response: [ver][status][len]
    uint16_t response[3] = {};
    size_t received = 0;
    rc = ipc_recv(client, response, sizeof(response), &received);
    REQUIRE(rc == IPC_OK);
    REQUIRE(received == 6);
    REQUIRE(response[0] == 1);  // ver
    REQUIRE(response[1] == 9);  // status = Idle
    REQUIRE(response[2] == 0);  // len

    ipc_transport_close(client);
    localapi_uninit();
}

TEST_CASE("localapi - invalid version gets 400", "[localapi][server]")
{
    LocalApiConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.timeoutMs = 2000;

    REQUIRE(localapi_init(&config));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    IpcTransport* client = nullptr;
    IpcTransportResult rc = ipc_client_connect(TEST_ENDPOINT, 2000, &client);
    REQUIRE(rc == IPC_OK);

    // Send request with bad version
    uint16_t request[3] = { 99, 1, 0 }; // ver=99 (invalid)
    rc = ipc_send(client, request, sizeof(request));
    REQUIRE(rc == IPC_OK);

    uint16_t response[3] = {};
    size_t received = 0;
    rc = ipc_recv(client, response, sizeof(response), &received);
    REQUIRE(rc == IPC_OK);
    REQUIRE(response[1] == 400); // Bad Request

    ipc_transport_close(client);
    localapi_uninit();
}

TEST_CASE("localapi - uninit without init is safe", "[localapi][server]")
{
    REQUIRE_FALSE(localapi_uninit());
}
