/*
 * API_BT.c
 *
 *  Created on: Jan 31, 2025
 *      Author: tomas
 */
/*Includes*/
#include "main.h"
#include "API_BT.h"

/*Declaracion de variables*/
//msj_t Mx_TX[20]="Alarma Activada \n\r", Mx_RX[20];
msj_t Mx_TX[100];
msj_t Mx_RX[100];

UART_HandleTypeDef huart2;


/* Function Definition *******************************/

/*
 * @brief Inicializacion USART2 (Asincrona)
 * @param void
 * @retval void
*/
void MX_USART2_UART_Init(void)
{
  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/*
 * @brief Enviar un mensaje por medio de USART2
 * @param msj_t Mx_TX (cadena de caracteres)
 * @retval void

*/
void BT_TX(msj_t Mx_TX)
{
	HAL_UART_Transmit(&huart2, Mx_TX, sizeof(Mx_TX), HAL_MAX_DELAY);
}


/*
 * @brief Enviar un mensaje por medio de USART2 utilizando interrupciones del NVIC
 * @param msj_t Mx_TX (cadena de caracteres)
 * @retval void
*/
void BT_TX_IT(msj_t Mx_TX)
{
	HAL_UART_Transmit_IT(&huart2, Mx_TX, sizeof(Mx_TX)-1);
}

/*
 * @brief Cuando se completa la transmision se puede realizar una tarea determinada
 * @param modulo UART
 * @retval void
*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2){
		/*USER CODE BEGIN TxCpltCallback*/

		/*USER CODE END TxCpltCallback*/
	}
}

/*
 * @brief Recibir un mensaje del modulo BT con USART2
 * @param msj_t Mx_RX (cadena de caracteres)
 * @retval void
*/
void BT_RX(msj_t Mx_RX)
{
	HAL_UART_Receive(&huart2, Mx_RX, sizeof(Mx_RX), HAL_MAX_DELAY);
}




