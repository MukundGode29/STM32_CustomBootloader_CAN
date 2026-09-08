#ifndef UDS_SERVER_H
#define UDS_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#define UDS_MAX_REQ_LEN     4095
#define UDS_MAX_RESP_LEN    4095

typedef struct
{
    uint8_t data[UDS_MAX_REQ_LEN];
    uint16_t length;

} UdsMessage_t;

void UDS_Init(void);

void UDS_ProcessRequest(
        const UdsMessage_t* request,
        UdsMessage_t* response);

#endif