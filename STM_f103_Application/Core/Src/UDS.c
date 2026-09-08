#include "uds_server.h"
#include <string.h>

#define POS_RESP_OFFSET     0x40

/* Sessions */

#define SESSION_DEFAULT     0x01
#define SESSION_PROGRAMMING 0x02
#define SESSION_EXTENDED    0x03

/* NRC */

#define NRC_GENERAL_REJECT                0x10
#define NRC_SERVICE_NOT_SUPPORTED         0x11
#define NRC_SUBFUNCTION_NOT_SUPPORTED     0x12
#define NRC_INVALID_FORMAT                0x13
#define NRC_CONDITIONS_NOT_CORRECT        0x22
#define NRC_REQUEST_OUT_OF_RANGE          0x31
#define NRC_SECURITY_DENIED               0x33
#define NRC_INVALID_KEY                   0x35

/* Services */

#define SID_DIAG_SESSION_CONTROL          0x10
#define SID_READ_DTC                      0x19
#define SID_READ_DID                      0x22
#define SID_SECURITY_ACCESS               0x27
#define SID_ROUTINE_CONTROL               0x31
#define SID_REQUEST_DOWNLOAD              0x34
#define SID_TRANSFER_DATA                 0x36
#define SID_TRANSFER_EXIT                 0x37
#define SID_WRITE_DID                     0x2E

/* DIDs */

#define DID_VIN                           0xF190
#define DID_SW_VERSION                    0xF187
#define DID_SERIAL_NUMBER                 0xF18C

typedef struct
{
    uint32_t address;
    uint32_t size;
    uint32_t received;

} DownloadContext_t;

/*------------------------------------------------------------------*/
/* GLOBALS */
/*------------------------------------------------------------------*/

static uint8_t CurrentSession;
static bool SecurityUnlocked;
static uint32_t SecuritySeed;

static DownloadContext_t DownloadCtx;

static char Vin[]      = "WAUZZZ12345678901";
static char SwVer[]    = "V1.0.0";
static char Serial[]   = "SN123456";

/*------------------------------------------------------------------*/
/* STUB FUNCTIONS */
/*------------------------------------------------------------------*/

static void Flash_Write(
        uint32_t address,
        const uint8_t* data,
        uint32_t length)
{
    (void)address;
    (void)data;
    (void)length;
}

static void Flash_Finalize(void)
{
}

static void Routine_Start(uint16_t routine)
{
    (void)routine;
}

static void Routine_Stop(uint16_t routine)
{
    (void)routine;
}

/*------------------------------------------------------------------*/
/* HELPERS */
/*------------------------------------------------------------------*/

static void UDS_NegativeResponse(
        uint8_t sid,
        uint8_t nrc,
        UdsMessage_t* resp)
{
    resp->data[0] = 0x7F;
    resp->data[1] = sid;
    resp->data[2] = nrc;
    resp->length = 3;
}

static uint16_t GetDid(
        const UdsMessage_t* req)
{
    return ((uint16_t)req->data[1] << 8) |
            req->data[2];
}

static uint32_t GenerateSeed(void)
{
    return 0x12345678;
}

static bool ValidateKey(
        uint32_t seed,
        uint32_t key)
{
    return ((seed ^ 0x55AA55AAU) == key);
}

/*------------------------------------------------------------------*/
/* SERVICE 0x10 */
/*------------------------------------------------------------------*/

