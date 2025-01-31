/*
 * API_BT.h
 *
 *  Created on: Jan 31, 2025
 *      Author: tomas
 *
 *  Libreria para el manejo de modulo bluetooth HC-05 utilizando USART2, configurado a los pines PD6 (RX) y PD5 (TX)
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef API_INC_API_BT_H_
#define API_INC_API_BT_H_

typedef uint8_t msj_t;

void MX_USART2_UART_Init(void);

void BT_TX(msj_t Mx_TX); 									//Transmitir un mensaje
void BT_RX(msj_t Mx_RX);									//Recibir un mensaje
void BT_TX_IT(msj_t Mx_TX);									//Transmitir utilizando interrupciones
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);


#endif /* API_INC_API_BT_H_ */
