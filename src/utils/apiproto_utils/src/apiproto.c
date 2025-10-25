/**
 * @file apiproto.c
 * @brief The implementation for SDK API wire protocol.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apiproto.h"
#include "aduc/c_utils.h"

#ifndef ADUC_APIPROTO_NO_LOGGING
#    include "aduc/logging.h"
#else
// Define no-op logging macros when logging is disabled
#    define Log_Error(...)
#    define Log_Warn(...)
#    define Log_Info(...)
#    define Log_Debug(...)
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_SEL_ATTEMPTS 5

ssize_t msg_send_req(int fd, const ApiWireRequestMsg* msg)
{
    // Validate input parameters
    if (msg == NULL)
    {
        Log_Error("msg_send_req: msg parameter is NULL");
        return -1;
    }

    STATIC_ASSERT(MSG_HDR_LEN == 3 * sizeof(uint16_t));
    char hdr_buf[MSG_HDR_LEN];
    uint16_t net_val = 0;
    net_val = htons(msg->ver);
    hdr_buf[0] = ((char*)&net_val)[0];
    hdr_buf[1] = ((char*)&net_val)[1];

    net_val = htons(msg->type);
    hdr_buf[2] = ((char*)&net_val)[0];
    hdr_buf[3] = ((char*)&net_val)[1];

    net_val = htons(msg->len);
    hdr_buf[4] = ((char*)&net_val)[0];
    hdr_buf[5] = ((char*)&net_val)[1];

    ssize_t bytes_written = write(fd, hdr_buf, sizeof(hdr_buf));
    if (bytes_written < 0)
    {
        return -1;
    }
    if (bytes_written != (ssize_t)(MSG_HDR_LEN))
    {
        Log_Error("msg header: wrote %zd instead of %zu bytes", bytes_written, MSG_HDR_LEN);
        return -1;
    }
    Log_Debug("msg_send_req: wrote %zd bytes of header", bytes_written);

    if (msg->len != 0)
    {
        Log_Debug("msg_send_req: writing %u bytes of .data", msg->len);
        ssize_t nbw = write(fd, msg->data, msg->len);
        if (nbw < 0)
        {
            Log_Error("msg data: write error: %d (%s)", errno, strerror(errno));
            return -1;
        }
        if (nbw != msg->len)
        {
            Log_Error("msg data: wrote only %zd of %u bytes", nbw, msg->len);
            return -1;
        }
        Log_Debug("msg_send_req: wrote %zd bytes of data", nbw);
        // Safe conversion: ensure nbw is non-negative before adding to sizeof result
        if (nbw < 0) {
            return -1;
        }
        return (ssize_t)(sizeof(uint16_t) * 3) + nbw;
    }
    Log_Debug("msg_send_req: wrote %zd bytes of data", bytes_written);
    return (ssize_t)(sizeof(uint16_t) * 3);
}

static ssize_t _wait_until_bytes_avail_for_read(int fd)
{
    fd_set read_fds;
    struct timeval sel_timeout;
    int rdy;
    int attempts = 0;
    while (attempts < MAX_SEL_ATTEMPTS)
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        sel_timeout.tv_sec = 0;
        sel_timeout.tv_usec = 500000; // 500 ms in microseconds (increased for ARM systems)

        rdy = select(fd + 1, &read_fds, NULL, NULL, &sel_timeout);
        if (rdy > 0 && FD_ISSET(fd, &read_fds))
        {
            break; // data is ready to read
        }

        if (rdy == 0)
        {
            // timeout
            return MSGREV_AGAIN;
        }
        else // if (rdy < 0)
        {
            if (errno == EINTR)
            {
                attempts++;
                continue;
            }
            Log_Error("select: %d ( %s )", errno, strerror(errno));
            return -1;
        }
    }
    if (attempts == MAX_SEL_ATTEMPTS)
    {
        Log_Debug("max select attempts reached");
        return MSGREV_AGAIN;
    }

    return 0;
}

ssize_t msg_recv_req(int fd, ApiWireRequestMsg* out_msg)
{
    // Validate input parameters
    if (out_msg == NULL)
    {
        Log_Error("msg_recv_req: out_msg parameter is NULL");
        return -1;
    }

    ssize_t bytes_read;
    uint16_t header_buf[MSG_HDR_LEN] = { 0 };
    uint16_t msg_ver = 0, msg_type = 0, msg_len = 0;
    ssize_t bytes_read_total = 0;
    ssize_t br = 0;
    ssize_t wait_res = 0;
    char printed_bytes[MAX_BUF_LEN * 2 + 1] = { 0 };

    char msg_data[MAX_BUF_LEN] = { 0 };

    while (bytes_read_total < (ssize_t)(MSG_HDR_LEN))
    {
        if ((wait_res = _wait_until_bytes_avail_for_read(fd)) < 0)
        {
            return wait_res;
        }

        // Safe conversion: calculate remaining bytes and ensure it's positive
        if (bytes_read_total >= (ssize_t)MSG_HDR_LEN) {
            Log_Error("msg_recv_req: bytes_read_total exceeds MSG_HDR_LEN");
            return -1;
        }
        size_t bytes_to_read = MSG_HDR_LEN - (size_t)bytes_read_total;
        br = read(fd, ((char*)header_buf) + bytes_read_total, bytes_to_read);
        if (br < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            Log_Error("msg header: read error: %d (%s)", errno, strerror(errno));
            return -1;
        }
        if (br == 0)
        {
            break; // EOF
        }
        bytes_read_total += br;
    }

    msg_ver = ntohs(header_buf[0]);
    msg_type = ntohs(header_buf[1]);
    msg_len = ntohs(header_buf[2]);

    if (msg_len > MAX_BUF_LEN)
    {
        Log_Error("msg length %u exceeds max %d", msg_len, MAX_BUF_LEN);
        return -1;
    }

    if (msg_len == 0)
    {
        out_msg->ver = msg_ver;
        out_msg->type = msg_type;
        out_msg->len = 0;
        return bytes_read_total;
    }

    if ((wait_res = _wait_until_bytes_avail_for_read(fd)) < 0)
    {
        return wait_res;
    }
    bytes_read = read(fd, msg_data, msg_len);

    for (size_t i = 0; i < (size_t)bytes_read && i < MAX_BUF_LEN; ++i)
    {
        snprintf(&printed_bytes[i * 2], 3, "%02x", (unsigned char)msg_data[i]);
    }
    Log_Debug("msg_recv_req: READ msg data: %s", printed_bytes);

    if (bytes_read < 0)
    {
        // Handle read errors
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            Log_Warn("msg data: read would block (EAGAIN/EWOULDBLOCK)");
            return MSGREV_AGAIN;
        }
        if (errno == EINTR)
        {
            Log_Warn("msg data: read interrupted (EINTR)");
            return MSGREV_AGAIN;
        }
        Log_Error("msg data: read error: %d (%s)", errno, strerror(errno));
        return -1;
    }
    if (bytes_read != (ssize_t)msg_len)
    {
        Log_Error("msg data: read only %zd of %u bytes (expected)", bytes_read, msg_len);
        return -1;
    }

    out_msg->ver = msg_ver;
    out_msg->type = msg_type;
    out_msg->len = msg_len;
    memcpy(out_msg->data, msg_data, msg_len);

    return bytes_read_total + bytes_read;
}

ssize_t msg_send_resp(int fd, const ApiWireResponseMsg* msg)
{
    // Validate input parameters
    if (msg == NULL)
    {
        Log_Error("msg_send_resp: msg parameter is NULL");
        return -1;
    }

    char write_buf[RESP_MSG_READ_BUF_SIZE] = { 0 };
    ssize_t bytes_written = -1;
    int retries = 0;
    const int MAX_WRITE_RETRIES = 10;

    uint16_t v = htons(msg->code);
    memcpy(write_buf, &v, sizeof(uint16_t));
    v = htons(msg->ret_val);
    memcpy(write_buf + sizeof(uint16_t), &v, sizeof(uint16_t));

retry_write:
    bytes_written = write(fd, write_buf, sizeof(write_buf));
    if (bytes_written < 0)
    {
        if ((errno == EAGAIN || errno == EWOULDBLOCK) && retries < MAX_WRITE_RETRIES)
        {
            retries++;
            usleep(10000); // 10ms
            goto retry_write;
        }
        Log_Error("msg_send_resp: write error: %d (%s)", errno, strerror(errno));
        return -1;
    }

    if (bytes_written != sizeof(write_buf))
    {
        Log_Error("msg_send_resp: wrote %zd, expected %zu", bytes_written, sizeof(write_buf));
        return -1;
    }

    return bytes_written;
}

ssize_t msg_recv_resp(int fd, ApiWireResponseMsg* out_msg)
{
    // Validate input parameters
    if (out_msg == NULL)
    {
        Log_Error("msg_recv_resp: out_msg parameter is NULL");
        return -1;
    }

    ssize_t total_bytes_read = 0, bytes_read = -1;
    uint16_t msg_code = 0, msg_ret_val = 0;
    char read_buf[RESP_MSG_READ_BUF_SIZE] = { 0 };
    int retries = 0;
    const int MAX_RETRIES = 20; // ~3 seconds total with 150ms sleeps

    // Single select call with longer timeout for entire message
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 3; // 3 second timeout
    timeout.tv_usec = 0;

    int rdy = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (rdy <= 0)
    {
        if (rdy == 0)
        {
            Log_Warn("msg_recv_resp: timeout waiting for response");
            return MSGREV_AGAIN;
        }
        Log_Error("msg_recv_resp: select error: %d (%s)", errno, strerror(errno));
        return -1;
    }

    // Now read without select in loop - data is available
    while (total_bytes_read < (ssize_t)(RESP_MSG_READ_BUF_SIZE) && retries < MAX_RETRIES)
    {
        // Safe conversion: calculate remaining bytes and ensure it's positive
        if (total_bytes_read >= (ssize_t)RESP_MSG_READ_BUF_SIZE) {
            Log_Error("msg_recv_resp: total_bytes_read exceeds RESP_MSG_READ_BUF_SIZE");
            return -1;
        }
        size_t bytes_to_read = RESP_MSG_READ_BUF_SIZE - (size_t)total_bytes_read;
        bytes_read = read(fd, read_buf + total_bytes_read, bytes_to_read);
        if (bytes_read < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                retries++;
                usleep(150000); // 150ms
                continue;
            }
            Log_Error("msg_recv_resp: read error: %d (%s)", errno, strerror(errno));
            return -1;
        }
        if (bytes_read == 0)
        {
            // EOF - writer closed before we got all data
            if (total_bytes_read == 0)
            {
                // No data yet, retry a few times for slow ARM systems
                retries++;
                usleep(150000); // 150ms
                continue;
            }
            break;
        }
        total_bytes_read += bytes_read;
    }

    if (total_bytes_read < RESP_MSG_READ_BUF_SIZE)
    {
        Log_Error("msg_recv_resp: incomplete message: %zd/%zu bytes", total_bytes_read, RESP_MSG_READ_BUF_SIZE);
        return -1;
    }

    memcpy(&msg_code, read_buf, sizeof(uint16_t));
    memcpy(&msg_ret_val, read_buf + sizeof(uint16_t), sizeof(uint16_t));

    Log_Debug("msg_recv_resp: READ raw: %02x %02x %02x %02x", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

    out_msg->ret_val = ntohs(msg_ret_val);
    out_msg->code = ntohs(msg_code);

    Log_Debug(
        "msg_recv_resp: NET->HOST: code %u (%02x %02x) ret_val %u (%02x %02x)",
        out_msg->code,
        ((char*)&out_msg->code)[0],
        ((char*)&out_msg->code)[1],
        out_msg->ret_val,
        ((char*)&out_msg->ret_val)[0],
        ((char*)&out_msg->ret_val)[1]);

    return total_bytes_read;
}