static void HandleSessionControl(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint8_t session;

    if(req->length != 2)
    {
        UDS_NegativeResponse(
            SID_DIAG_SESSION_CONTROL,
            NRC_INVALID_FORMAT,
            resp);
        return;
    }

    session = req->data[1];

    switch(session)
    {
        case SESSION_DEFAULT:
        case SESSION_PROGRAMMING:
        case SESSION_EXTENDED:
            CurrentSession = session;
            break;

        default:
            UDS_NegativeResponse(
                    SID_DIAG_SESSION_CONTROL,
                    NRC_SUBFUNCTION_NOT_SUPPORTED,
                    resp);
            return;
    }

    resp->data[0] = 0x50;
    resp->data[1] = session;

    resp->data[2] = 0x00;
    resp->data[3] = 0x32;

    resp->data[4] = 0x01;
    resp->data[5] = 0xF4;

    resp->length = 6;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x27 */
/*------------------------------------------------------------------*/

static void HandleSecurityAccess(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint8_t sub;

    sub = req->data[1];

    if(sub == 0x01)
    {
        SecuritySeed = GenerateSeed();

        resp->data[0] = 0x67;
        resp->data[1] = 0x01;

        resp->data[2] = (SecuritySeed >> 24);
        resp->data[3] = (SecuritySeed >> 16);
        resp->data[4] = (SecuritySeed >> 8);
        resp->data[5] = SecuritySeed;

        resp->length = 6;
    }
    else if(sub == 0x02)
    {
        uint32_t key;

        key =
            ((uint32_t)req->data[2] << 24) |
            ((uint32_t)req->data[3] << 16) |
            ((uint32_t)req->data[4] << 8 ) |
            ((uint32_t)req->data[5]);

        if(ValidateKey(SecuritySeed,key))
        {
            SecurityUnlocked = true;

            resp->data[0] = 0x67;
            resp->data[1] = 0x02;
            resp->length = 2;
        }
        else
        {
            UDS_NegativeResponse(
                    SID_SECURITY_ACCESS,
                    NRC_INVALID_KEY,
                    resp);
        }
    }
}

/*------------------------------------------------------------------*/
/* SERVICE 0x22 */
/*------------------------------------------------------------------*/

static void HandleReadDID(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint16_t did = GetDid(req);

    resp->data[0] = 0x62;
    resp->data[1] = req->data[1];
    resp->data[2] = req->data[2];

    switch(did)
    {
        case DID_VIN:

            memcpy(
                &resp->data[3],
                Vin,
                strlen(Vin));

            resp->length = 3 + strlen(Vin);
            break;

        case DID_SW_VERSION:

            memcpy(
                &resp->data[3],
                SwVer,
                strlen(SwVer));

            resp->length = 3 + strlen(SwVer);
            break;

        case DID_SERIAL_NUMBER:

            memcpy(
                &resp->data[3],
                Serial,
                strlen(Serial));

            resp->length = 3 + strlen(Serial);
            break;

        default:

            UDS_NegativeResponse(
                    SID_READ_DID,
                    NRC_REQUEST_OUT_OF_RANGE,
                    resp);
            break;
    }
}

/*------------------------------------------------------------------*/
/* SERVICE 0x2E */
/*------------------------------------------------------------------*/

static void HandleWriteDID(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint16_t did = GetDid(req);

    if(SecurityUnlocked == false)
    {
        UDS_NegativeResponse(
                SID_WRITE_DID,
                NRC_SECURITY_DENIED,
                resp);
        return;
    }

    switch(did)
    {
        case DID_SERIAL_NUMBER:

            memset(Serial,0,sizeof(Serial));

            memcpy(
                Serial,
                &req->data[3],
                req->length-3);

            break;

        default:

            UDS_NegativeResponse(
                    SID_WRITE_DID,
                    NRC_REQUEST_OUT_OF_RANGE,
                    resp);
            return;
    }

    resp->data[0] = 0x6E;
    resp->data[1] = req->data[1];
    resp->data[2] = req->data[2];
    resp->length = 3;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x19 */
/*------------------------------------------------------------------*/

static void HandleReadDTC(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint8_t subfunc;

    subfunc = req->data[1];

    if(subfunc != 0x02)
    {
        UDS_NegativeResponse(
                SID_READ_DTC,
                NRC_SUBFUNCTION_NOT_SUPPORTED,
                resp);
        return;
    }

    resp->data[0] = 0x59;
    resp->data[1] = 0x02;

    resp->data[2] = 0x12;
    resp->data[3] = 0x34;
    resp->data[4] = 0x56;
    resp->data[5] = 0x0F;

    resp->length = 6;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x31 */
/*------------------------------------------------------------------*/

static void HandleRoutineControl(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint8_t controlType;
    uint16_t routine;

    controlType = req->data[1];

    routine =
            ((uint16_t)req->data[2] << 8) |
             req->data[3];

    switch(controlType)
    {
        case 0x01:
            Routine_Start(routine);
            break;

        case 0x02:
            Routine_Stop(routine);
            break;

        case 0x03:
            break;

        default:
            UDS_NegativeResponse(
                    SID_ROUTINE_CONTROL,
                    NRC_SUBFUNCTION_NOT_SUPPORTED,
                    resp);
            return;
    }

    resp->data[0] = 0x71;
    resp->data[1] = controlType;
    resp->data[2] = req->data[2];
    resp->data[3] = req->data[3];
    resp->length = 4;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x34 */
/*------------------------------------------------------------------*/

static void HandleRequestDownload(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    if(CurrentSession != SESSION_PROGRAMMING)
    {
        UDS_NegativeResponse(
                SID_REQUEST_DOWNLOAD,
                NRC_CONDITIONS_NOT_CORRECT,
                resp);
        return;
    }

    DownloadCtx.address =
        ((uint32_t)req->data[3] << 24) |
        ((uint32_t)req->data[4] << 16) |
        ((uint32_t)req->data[5] << 8)  |
        ((uint32_t)req->data[6]);

    DownloadCtx.size =
        ((uint32_t)req->data[7] << 24) |
        ((uint32_t)req->data[8] << 16) |
        ((uint32_t)req->data[9] << 8)  |
        ((uint32_t)req->data[10]);

    DownloadCtx.received = 0;

    resp->data[0] = 0x74;
    resp->data[1] = 0x20;
    resp->data[2] = 0xF0;

    resp->length = 3;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x36 */
/*------------------------------------------------------------------*/

static void HandleTransferData(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    uint8_t blockCounter;

    blockCounter = req->data[1];

    Flash_Write(
        DownloadCtx.address + DownloadCtx.received,
        &req->data[2],
        req->length - 2);

    DownloadCtx.received +=
        (req->length - 2);

    resp->data[0] = 0x76;
    resp->data[1] = blockCounter;
    resp->length = 2;
}

/*------------------------------------------------------------------*/
/* SERVICE 0x37 */
/*------------------------------------------------------------------*/

static void HandleTransferExit(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    (void)req;

    Flash_Finalize();

    resp->data[0] = 0x77;
    resp->length = 1;
}

/*------------------------------------------------------------------*/
/* API */
/*------------------------------------------------------------------*/

void UDS_Init(void)
{
    CurrentSession = SESSION_DEFAULT;
    SecurityUnlocked = false;
    SecuritySeed = 0;

    memset(&DownloadCtx,0,sizeof(DownloadCtx));
}

void UDS_ProcessRequest(
        const UdsMessage_t* req,
        UdsMessage_t* resp)
{
    memset(resp,0,sizeof(UdsMessage_t));

    switch(req->data[0])
    {
        case SID_DIAG_SESSION_CONTROL:
            HandleSessionControl(req,resp);
            break;

        case SID_SECURITY_ACCESS:
            HandleSecurityAccess(req,resp);
            break;

        case SID_READ_DID:
            HandleReadDID(req,resp);
            break;

        case SID_WRITE_DID:
            HandleWriteDID(req,resp);
            break;

        case SID_READ_DTC:
            HandleReadDTC(req,resp);
            break;

        case SID_ROUTINE_CONTROL:
            HandleRoutineControl(req,resp);
            break;

        case SID_REQUEST_DOWNLOAD:
            HandleRequestDownload(req,resp);
            break;

        case SID_TRANSFER_DATA:
            HandleTransferData(req,resp);
            break;

        case SID_TRANSFER_EXIT:
            HandleTransferExit(req,resp);
            break;

        default:
            UDS_NegativeResponse(
                    req->data[0],
                    NRC_SERVICE_NOT_SUPPORTED,
                    resp);
            break;
    }
}