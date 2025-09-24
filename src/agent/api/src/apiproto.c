
#include "aduc/apiproto.h"
#include "aduc/logging.h"

ssize_t msg_send(int fd, const ApiWireResponseMsg* msg)
{
    return write(fd, msg, sizeof(ApiWireResponseMsg));
}

ssize_t msg_recv(int fd, ApiWireRequestMsg* out_msg)
{
    size_t len;
    ssize_t bytes_read;

    if ((bytes_read = read(fd, out_msg, MSG_HDR_LEN)) == 0)
    {
        return 0; // EOF
    }
    if (bytes_read != MSG_HDR_LEN)
    {
        Log_Error("msg hdr: read only %zd of %zu bytes", bytes_read, MSG_HDR_LEN);
        return -1;
    }

    if ((len = out_msg->len) > 0)
    {
        if ((bytes_read = read(fd, out_msg->data, len)) != (ssize_t)len)
        {
            Log_Error("msg data: read only %zd of %zu bytes", bytes_read, len);
            return -1;
        }
    }

    return len;
}
