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
#include <stdbool.h>
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
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */
#define MY_SLOT_ID 0x01
#define RX_BUFFER_SIZE 1024
char rxBuffer[RX_BUFFER_SIZE];

typedef enum {
  SENSE = 0X01,
  ACTUATE = 0X02,
  OPTICS = 0X03,
  POWER = 0X04,
} SliceID;
SliceID activeSlice = SENSE;

const int POLL_CYCLE_COOLDOWN = 1000; // Time in milliseconds between each poll cycle to prevent spamming the loaf with requests
const int EXPORT_CYCLE_COOLDOWN = 3000; // Time in milliseconds between each export cycle to prevent spamming the loaf with exports

double S_temperature = 0.0;

typedef struct {
  SliceID owner;
  char varName[32];
  double valueNum;
  char valueStr[32];
  int tick;
} ImportedVariable;
ImportedVariable importedVariables[32] = {0}; // Array to hold imported variables from slices, with a max of 32 variables for simplicity
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool processCommand(const char* command)
{
  double value = 0.0;
  char varName[32] = {0};
  char valueStr[32] = {0};
  bool isString = false;


  if (sscanf(command, "%[^:]:\"%[^\"]\"", &varName, &valueStr) == 2) // Check for string value
  {
	  isString = true;
  } 
  else if (sscanf(command, "%[^:]:%lf", &varName, &value) == 2) // Check for numeric value
  {
	  isString = false;
  }

  for (int i = 0; i < 32; i++)
  {
      if (importedVariables[i].owner == 0) // Empty slot
      {
          importedVariables[i].owner = activeSlice;
          strncpy(importedVariables[i].varName, varName, sizeof(importedVariables[i].varName) - 1);
          importedVariables[i].valueNum = value;
          strncpy(importedVariables[i].valueStr, valueStr, sizeof(importedVariables[i].valueStr) - 1);
          importedVariables[i].tick = HAL_GetTick();
          break;
      }
      else if (strcmp(importedVariables[i].varName, varName) == 0) // Update existing variable
      {
          importedVariables[i].valueNum = value;
          strncpy(importedVariables[i].valueStr, valueStr, sizeof(importedVariables[i].valueStr) - 1);
          importedVariables[i].tick = HAL_GetTick();
          break;
      }
  }

  // You can add more handling for string values here
  // For demonstration, toggle LED and print info (if printf available)
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3); // Toggle an LED to indicate command processing
  // Debug print (optional):
  // printf("Var: %s, Value: %f, Str: %s, isString: %d\n", varName, value, valueStr, isString);
  return true;
}

void parseUARTBuffer(char *buffer, size_t len)
{
  char cmd[RX_BUFFER_SIZE];
  size_t cmdIndex = 0;
  bool inCommand = false;

  for (size_t i = 0; i < len; i++)
  {
      char c = buffer[i];

      if (c == '>')
      {
          // If we were already building a command, finalize it
          if (inCommand && cmdIndex > 0)
          {
              cmd[cmdIndex] = '\0';
              processCommand(cmd);
              cmdIndex = 0;
          }

          // Start a new command
          inCommand = true;
          buffer[i] = '\0'; // Clear processed command
      }
      else if (c == '\n')
      {
          // End of packet — flush any active command
          if (inCommand && cmdIndex > 0)
          {
              cmd[cmdIndex] = '\0';
              processCommand(cmd);
          }

          // Reset state
          inCommand = false;
          cmdIndex = 0;
          buffer[i] = '\0'; // Clear processed command
      }
      else if (inCommand)
      {
          // Accumulate command characters safely
          if (cmdIndex < (RX_BUFFER_SIZE - 1))
          {
              cmd[cmdIndex++] = c;
              buffer[i] = '\0'; // Clear processed command
          }
          else
          {
              // Overflow protection: discard oversized command
              cmdIndex = 0;
              inCommand = false;
          }
      }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
      // Process the received data in rxBuffer
      // Restart DMA reception
      HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxBuffer, RX_BUFFER_SIZE);
  }
}

void requestFromSlice(uint8_t sliceID)
{
  uint8_t rqstPacket[] = {0x99, sliceID}; // First byte is 0x99 to indicate a request, second byte is the slice ID
  activeSlice = sliceID;
  HAL_UART_Transmit(&huart1, (uint8_t *)rqstPacket, sizeof(rqstPacket), 1000);
  HAL_Delay(100); // Short delay to allow slice time to respond before sending another request
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxBuffer, RX_BUFFER_SIZE);

  int lastPollCycle = 0;
  int lastExportCycle = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    if (HAL_GetTick() - lastPollCycle > POLL_CYCLE_COOLDOWN)
    {
        lastPollCycle = HAL_GetTick();
        requestFromSlice(SENSE);
        requestFromSlice(ACTUATE);
        requestFromSlice(OPTICS);
        requestFromSlice(POWER);
    }

    if (HAL_GetTick() - lastExportCycle > EXPORT_CYCLE_COOLDOWN)
    {
        lastExportCycle = HAL_GetTick();
        for (int i = 0; i < 32; i++)
        {
          if (importedVariables[i].owner != 0) // If slot is occupied
          {
            char exportPacket[64];
            if (importedVariables[i].valueStr[0] != '\0') {
              // Export as string
              snprintf(exportPacket, sizeof(exportPacket), ">%s:\"%s\"\n", importedVariables[i].varName, importedVariables[i].valueStr);
            } else {
              // Export as number
              snprintf(exportPacket, sizeof(exportPacket), ">%s:%.2f\n", importedVariables[i].varName, importedVariables[i].valueNum);
            }
            HAL_UART_Transmit(&huart2, (uint8_t *)exportPacket, strlen(exportPacket), HAL_MAX_DELAY);
            HAL_Delay(1);
          }
        }
    }

    parseUARTBuffer(rxBuffer, RX_BUFFER_SIZE);

    HAL_Delay(5);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
  huart1.AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
