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
#include <string.h>
#include <stdlib.h>
#include "flash_layout.h"
#include "app_header.h"
#include "app_ota.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UDS_CAN_REQUEST_ID 0x7E0U
#define UDS_CAN_RESPONSE_ID 0x7E8U
#define UDS_DID_SPEED_RPM 0xF100U
#define UDS_DID_STATE 0xF101U
#define UDS_NRC_SERVICE_NOT_SUPPORTED 0x11U
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED 0x12U
#define UDS_NRC_INCORRECT_LENGTH 0x13U
#define UDS_NRC_REQUEST_OUT_OF_RANGE 0x31U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
/* USER CODE BEGIN PFP */
static void CAN_Filter_Config(void);
static void Vehicle_Init(VehicleState_t *v);
static void Vehicle_Update(VehicleState_t *v, uint32_t dt_ms);
static uint8_t Vehicle_TransmitFrames(CAN_HandleTypeDef *hcanx, VehicleState_t *v);
static float Clampf(float val, float lo, float hi);
static void CAN_ProcessReception(CAN_HandleTypeDef *hc);
static void UDS_HandleRequest(CAN_HandleTypeDef *hc,const uint8_t *p,uint8_t n);
static void UDS_HandleResponse(const uint8_t *p,uint8_t n);
static uint8_t UDS_SendSF(CAN_HandleTypeDef *hc,uint16_t id,const uint8_t *p,uint8_t n);
static void UDS_SendNRC(CAN_HandleTypeDef *hc,uint8_t sid,uint8_t nrc);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static VehicleState_t vehicle;
static uint32_t last_sim_tick = 0;
static uint32_t last_tx_tick = 0;
uint32_t now = 0;
uint32_t dt = 0;

uint8_t ota_begin = 0;
extern uint8_t ota_active;
static volatile uint8_t uds_last_response[7];
static volatile uint8_t uds_last_response_len;
static volatile uint8_t uds_response_pending;
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
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  /* Filter must be configured before HAL_CAN_Start(), otherwise bxCAN
   * will not pass any received frames to a FIFO (harmless here since we
   * only transmit, but keep it in case you add a receive path later,
   * e.g. to accept OTA commands from the Smart Network node). */
  CAN_Filter_Config();

  Vehicle_Init(&vehicle);
  last_sim_tick = HAL_GetTick();
  last_tx_tick  = HAL_GetTick();

  if (HAL_CAN_Start(&hcan) != HAL_OK) { Error_Handler(); }

  /* Seed the pseudo-random generator differently each power-up using the
   * SysTick counter so simulated noise isn't identical every boot */
  srand(HAL_GetTick() + 12345);

//  CAN_TxHeaderTypeDef TxHeader;
//  uint8_t TxData[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
//  uint32_t TxMailbox;
//
//  TxHeader.StdId = 0x321;
//  TxHeader.ExtId = 0x01; // Not used in standard ID
//  TxHeader.IDE = CAN_ID_STD;
//  TxHeader.RTR = CAN_RTR_DATA;
//  TxHeader.DLC = 8;
//  TxHeader.TransmitGlobalTime = DISABLE;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
//	  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);

	  now = HAL_GetTick();
      CAN_ProcessReception(&hcan);

//	  if(HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
//	  {
//		  // Transmission request Error
//		  Error_Handler();
//	  }
//
////	  printf("Transmitted\r\n");
//
//	  HAL_Delay(1000);

		if (ota_begin == 1)
		{
			HAL_Delay(500);  // debounce
			ota_begin = 0;
		}

	  /* Update the simulated vehicle physics/state machine */
	  if ((now - last_sim_tick) >= SIM_UPDATE_PERIOD_MS)
	  {
		  dt = now - last_sim_tick;
		  last_sim_tick = now;
		  Vehicle_Update(&vehicle, dt);
	  }

	  /* Transmit the current snapshot on CAN at a fixed period */
	  if ((now - last_tx_tick) >= CAN_TX_PERIOD_MS)
	  {
		  last_tx_tick = now;

		  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); /* heartbeat LED */

		  if (Vehicle_TransmitFrames(&hcan, &vehicle) != 0)
		  {
			  /* Mailboxes full or a send failed - don't hard-fault the ECU
			   * over a single dropped telemetry frame, just flag it on PB9
			   * and try again next period. */
			  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
		  }
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

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

  /*Configure GPIO pin : PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  Configures bxCAN to accept all standard IDs into FIFO0.
  *         Required even for a transmit-only node: without an active
  *         filter, bxCAN silently drops anything it receives, which
  *         matters if you later add OTA command handling from the
  *         Smart Network node.
  * @retval None
  */
