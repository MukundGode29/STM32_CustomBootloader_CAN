/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "flash_layout.h"
#include "app_header.h"
#include "app_ota.h"

/* USER CODE END Includes */

/* USER CODE BEGIN UDS_DECLARATIONS */
/* Integrated from UDS.h */
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
/* USER CODE END UDS_DECLARATIONS */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static UdsMessage_t UdsRequest;
static UdsMessage_t UdsResponse;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void UDS_ApplicationTask(void);

/*
 * Transport adaptation hooks.
 * Replace these weak functions in the CAN ISO-TP, DoIP, or UART module.
 */
bool UDS_TransportReceive(uint8_t *data, uint16_t *length);
bool UDS_TransportTransmit(const uint8_t *data, uint16_t length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
__attribute__((section(".header"))) const app_header_t app_header =
{
	.ota_flag = 0,
    .magic   = 0xABCDEFAB,
    .size    = 7716,
    .crc     = 0xA8EB1242,
    .version = 0
};

uint32_t blinkLED = 0U;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint32_t wait = 10000;
	while (wait--);
	//blinkLED = 1;
	enable_ota_request();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SCB->VTOR = APP_START_ADDR;
  __enable_irq();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  UDS_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    UDS_ApplicationTask();

    /* Nonblocking application heartbeat. */
    if ((HAL_GetTick() - blinkLED) >= 2000U)
    {
      blinkLED = HAL_GetTick();
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief Polls the diagnostic transport, processes one UDS request, and sends
  *        the corresponding positive or negative response.
  * @note  Call this function continuously from the main loop.
  */
static void UDS_ApplicationTask(void)
{
  uint16_t receivedLength = 0U;

  if (UDS_TransportReceive(UdsRequest.data, &receivedLength) == false)
  {
    return; /* No complete UDS request is available. */
  }

  /* Reject invalid transport output before the UDS service dispatcher runs. */
  if ((receivedLength == 0U) || (receivedLength > UDS_MAX_REQ_LEN))
  {
    return;
  }

  UdsRequest.length = receivedLength;
  UdsResponse.length = 0U;

  UDS_ProcessRequest(&UdsRequest, &UdsResponse);

  if ((UdsResponse.length > 0U) &&
      (UdsResponse.length <= UDS_MAX_RESP_LEN))
  {
    (void)UDS_TransportTransmit(UdsResponse.data, UdsResponse.length);
  }
}

/**
  * @brief Returns one complete UDS payload received by the transport layer.
  * @param data Destination buffer with capacity UDS_MAX_REQ_LEN.
  * @param length Output payload length in bytes.
  * @retval true when one complete request was copied, otherwise false.
  *
  * Override this weak function in the communication module. For CAN, this
  * function should return a fully reassembled ISO-TP payload, not one CAN frame.
  */
__attribute__((weak)) bool UDS_TransportReceive(uint8_t *data, uint16_t *length)
{
  (void)data;

  if (length != NULL)
  {
    *length = 0U;
  }

  return false;
}

/**
  * @brief Sends one complete UDS response payload through the transport layer.
  * @param data Response payload.
  * @param length Number of response bytes.
  * @retval true when accepted for transmission, otherwise false.
  *
  * Override this weak function in the communication module. The CAN ISO-TP
  * implementation must segment responses that exceed one CAN frame.
  */
__attribute__((weak)) bool UDS_TransportTransmit(const uint8_t *data,
                                                 uint16_t length)
{
  (void)data;
  (void)length;
  return false;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/* USER CODE BEGIN UDS_IMPLEMENTATION */
/* Integrated from UDS.c.
 * UDS_ProcessRequest() is transport-independent. The CAN/ISO-TP/UART
 * receive layer should fill UdsMessage_t, call UDS_ProcessRequest(),
 * and transmit response.data[0..response.length-1].
 */
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
/* USER CODE END UDS_IMPLEMENTATION */
