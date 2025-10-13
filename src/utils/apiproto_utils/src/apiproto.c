
#include "aduc/apiproto.h"
#include "aduc/logging.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_SEL_ATTEMPTS 5

#define READSHORT(OUT, EOFOK)                                                             \
    do                                                                                    \
    {                                                                                     \
        bytes_read = read(fd, &OUT, sizeof(OUT));                                         \
        if (bytes_read < 0)                                                               \
        {                                                                                 \
            return bytes_read;                                                            \
        }                                                                                 \
        if (bytes_read != sizeof(uint16_t))                                               \
        {                                                                                 \
            if (bytes_read == 0 && (EOFOK))                                               \
            {                                                                             \
                return 0;                                                                 \
            }                                                                             \
            if (errno == EINTR)                                                           \
            {                                                                             \
                continue;                                                                 \
            }                                                                             \
            Log_Error("READSHORT: read only %zd of %zu bytes", bytes_read, sizeof(OUT));  \
            return -1;                                                                    \
        }                                                                                 \
        if (bytes_read == 0)                                                              \
        {                                                                                 \
            return EOFOK ? 0 : -2;                                                        \
        }                                                                                 \
        Log_Debug("READSHORT: READ raw: %02x %02x", ((char*)&OUT)[0], ((char*)&OUT)[1]);  \
        OUT = ntohs(OUT);                                                                 \
        Log_Debug("READSHORT: NET->HOST: %02x %02x", ((char*)&OUT)[0], ((char*)&OUT)[1]); \
    } while (0)

#define WRITESHORT(VAL)                                                                            \
    do                                                                                             \
    {                                                                                              \
        Log_Debug("WRITESHORT: WRITE raw: %02x %02x", ((char*)&VAL)[0], ((char*)&VAL)[1]);         \
        uint16_t net_val = htons(VAL);                                                             \
        Log_Debug("WRITESHORT: WRITE net: %02x %02x", ((char*)&net_val)[0], ((char*)&net_val)[1]); \
        if (sizeof(uint16_t) != write(fd, &net_val, sizeof(uint16_t)))                             \
        {                                                                                          \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

ssize_t msg_send_req(int fd, const ApiWireRequestMsg* msg)
{
    char hdr_buf[3 * sizeof(uint16_t)];
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
    if (bytes_written != (ssize_t)(3 * sizeof(uint16_t)))
    {
        Log_Error("msg header: wrote %zd instead of %zu bytes", bytes_written, 3 * sizeof(uint16_t));
        return -1;
    }
    Log_Debug("msg_send_req: wrote %zd bytes of header", bytes_written);

    if (msg->len != 0)
    {
        Log_Debug("msg_send_req: writing %u bytes of .data", msg->len);
        ssize_t nbw = write(fd, msg->data, msg->len);
        if (nbw < 0)
        {
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

ssize_t msg_recv_req(int fd, ApiWireRequestMsg* out_msg)
{
    ssize_t bytes_read;

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
        return MSGREV_AGAIN;
    }

    uint16_t msg_ver = 0, msg_type = 0, msg_len = 0;

    READSHORT(msg_ver, false);
    Log_Debug("msg_recv_req: READ msg ver: %u", msg_ver);

    READSHORT(msg_type, false);
    Log_Debug("msg_recv_req: READ msg type: %u", msg_type);

    READSHORT(msg_len, false);
    Log_Debug("msg_recv_req: READ msg len: %u", msg_len);

    char msg_data[MAX_BUF_LEN] = { 0 };
    bytes_read = read(fd, msg_data, msg_len);

    char printed_bytes[MAX_BUF_LEN * 2 + 1] = { 0 };
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

    return 3 * sizeof(uint16_t) + msg_len;
}

ssize_t msg_send_resp(int fd, const ApiWireResponseMsg* msg)
{
    WRITESHORT(msg->code);
    WRITESHORT(msg->ret_val);
    return sizeof(sizeof(msg->code) + sizeof(msg->ret_val));
}

ssize_t msg_recv_resp(int fd, ApiWireResponseMsg* out_msg)
{
    ssize_t bytes_read;

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
        return MSGREV_AGAIN;
    }

    uint16_t msg_code = 0, msg_ret_val = 0;

    READSHORT(msg_code, false);
    Log_Debug("msg_recv_req: READ msg code: %u", msg_code);

    READSHORT(msg_ret_val, false);
    Log_Debug("msg_recv_req: READ msg ret_val: %u", msg_ret_val);

    out_msg->code = msg_code;
    out_msg->ret_val = msg_ret_val;

    return 2 * sizeof(uint16_t);
}
