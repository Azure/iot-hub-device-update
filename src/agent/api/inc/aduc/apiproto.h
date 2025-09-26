#ifndef ADUC_APIPROTO_H_
#define ADUC_APIPROTO_H_

#include <stdint.h>
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <unistd.h>

#define PIPE_BUF 4096 // could properly use fcntl and F_GETPIPE_SZ and malloc, but this is simpler and good enough for now
#define MAX_BUF_LEN (PIPE_BUF - 3*sizeof(uint16_t))
#define MSG_HDR_LEN (sizeof(ApiWireRequestMsg) - MAX_BUF_LEN * sizeof(char))
#define MSGREV_AGAIN -37

#define ApiRequestType_NONE 0x00
#define ApiRequestType_GETSTATE 0x01

typedef struct tagApiWireRequestMsg
{
    uint16_t ver;
    uint16_t type;
    uint16_t len;
    char data[MAX_BUF_LEN];
} ApiWireRequestMsg;

typedef struct tagApiWireResponseMsg
{
    uint16_t code;
    uint16_t ret_val;
} ApiWireResponseMsg;

ssize_t msg_send(int fd, const ApiWireResponseMsg* msg);
ssize_t msg_recv(int fd, ApiWireRequestMsg* out_msg);

#endif // ADUC_APIPROTO_H_
