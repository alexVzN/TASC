/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Encoder_CLK_Pin GPIO_PIN_0
#define Encoder_CLK_GPIO_Port GPIOA
#define Encoder_DT_Pin GPIO_PIN_1
#define Encoder_DT_GPIO_Port GPIOA
#define CH0_SSR_Pin GPIO_PIN_0
#define CH0_SSR_GPIO_Port GPIOB
#define CH1_SSR_Pin GPIO_PIN_1
#define CH1_SSR_GPIO_Port GPIOB
#define CH2_SSR_Pin GPIO_PIN_10
#define CH2_SSR_GPIO_Port GPIOB
#define CH3_SSR_Pin GPIO_PIN_11
#define CH3_SSR_GPIO_Port GPIOB
#define SPI_NMR_Pin GPIO_PIN_12
#define SPI_NMR_GPIO_Port GPIOB
#define SPI_NSTCP_Pin GPIO_PIN_14
#define SPI_NSTCP_GPIO_Port GPIOB
#define DIO_Pin GPIO_PIN_9
#define DIO_GPIO_Port GPIOA
#define Encoder_SW_Pin GPIO_PIN_15
#define Encoder_SW_GPIO_Port GPIOA
#define Encoder_SW_EXTI_IRQn EXTI15_10_IRQn
#define Termostate_status_Pin GPIO_PIN_4
#define Termostate_status_GPIO_Port GPIOB
#define Prgm__sensor_btn_Pin GPIO_PIN_5
#define Prgm__sensor_btn_GPIO_Port GPIOB
#define Prgm__sensor_btn_EXTI_IRQn EXTI9_5_IRQn
#define SPI_NOE_Pin GPIO_PIN_7
#define SPI_NOE_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
