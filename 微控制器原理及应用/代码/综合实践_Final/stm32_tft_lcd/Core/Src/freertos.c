/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "GUI.h"
#include "WindowDLG.h"
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
/* USER CODE BEGIN Variables */
osThreadId USBTaskHandle;
osThreadId PoolButtonTaskHandle;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osMutexId myMutex01Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void USBTask(void const * argument);
void PoolButtonTask(void const * argument);

void GUI_Button_SignalEvent(void);
void GUI_Button_WaitEvent(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);



extern void MX_USB_HOST_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of myMutex01 */
  osMutexDef(myMutex01);
  myMutex01Handle = osMutexCreate(osMutex(myMutex01));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
//   osMutex = osMutexCreate(osMutex(MUTEX));
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(usbTask, USBTask, osPriorityNormal, 0, 1024);
  USBTaskHandle = osThreadCreate(osThread(usbTask), NULL);

  osThreadDef(poolbuttonTask, PoolButtonTask, osPriorityNormal, 0, 128);
  PoolButtonTaskHandle = osThreadCreate(osThread(poolbuttonTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  GUI_Init();
  CreateWindow();
  GUI_CURSOR_Show();

   /* Infinite loop */
  for(;;)
  {
	GUI_Exec();
	GUI_X_ExecIdle();
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void USBTask (void const * argument)
{
	/* init code for USB_HOST */
	MX_USB_HOST_Init();

	for(;;)
	{
	  osDelay(1);
	}
}

void PoolButtonTask (void const * argument)
{
	GUI_SetSignalEventFunc(GUI_Button_SignalEvent);
	GUI_SetWaitEventFunc(GUI_Button_WaitEvent);

	for(;;)
	{
	  if (HAL_GPIO_ReadPin (KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) // Button press
	  {
		  GUI_Button_SignalEvent();
	  }
	  if (HAL_GPIO_ReadPin (KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) // Button press
	  {
		  GUI_Button_SignalEvent();
	  }

	  osDelay(1);
	}
}

void GUI_Button_SignalEvent(void)
{

}

void GUI_Button_WaitEvent(void)
{

}


/* USER CODE END Application */
