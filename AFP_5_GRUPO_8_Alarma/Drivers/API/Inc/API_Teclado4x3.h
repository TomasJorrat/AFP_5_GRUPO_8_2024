/*
 * API_Teclado4x3.h
 *
 *  Created on: Feb 1, 2025
 *      Author: EMILIO
 */

#ifndef API_INC_API_TECLADO4X3_H_
#define API_INC_API_TECLADO4X3_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define ROWS 4
#define COLS 3

// Prototipos de funciones
void keypad_init(void);            // Inicializa el teclado matricial
char keypad_get_key(void);         // Obtiene la última tecla presionada
bool keypad_key_pressed(void);     // Verifica si hay una tecla presionada

#endif /* API_INC_API_TECLADO4X3_H_ */
