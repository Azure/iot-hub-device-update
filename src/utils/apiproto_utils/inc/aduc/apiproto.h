/**
 * @file apiproto.h
 * @brief The header for
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_APIPROTO_H_
#define ADUC_APIPROTO_H_

#include "aduc/c_utils.h"
#include <stdint.h>
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <unistd.h>

EXTERN_C_BEGIN

#define PIPE_BUF \
    4096 // could properly use fcntl and F_GETPIPE_SZ and malloc, but this is simpler and good enough for now

// max data length in a message, leaving room for header (fields other than
//     char data[MAX_BUF_LEN]), which is 3 uint16_t fields (keep it synced)
// NOTE: for apiproto.c implementation, do not write struct padding to the pipe but only the actual fields,
// unless changing to remove padding using #pragma pack or equivalent. Instead, we've opted to keep it simple
// and calculate the header length and "manually" write those many bytes.
#define MAX_BUF_LEN (PIPE_BUF - 3 * sizeof(uint16_t))

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

ssize_t msg_send_req(int fd, const ApiWireRequestMsg* msg);
ssize_t msg_recv_req(int fd, ApiWireRequestMsg* out_msg);
ssize_t msg_send_resp(int fd, const ApiWireResponseMsg* msg);
ssize_t msg_recv_resp(int fd, ApiWireResponseMsg* out_msg);

EXTERN_C_END

#endif // ADUC_APIPROTO_H_
