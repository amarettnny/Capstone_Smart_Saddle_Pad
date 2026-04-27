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
#include <stdio.h>
/* USER CODE END Includes */

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

I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define ADS1015_ADDR        (0x48 << 1)
#define ADS1015_REG_CONV    0x00
#define ADS1015_REG_CONFIG  0x01
volatile int16_t g_ch0_raw = 0;
volatile int16_t g_ch1_raw = 0;
volatile int16_t g_ch2_raw = 0;
volatile int16_t g_ch3_raw = 0;
volatile int32_t g_ch0_mv = 0;
volatile int32_t g_ch1_mv = 0;
volatile int32_t g_ch2_mv = 0;
volatile int32_t g_ch3_mv = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Reads one channel from ADS1015 in single-ended mode, ±4.096V range
int16_t ADS1015_Read(uint8_t channel)
{
    if (channel > 3) return 0;

    // Build 16-bit config word:
    //   Bit 15:    OS = 1 (start single conversion)
    //   Bits 14:12 MUX = 100 + channel (single-ended: AINx vs GND)
    //   Bits 11:9  PGA = 001 (±4.096V range — covers 0..3.3V)
    //   Bit 8:     MODE = 1 (single-shot)
    //   Bits 7:5   DR = 100 (1600 SPS, default)
    //   Bit 4:     COMP_MODE = 0
    //   Bit 3:     COMP_POL = 0
    //   Bit 2:     COMP_LAT = 0
    //   Bits 1:0   COMP_QUE = 11 (disable comparator)
    uint16_t config = 0
        | (1U << 15)                        // start conversion
        | ((uint16_t)(0x4 | channel) << 12) // MUX: AINx vs GND
        | (0x1 << 9)                        // PGA = ±4.096V
        | (1U << 8)                         // single-shot mode
        | (0x4 << 5)                        // 1600 SPS
        | 0x3;                              // disable comparator

    // Write config to register 0x01 (3 bytes: pointer + config MSB + config LSB)
    uint8_t tx[3];
    tx[0] = ADS1015_REG_CONFIG;
    tx[1] = (uint8_t)(config >> 8);
    tx[2] = (uint8_t)(config & 0xFF);

    HAL_StatusTypeDef s1 = HAL_I2C_Master_Transmit(
        &hi2c2, ADS1015_ADDR, tx, 3, HAL_MAX_DELAY);
    if (s1 != HAL_OK) {
        printf("  [I2C write failed: %d]\r\n", (int)s1);
        return 0;
    }

    // Wait for conversion. At 1600 SPS, conversion takes ~625 us.
    // 2ms is plenty of margin.
    HAL_Delay(2);

    // Set pointer register to conversion register (0x00)
    uint8_t ptr = ADS1015_REG_CONV;
    HAL_StatusTypeDef s2 = HAL_I2C_Master_Transmit(
        &hi2c2, ADS1015_ADDR, &ptr, 1, HAL_MAX_DELAY);
    if (s2 != HAL_OK) {
        printf("  [I2C ptr write failed: %d]\r\n", (int)s2);
        return 0;
    }

    // Read 2 bytes of result
    uint8_t rx[2] = {0};
    HAL_StatusTypeDef s3 = HAL_I2C_Master_Receive(
        &hi2c2, ADS1015_ADDR, rx, 2, HAL_MAX_DELAY);
    if (s3 != HAL_OK) {
        printf("  [I2C read failed: %d]\r\n", (int)s3);
        return 0;
    }

    // Combine to 16-bit value, then shift right 4 (ADS1015 result is left-aligned)
    int16_t raw = (int16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    raw >>= 4;
    return raw;
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);
  printf("PGA = +/- 4.096V, single-shot, 1600 SPS\r\n\r\n");

  // Quick presence check — scan for the device at 0x48
  if (HAL_I2C_IsDeviceReady(&hi2c2, ADS1015_ADDR, 3, 100) == HAL_OK) {
      printf("ADS1015 detected at 0x48\r\n\r\n");
  } else {
      printf("WARNING: ADS1015 not responding at 0x48\r\n");
      printf("Check: VDD/GND, SDA/SCL wiring, ADDR pin tied to GND\r\n\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    int16_t ch0 = ADS1015_Read(0);
	    int16_t ch1 = ADS1015_Read(1);
	    int16_t ch2 = ADS1015_Read(2);
	    int16_t ch3 = ADS1015_Read(3);

	    // PGA = ±4.096V → 1 LSB = 2 mV
	    int32_t mv0 = (int32_t)ch0 * 2;
	    int32_t mv1 = (int32_t)ch1 * 2;
	    int32_t mv2 = (int32_t)ch2 * 2;
	    int32_t mv3 = (int32_t)ch3 * 2;

	    printf("CH0: %5d (%5ld mV)   CH1: %5d (%5ld mV)   CH2: %5d (%5ld mV)   CH3: %5d (%5ld mV)\r\n",
	           ch0, mv0, ch1, mv1, ch2, mv2, ch3, mv3);

	    HAL_Delay(200);

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

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV4;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00402D41;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(User_LED1_GPIO_Port, User_LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : User_button_Pin */
  GPIO_InitStruct.Pin = User_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(User_button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : User_LED1_Pin */
  GPIO_InitStruct.Pin = User_LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(User_LED1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
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
