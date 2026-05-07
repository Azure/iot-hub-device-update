/**
 * @file ipc_transport_ut.cpp
 * @brief Unit tests for the IPC transport layer (cross-platform).
 *
 * These tests verify the transport layer works correctly on the current platform
 * without depending on FIFOs or any platform-specific filesystem features.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

extern "C"
{
#include "aduc/ipc_transport.h"
}

// Use a temp path that works on both Linux and Windows
#ifdef _WIN32
static const char* TEST_ENDPOINT = "\\\\.\\pipe\\adu-localapi-test";
#else
static const char* TEST_ENDPOINT = "/tmp/adu-localapi-test.sock";
#endif

TEST_CASE("ipc_transport - server create and close", "[localapi][ipc]")
{
    IpcTransport* server = nullptr;
    IpcServerConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.maxConnections = 2;
    config.recvTimeoutMs = 1000;
    config.sendTimeoutMs = 1000;

    IpcTransportResult rc = ipc_server_create(&config, &server);
    REQUIRE(rc == IPC_OK);
    REQUIRE(server != nullptr);

    ipc_transport_close(server);
}

TEST_CASE("ipc_transport - server create with null config fails", "[localapi][ipc]")
{
    IpcTransport* server = nullptr;
    IpcTransportResult rc = ipc_server_create(nullptr, &server);
    REQUIRE(rc == IPC_ERR_INVALID_ARG);
}

TEST_CASE("ipc_transport - client connect to nonexistent endpoint fails", "[localapi][ipc]")
{
    IpcTransport* client = nullptr;
#ifdef _WIN32
    const char* badEndpoint = "\\\\.\\pipe\\adu-nonexistent-test-pipe-xyz";
#else
    const char* badEndpoint = "/tmp/adu-nonexistent-test.sock";
#endif
    IpcTransportResult rc = ipc_client_connect(badEndpoint, 500, &client);
    REQUIRE(rc != IPC_OK);
    REQUIRE(client == nullptr);
}

TEST_CASE("ipc_transport - accept timeout when no client connects", "[localapi][ipc]")
{
    IpcTransport* server = nullptr;
    IpcServerConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.recvTimeoutMs = 200; // Short timeout

    IpcTransportResult rc = ipc_server_create(&config, &server);
    REQUIRE(rc == IPC_OK);

    IpcTransport* client = nullptr;
    rc = ipc_server_accept(server, &client);
    REQUIRE(rc == IPC_ERR_TIMEOUT);
    REQUIRE(client == nullptr);

    ipc_transport_close(server);
}

TEST_CASE("ipc_transport - client/server roundtrip", "[localapi][ipc]")
{
    IpcTransport* server = nullptr;
    IpcServerConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.recvTimeoutMs = 2000;
    config.sendTimeoutMs = 2000;

    IpcTransportResult rc = ipc_server_create(&config, &server);
    REQUIRE(rc == IPC_OK);

    std::atomic<bool> serverReady{ true };
    std::thread serverThread([&]() {
        IpcTransport* accepted = nullptr;
        IpcTransportResult acceptRc = ipc_server_accept(server, &accepted);
        if (acceptRc != IPC_OK)
        {
            return;
        }

        // Receive message
        char buf[64] = {};
        size_t received = 0;
        ipc_recv(accepted, buf, 5, &received);

        // Echo back with prefix
        char resp[64] = "RE:";
        memcpy(resp + 3, buf, received);
        ipc_send(accepted, resp, 3 + received);

        ipc_transport_close(accepted);
    });

    // Give server a moment to start accepting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client connects and sends
    IpcTransport* client = nullptr;
    rc = ipc_client_connect(TEST_ENDPOINT, 2000, &client);
    REQUIRE(rc == IPC_OK);
    REQUIRE(client != nullptr);

    const char* msg = "HELLO";
    rc = ipc_send(client, msg, 5);
    REQUIRE(rc == IPC_OK);

    // Receive response
    char response[64] = {};
    size_t received = 0;
    rc = ipc_recv(client, response, 8, &received);
    REQUIRE(rc == IPC_OK);
    REQUIRE(received == 8);
    REQUIRE(std::string(response, 8) == "RE:HELLO");

    ipc_transport_close(client);
    serverThread.join();
    ipc_transport_close(server);
}

TEST_CASE("ipc_transport - peer credentials are available", "[localapi][ipc]")
{
    IpcTransport* server = nullptr;
    IpcServerConfig config = {};
    config.endpoint = TEST_ENDPOINT;
    config.recvTimeoutMs = 2000;

    IpcTransportResult rc = ipc_server_create(&config, &server);
    REQUIRE(rc == IPC_OK);

    std::thread serverThread([&]() {
        IpcTransport* accepted = nullptr;
        IpcTransportResult acceptRc = ipc_server_accept(server, &accepted);
        if (acceptRc != IPC_OK)
        {
            return;
        }

        IpcPeerCredentials creds = {};
        IpcTransportResult credRc = ipc_get_peer_credentials(accepted, &creds);
        REQUIRE(credRc == IPC_OK);

#ifdef _WIN32
        REQUIRE(creds.processId > 0);
#else
        REQUIRE(creds.pid > 0);
        REQUIRE(creds.uid == (uint32_t)getuid());
#endif

        ipc_transport_close(accepted);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    IpcTransport* client = nullptr;
    rc = ipc_client_connect(TEST_ENDPOINT, 2000, &client);
    REQUIRE(rc == IPC_OK);

    // Keep connection open briefly for server to read credentials
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ipc_transport_close(client);
    serverThread.join();
    ipc_transport_close(server);
}

TEST_CASE("ipc_transport - close NULL is safe", "[localapi][ipc]")
{
    ipc_transport_close(nullptr); // Should not crash
}

TEST_CASE("ipc_transport - default endpoint is not null", "[localapi][ipc]")
{
    const char* ep = ipc_get_default_endpoint();
    REQUIRE(ep != nullptr);
    REQUIRE(strlen(ep) > 0);
}
