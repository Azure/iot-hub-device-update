/**
 * @file apiproto.c
 * @brief The implementation for SDK API wire protocol.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apiproto.h"
#include "aduc/c_utils.h"
#include "aduc/logging.h"

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
        return sizeof(uint16_t) * 3 + nbw;
    }
    Log_Debug("msg_send_req: wrote %zd bytes of data", bytes_written);
    return sizeof(uint16_t) * 3;
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
        sel_timeout.tv_usec = 150000; // 150 ms in microseconds

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

        br = read(fd, ((char*)header_buf) + bytes_read_total, (MSG_HDR_LEN) - bytes_read_total);
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
    char write_buf[2 * sizeof(uint16_t)] = { 0 };
    ssize_t bytes_written = -1;

    uint16_t v = htons(msg->code);
    memcpy(write_buf, &v, sizeof(uint16_t));
    v = htons(msg->ret_val);
    memcpy(write_buf + sizeof(uint16_t), &v, sizeof(uint16_t));

    bytes_written = write(fd, write_buf, sizeof(write_buf));
    if (bytes_written != sizeof(write_buf))
    {
        Log_Error("msg_send_resp: wrote %zd, expected %zu", bytes_written, sizeof(write_buf));
        return -1;
    }

    return bytes_written;
}

ssize_t msg_recv_resp(int fd, ApiWireResponseMsg* out_msg)
{
    ssize_t total_bytes_read = 0, bytes_read = -1, wait_res = -1;
    uint16_t msg_code = 0, msg_ret_val = 0;
    char read_buf[2 * sizeof(uint16_t)] = { 0 };

    while (total_bytes_read < (ssize_t)(2 * sizeof(uint16_t)))
    {
        if ((wait_res = _wait_until_bytes_avail_for_read(fd)) < 0)
        {
            return wait_res;
        }

        bytes_read = read(fd, read_buf + total_bytes_read, (2 * sizeof(uint16_t)) - total_bytes_read);
        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            Log_Error("msg_recv_resp: read error: %d (%s)", errno, strerror(errno));
            return -1;
        }
        if (bytes_read == 0)
        {
            break; // EOF
        }
        total_bytes_read += bytes_read;
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
