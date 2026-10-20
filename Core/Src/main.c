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
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define WS2812B_0val 68 // 170 000 000 * 0.4 * 10^-6 (T0H -> T0L)
#define WS2812B_1val 136 // to samo co wyzej tylko 0.8 * 10^-6 (T1H -> T1L)
#define WS2812B_RESPULSES 40 // 8500 ale musimy to upchac to 1.25 czyli 212 cykli wiec dzielimy i
							// w zaokragleniu 40 cykli musi przemilczec

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint32_t led_info[24 + WS2812B_RESPULSES] = {0}; // inicjalizacja tablicy ktora pomiesci informacje dla
											 // jednej diody i daje zapas na poslanie resetu

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void RGB_to_GRB(uint8_t R, uint8_t G, uint8_t B) {
	uint32_t GRB = (G << 16) | (R << 8) | B; // przesuwam odpowiednio kolory na bitach aby z RGB zrobić GRB (ledy tak odczytuja dane)

	for (int i = 23; i >= 0; i--){ // odczytuje GRB od najstarszego bitu, "tłumacze" na język ledów i wkładam do pakietu
		if (GRB & (1 << i)) { // slicing ale przez maske
			led_info[23-i] = WS2812B_1val;
		}
		else {
			led_info[23-i] = WS2812B_0val;
		}
	}

	for (int i = 24; i < (24+WS2812B_RESPULSES); i++) { // info o zakończeniu przesyłu
		led_info[i] = 0;
	}
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  uint8_t blink_steps = 0;
  uint32_t next_toggle_time = 0;
  uint8_t last_button_state = GPIO_PIN_SET;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  uint32_t curr_time = HAL_GetTick();
	  uint32_t cycle_t3s_time = curr_time % 3000; // T = 3s

	  uint8_t red = 0;
	  uint8_t green = 0;
	  uint8_t blue = 0;

	  // 2.1.1: obsluga wyswietlania czerwonego, po 50% czasu lampka gaśnie (sygnal prostokatny)

	  if (cycle_t3s_time < 1500) {
		  red = 255;
	  }
	  else {
		  red = 0;
	  }

	  // 2.1.2: obsługa plynnego rozjasniania zielonego, rozjasnia sie proporcjonalnie do przebytego czasu cyklu

	  green = 255*(cycle_t3s_time/3000);

	  // 2.1.3: obsluga niebieskiego jest wykonywana poprzez guzik wiec zostawiam na 0

	  // ...

	  // Wykorzystuje swoja funkcje do przeslania pakietu

	  RGB_to_GRB(red,green,blue);

	  // Wysyłam pakiet do diod LED

	  HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)led_info, (24 + WS2812B_RESPULSES));

	  // 2.1.3:

	  uint8_t current_button_state = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

	  // weryfikuje, że guzik jest wciskany i nie przerywam trwajacego juz cyklu
	  if (current_button_state == GPIO_PIN_RESET && last_button_state == GPIO_PIN_SET && blink_steps == 0) {
		  blink_steps = 6; // 3 wlaczenia i 3 wylaczenia
		  next_toggle_time = curr_time + 200; // 200 ms realizuje cale mrugania w 1.2s wiec w granicy t=3s
		  blue = 255;
	  }
	  last_button_state = current_button_state;


	  if (blink_steps > 0) {
		  if (curr_time >= next_toggle_time) { // weyrifkacja czy czas kroku nie minal
			  if (blue == 255) { // jezeli tak i zalaczone to wylacz diodke lub ja zapal
				  blue = 0;
			  }
			  else {
				  blue = 255;
			  }
			  blink_steps--; // jeden krok mniej
			  next_toggle_time = curr_time + 200; // dodanie czasu dla kolejnego kroku
		  }
	  }
	  else {
		  blue = 0; // cykl sie zakonczyl wiec wylaczam diode
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
