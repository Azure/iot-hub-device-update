#include <aduc/apiproto.h>

#include <catch2/catch_all.hpp>

#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <unistd.h>

namespace
{
struct PipePair
{
    int readFd = -1;
    int writeFd = -1;

    PipePair()
    {
        int fds[2] = { -1, -1 };
        REQUIRE(pipe(fds) == 0);
        readFd = fds[0];
        writeFd = fds[1];
    }

    ~PipePair()
    {
        if (readFd >= 0)
        {
            close(readFd);
        }
        if (writeFd >= 0)
        {
            close(writeFd);
        }
    }
};
}

TEST_CASE("msg_send_req and msg_recv_req roundtrip with payload")
{
    PipePair pipePair;

    ApiWireRequestMsg sent{};
    sent.ver = 3;
    sent.type = ApiRequestType_GETSTATE;
    const std::string payload = "abc123";
    sent.len = static_cast<uint16_t>(payload.size());
    std::memcpy(sent.data, payload.data(), payload.size());

    const ssize_t bytesSent = msg_send_req(pipePair.writeFd, &sent);
    REQUIRE(bytesSent == MSG_HDR_LEN + static_cast<ssize_t>(payload.size()));

    ApiWireRequestMsg received{};
    const ssize_t bytesReceived = msg_recv_req(pipePair.readFd, &received);
    REQUIRE(bytesReceived == bytesSent);
    CHECK(received.ver == sent.ver);
    CHECK(received.type == sent.type);
    CHECK(received.len == sent.len);
    CHECK(std::memcmp(received.data, payload.data(), payload.size()) == 0);
}

TEST_CASE("msg_send_req and msg_recv_req handle header-only message")
{
    PipePair pipePair;

    ApiWireRequestMsg sent{};
    sent.ver = 1;
    sent.type = ApiRequestType_NONE;
    sent.len = 0;

    const ssize_t bytesSent = msg_send_req(pipePair.writeFd, &sent);
    REQUIRE(bytesSent == MSG_HDR_LEN);

    ApiWireRequestMsg received{};
    const ssize_t bytesReceived = msg_recv_req(pipePair.readFd, &received);
    REQUIRE(bytesReceived == MSG_HDR_LEN);
    CHECK(received.ver == sent.ver);
    CHECK(received.type == sent.type);
    CHECK(received.len == 0);
}

TEST_CASE("msg_recv_req rejects oversized payload length")
{
    PipePair pipePair;

    uint16_t header[3] = {
        htons(1),
        htons(ApiRequestType_GETSTATE),
        htons(static_cast<uint16_t>(MAX_BUF_LEN + 1))
    };

    REQUIRE(write(pipePair.writeFd, header, sizeof(header)) == static_cast<ssize_t>(sizeof(header)));

    ApiWireRequestMsg received{};
    CHECK(msg_recv_req(pipePair.readFd, &received) == -1);
}

TEST_CASE("msg_send_resp and msg_recv_resp roundtrip")
{
    PipePair pipePair;

    ApiWireResponseMsg sent{};
    sent.code = 200;
    sent.ret_val = 7;

    const ssize_t bytesSent = msg_send_resp(pipePair.writeFd, &sent);
    REQUIRE(bytesSent == RESP_MSG_READ_BUF_SIZE);

    ApiWireResponseMsg received{};
    const ssize_t bytesReceived = msg_recv_resp(pipePair.readFd, &received);
    REQUIRE(bytesReceived == RESP_MSG_READ_BUF_SIZE);
    CHECK(received.code == sent.code);
    CHECK(received.ret_val == sent.ret_val);
}

TEST_CASE("msg_recv_resp returns error for incomplete response")
{
    PipePair pipePair;

    uint16_t partial = htons(200);
    REQUIRE(write(pipePair.writeFd, &partial, sizeof(partial)) == static_cast<ssize_t>(sizeof(partial)));
    close(pipePair.writeFd);
    pipePair.writeFd = -1;

    ApiWireResponseMsg received{};
    CHECK(msg_recv_resp(pipePair.readFd, &received) == -1);
}

TEST_CASE("msg_recv_req returns again on timeout when no data")
{
    PipePair pipePair;
    ApiWireRequestMsg received{};

    CHECK(msg_recv_req(pipePair.readFd, &received) == MSGREV_AGAIN);
}

TEST_CASE("msg_recv_resp returns again on timeout when no data")
{
    PipePair pipePair;
    ApiWireResponseMsg received{};

    CHECK(msg_recv_resp(pipePair.readFd, &received) == MSGREV_AGAIN);
}

TEST_CASE("msg_send_req and msg_send_resp fail on invalid fd")
{
    ApiWireRequestMsg req{};
    req.ver = 1;
    req.type = ApiRequestType_GETSTATE;
    req.len = 0;

    ApiWireResponseMsg resp{};
    resp.code = 200;
    resp.ret_val = 0;

    CHECK(msg_send_req(-1, &req) == -1);
    CHECK(msg_send_resp(-1, &resp) == -1);
}