static void CAN_Filter_Config(void)
{
  CAN_FilterTypeDef sFilterConfig;

  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000; /* mask 0 = accept everything */
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14; /* only relevant on dual-CAN parts */

  if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}


static float Clampf(float val, float lo, float hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

static uint8_t UDS_SendSF(CAN_HandleTypeDef *hc,uint16_t id,const uint8_t *p,uint8_t n)
{
 CAN_TxHeaderTypeDef h={0};uint8_t d[8]={0};uint32_t mb;
 if(!p||!n||n>7U||HAL_CAN_GetTxMailboxesFreeLevel(hc)==0U)return 1U;
 h.StdId=id;h.IDE=CAN_ID_STD;h.RTR=CAN_RTR_DATA;h.DLC=8U;h.TransmitGlobalTime=DISABLE;
 d[0]=n;memcpy(&d[1],p,n);return HAL_CAN_AddTxMessage(hc,&h,d,&mb)==HAL_OK?0U:1U;
}
static void UDS_SendNRC(CAN_HandleTypeDef *hc,uint8_t sid,uint8_t nrc)
{uint8_t r[3]={0x7FU,sid,nrc};(void)UDS_SendSF(hc,UDS_CAN_RESPONSE_ID,r,3U);}
static void UDS_HandleRequest(CAN_HandleTypeDef *hc,const uint8_t *p,uint8_t n)
{
 uint8_t r[7]={0};uint16_t did;if(!p||!n)return;
 if(p[0]==0x10U){
  if(n!=2U)UDS_SendNRC(hc,p[0],UDS_NRC_INCORRECT_LENGTH);
  else if(p[1]!=1U&&p[1]!=3U)UDS_SendNRC(hc,p[0],UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
  else{r[0]=0x50U;r[1]=p[1];r[3]=0x32U;r[4]=1U;r[5]=0xF4U;(void)UDS_SendSF(hc,UDS_CAN_RESPONSE_ID,r,6U);}}
 else if(p[0]==0x3EU){
  if(n!=2U)UDS_SendNRC(hc,p[0],UDS_NRC_INCORRECT_LENGTH);
  else if((p[1]&0x7FU)!=0U)UDS_SendNRC(hc,p[0],UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
  else if((p[1]&0x80U)==0U){r[0]=0x7EU;(void)UDS_SendSF(hc,UDS_CAN_RESPONSE_ID,r,2U);}}
 else if(p[0]==0x22U){
  if(n!=3U){UDS_SendNRC(hc,p[0],UDS_NRC_INCORRECT_LENGTH);return;}
  did=((uint16_t)p[1]<<8)|p[2];r[0]=0x62U;r[1]=p[1];r[2]=p[2];
  if(did==UDS_DID_SPEED_RPM){uint16_t v=(uint16_t)(vehicle.speed_kmh*10.0f);r[3]=v>>8;r[4]=v;r[5]=vehicle.rpm>>8;r[6]=vehicle.rpm;(void)UDS_SendSF(hc,UDS_CAN_RESPONSE_ID,r,7U);}
  else if(did==UDS_DID_STATE){r[3]=vehicle.throttle_pct;r[4]=vehicle.engine_load_pct;r[5]=(uint8_t)(vehicle.coolant_temp_c+40);r[6]=(uint8_t)vehicle.cycle_state;(void)UDS_SendSF(hc,UDS_CAN_RESPONSE_ID,r,7U);}
  else UDS_SendNRC(hc,p[0],UDS_NRC_REQUEST_OUT_OF_RANGE);}
 else UDS_SendNRC(hc,p[0],UDS_NRC_SERVICE_NOT_SUPPORTED);
}
static void UDS_HandleResponse(const uint8_t *p,uint8_t n)
{uint8_t i;if(!p||!n||n>7U)return;for(i=0;i<n;i++)uds_last_response[i]=p[i];uds_last_response_len=n;uds_response_pending=1U;}


static void CAN_ProcessReception(CAN_HandleTypeDef *hc)
{
 CAN_RxHeaderTypeDef h;
 uint8_t d[8],n;

 while(HAL_CAN_GetRxFifoFillLevel(hc,CAN_RX_FIFO0)>0U)
 {
  if(HAL_CAN_GetRxMessage(hc,CAN_RX_FIFO0,&h,d)!=HAL_OK)
    break;
  if(h.IDE!=CAN_ID_STD||h.RTR!=CAN_RTR_DATA||h.DLC==0U||(d[0]&0xF0U)!=0U)
    continue;
  n=d[0]&0x0FU;
  if(!n||n>7U||n>(uint8_t)(h.DLC-1U))
    continue;
  if(h.StdId==UDS_CAN_REQUEST_ID)
    UDS_HandleRequest(hc,&d[1],n);
  else if(h.StdId==UDS_CAN_RESPONSE_ID)
    UDS_HandleResponse(&d[1],n);
 }
}

/* Initialize vehicle to a sane parked state */
static void Vehicle_Init(VehicleState_t *v)
{
    memset(v, 0, sizeof(*v));
    v->speed_kmh         = 0.0f;
    v->rpm                = 800;      /* idle rpm */
    v->throttle_pct       = 0;
    v->engine_load_pct    = 15;
    v->fuel_level_pct     = 85;
    v->fuel_mileage_kmpl  = 0.0f;
    v->odometer_km        = 128450;   /* pretend this car has some history */
    v->coolant_temp_c     = 22;       /* ambient, engine cold */
    v->battery_mv         = 12600;
    v->cycle_state         = CYCLE_IDLE;
    v->cycle_state_ticks   = 0;
    v->dtc_flags           = 0x0000;
}



/* Advances the drive-cycle state machine and derives all parameters from it.
 * This is a simple simulation, not a physical vehicle model - good enough to
 * produce meaningful, changing data for testing the CAN link / diagnostics node. */
static void Vehicle_Update(VehicleState_t *v, uint32_t dt_ms)
{
    v->cycle_state_ticks += dt_ms;

    /* Randomized dwell time before switching drive-cycle state */
    uint32_t dwell = CYCLE_MIN_DWELL_MS +
                      (rand() % (CYCLE_MAX_DWELL_MS - CYCLE_MIN_DWELL_MS));

    if (v->cycle_state_ticks >= dwell)
    {
        v->cycle_state_ticks = 0;
        v->cycle_state = (DriveCycleState_t)((v->cycle_state + 1) % CYCLE_COUNT);
    }

    /* Target speed/throttle per state - simple ramps toward a target */
    float target_speed;
    uint8_t target_throttle;

    switch (v->cycle_state)
    {
        case CYCLE_IDLE:
            target_speed = 0.0f;
            target_throttle = 0;
            break;
        case CYCLE_ACCEL:
            target_speed = 90.0f;
            target_throttle = 70;
            break;
        case CYCLE_CRUISE:
            target_speed = 90.0f;
            target_throttle = 30;
            break;
        case CYCLE_DECEL:
            target_speed = 30.0f;
            target_throttle = 5;
            break;
        case CYCLE_BRAKE:
        default:
            target_speed = 0.0f;
            target_throttle = 0;
            break;
    }

    /* Move speed toward target (simple first-order ramp) */
    float speed_step = (target_speed - v->speed_kmh) * 0.05f;
    v->speed_kmh = Clampf(v->speed_kmh + speed_step, 0.0f, 180.0f);

    /* Throttle follows target with a bit of jitter for realism */
    int16_t jitter = (rand() % 5) - 2; /* -2..+2 */
    int16_t new_throttle = (int16_t)target_throttle + jitter;
    v->throttle_pct = (uint8_t)Clampf((float)new_throttle, 0.0f, 100.0f);

    /* RPM roughly follows speed + throttle (fake gearing curve) */
    float rpm_f = 800.0f + (v->speed_kmh * 35.0f) + (v->throttle_pct * 10.0f);
    v->rpm = (uint16_t)Clampf(rpm_f, 700.0f, 6500.0f);

    /* Engine load loosely tracks throttle with some noise */
    int16_t load_noise = (rand() % 7) - 3;
    v->engine_load_pct = (uint8_t)Clampf((float)v->throttle_pct + load_noise, 5.0f, 100.0f);

    /* Coolant temp slowly rises to operating temperature (~90C) then holds, with tiny ripple */
    if (v->coolant_temp_c < 90)
    {
        v->coolant_temp_c += (dt_ms > 0) ? 1 : 0; /* crude warm-up ramp */
        if (v->coolant_temp_c > 90) v->coolant_temp_c = 90;
    }
    else
    {
        v->coolant_temp_c = 88 + (rand() % 5); /* 88-92C ripple */
    }

    /* Fuel level drains slowly with load, mileage accumulates with distance */
    float distance_km_this_tick = v->speed_kmh * ((float)dt_ms / 3600000.0f);
    v->odometer_km += 0; /* accumulate in whole km below via fractional tracker */

    static float odo_fraction = 0.0f;
    odo_fraction += distance_km_this_tick;
    if (odo_fraction >= 1.0f)
    {
        v->odometer_km += (uint32_t)odo_fraction;
        odo_fraction -= (uint32_t)odo_fraction;
    }

    /* Instantaneous fuel mileage: better at cruise, worse at accel/idle */
    if (v->speed_kmh < 1.0f)
    {
        v->fuel_mileage_kmpl = 0.0f; /* stationary = 0 km/l, consuming fuel doing nothing */
    }
    else
    {
        float efficiency = 22.0f - (v->throttle_pct * 0.12f); /* heavier throttle = worse mileage */
        v->fuel_mileage_kmpl = Clampf(efficiency, 4.0f, 22.0f);
    }

    /* Slowly drain fuel level based on load (very rough) */
    static float fuel_fraction = 0.0f;
    fuel_fraction += (v->engine_load_pct * (float)dt_ms) / 20000000.0f;
    if (fuel_fraction >= 1.0f && v->fuel_level_pct > 0)
    {
        v->fuel_level_pct -= 1;
        fuel_fraction -= 1.0f;
    }

    /* Battery voltage - slightly higher while running (alternator), noisy */
    v->battery_mv = (uint16_t)(13800 + (rand() % 400) - 200);

    /* Fake DTCs: raise a "low fuel" flag under 10%, "overheat" flag if temp spikes */
    v->dtc_flags = 0;
    if (v->fuel_level_pct < 10)   v->dtc_flags |= 0x0001; /* P0460-style low fuel */
    if (v->coolant_temp_c > 105)  v->dtc_flags |= 0x0002; /* P0217-style overheat */
    if (v->battery_mv < 12000)    v->dtc_flags |= 0x0004; /* undercharge */
}

/* Packs and transmits the four telemetry frames. Returns 0 on success. */
static uint8_t Vehicle_TransmitFrames(CAN_HandleTypeDef *hcanx, VehicleState_t *v)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;

    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.TransmitGlobalTime = DISABLE;

    /* Only send if a mailbox is actually free, to avoid silently dropping /
     * blocking - HAL_CAN_AddTxMessage would just fail otherwise */
    if (HAL_CAN_GetTxMailboxesFreeLevel(hcanx) == 0)
    {
        return 1;
    }

    /* --- Frame 1: Speed / RPM / Throttle / Engine load --- */
    {
        uint16_t speed_x10 = (uint16_t)(v->speed_kmh * 10.0f); /* 0.1 km/h resolution */

        TxData[0] = (uint8_t)(speed_x10 >> 8);
        TxData[1] = (uint8_t)(speed_x10 & 0xFF);
        TxData[2] = (uint8_t)(v->rpm >> 8);
        TxData[3] = (uint8_t)(v->rpm & 0xFF);
        TxData[4] = v->throttle_pct;
        TxData[5] = v->engine_load_pct;
        TxData[6] = 0x00; /* reserved */
        TxData[7] = 0x00; /* reserved */

        TxHeader.StdId = CAN_ID_SPEED_RPM;
        TxHeader.DLC = 8;
        if (HAL_CAN_AddTxMessage(hcanx, &TxHeader, TxData, &TxMailbox) != HAL_OK)
        {
            return 1;
        }
    }

    /* --- Frame 2: Fuel level / instantaneous mileage / odometer --- */
    {
        uint16_t mileage_x10 = (uint16_t)(v->fuel_mileage_kmpl * 10.0f); /* 0.1 km/l */

        TxData[0] = v->fuel_level_pct;
        TxData[1] = (uint8_t)(mileage_x10 >> 8);
        TxData[2] = (uint8_t)(mileage_x10 & 0xFF);
        TxData[3] = (uint8_t)(v->odometer_km >> 24);
        TxData[4] = (uint8_t)(v->odometer_km >> 16);
        TxData[5] = (uint8_t)(v->odometer_km >> 8);
        TxData[6] = (uint8_t)(v->odometer_km & 0xFF);
        TxData[7] = 0x00; /* reserved */

        TxHeader.StdId = CAN_ID_FUEL_ODO;
        TxHeader.DLC = 8;
        if (HAL_CAN_AddTxMessage(hcanx, &TxHeader, TxData, &TxMailbox) != HAL_OK)
        {
            return 1;
        }
    }

    /* --- Frame 3: Coolant temp / battery voltage / drive cycle state --- */
    {
        /* Encode temp with +40 offset like classic OBD-II PID 0x05 (0 = -40C) */
        uint8_t temp_encoded = (uint8_t)(v->coolant_temp_c + 40);

        TxData[0] = temp_encoded;
        TxData[1] = (uint8_t)(v->battery_mv >> 8);
        TxData[2] = (uint8_t)(v->battery_mv & 0xFF);
        TxData[3] = (uint8_t)v->cycle_state;
        TxData[4] = 0x00; /* reserved */
        TxData[5] = 0x00; /* reserved */
        TxData[6] = 0x00; /* reserved */
        TxData[7] = 0x00; /* reserved */

        TxHeader.StdId = CAN_ID_TEMP_CYCLE;
        TxHeader.DLC = 8;
        if (HAL_CAN_AddTxMessage(hcanx, &TxHeader, TxData, &TxMailbox) != HAL_OK)
        {
            return 1;
        }
    }

//    /* --- Frame 4: Diagnostic flags (sent for the Smart Network node's OTA diagnosis) --- */
//    {
//        TxData[0] = (uint8_t)(v->dtc_flags >> 8);
//        TxData[1] = (uint8_t)(v->dtc_flags & 0xFF);
//        TxData[2] = 0x00;
//        TxData[3] = 0x00;
//        TxData[4] = 0x00;
//        TxData[5] = 0x00;
//        TxData[6] = 0x00;
//        TxData[7] = 0x00;
//
//        TxHeader.StdId = CAN_ID_DIAG;
//        TxHeader.DLC = 8;
//        if (HAL_CAN_AddTxMessage(hcanx, &TxHeader, TxData, &TxMailbox) != HAL_OK)
//        {
//            return 1;
//        }
//    }

    return 0;
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
