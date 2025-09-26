
#include "aduc/apiproto.h"
#include "aduc/logging.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define MAX_SEL_ATTEMPTS 5
#define READ_UINT16(p) ntohs((uint16_t)((p)[0] | ((p)[1] << 8)))

ssize_t msg_send(int fd, const ApiWireResponseMsg* msg)
{
    return write(fd, msg, sizeof(ApiWireResponseMsg));
}

ssize_t msg_recv(int fd, ApiWireRequestMsg* out_msg)
{
    ssize_t bytes_read;
    char buf[MAX_BUF_LEN] = { 0 };
    char* p = buf;

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

    if ((bytes_read = read(fd, p, MSG_HDR_LEN)) == 0)
    {
        return 0; // EOF
    }
    if (bytes_read != MSG_HDR_LEN)
    {
        Log_Error("msg hdr: read only %zd of %zu bytes", bytes_read, MSG_HDR_LEN);
        return -1;
    }
    p += bytes_read;

    _Static_assert(sizeof(uint16_t) == 2, "uint16_t not 2 bytes");
    _Static_assert(MSG_HDR_LEN == 3 * sizeof(uint16_t), "MSG_HDR_LEN not 3 uint16_t");
    uint16_t v = READ_UINT16(buf);
    uint16_t t = READ_UINT16(buf + sizeof(uint16_t));
    uint16_t l = READ_UINT16(buf + 2 * sizeof(uint16_t));

    if (l > 0)
    {
        if ((bytes_read = read(fd, p, l)) != (ssize_t)l)
        {
            Log_Error("msg data: read only %zd of %zu bytes", bytes_read, l);
            return -1;
        }
        memcpy(out_msg->data, p, l);
        p += l;
    }
    out_msg->ver = v;
    out_msg->type = t;
    out_msg->len = l;

    return p - buf;
}
