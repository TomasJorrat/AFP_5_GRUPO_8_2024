/*
 * API_GPIO.c
 *
 *  Created on: Aug 29, 2024
 *      Author: tomas jorrat
 */

/* Includes */
#include "main.h"
#include "API_GPIO.h"


/*Declaracion de variables*/
led_t LDx; 					/*Valores esperadors de LDx: LD1_Pin|LD3_Pin|LD2_Pin*/
/* Function Definition *******************************/

/*
 * @brief Encender LED GPIO
 * @param led_t LDx
 * @retval ninguno

 */
void writeLedOn_GPIO(led_t LDx){
	HAL_GPIO_WritePin(GPIOB, LDx, GPIO_PIN_SET);
}

/*
 * @brief Apagar LED GPIO
 * @param led_t LDx
 * @retval ninguno
 */

void writeLedOff_GPIO(led_t LDx){
	HAL_GPIO_WritePin(GPIOB, LDx, GPIO_PIN_RESET);
}

/*
 * @brief Alternar LED GPIO
 * @param led_t LDx
 * @retval ninguno
 */

void toggleLed_GPIO(led_t LDx){
	HAL_GPIO_TogglePin(GPIOB, LDx);
}

buttonStatus_t readButton_GPIO(void){
	return HAL_GPIO_ReadPin(GPIOC, USER_Btn_Pin);
}
