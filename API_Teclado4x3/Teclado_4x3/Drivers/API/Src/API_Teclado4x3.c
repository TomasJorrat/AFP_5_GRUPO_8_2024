/*
 * API_Teclado4x3.c
 *
 *  Created on: Feb 1, 2025
 *      Author: EMILIO
 */
#include "API_Teclado4x3.h"
#include "API_Debounce.h"  // Inclusión del driver de debounce
#include "API_Delay.h"      // Inclusión del driver de delay
#include "API_GPIO.h"       // Inclusión del driver de GPIO
#include <string.h>

#define BUFFER_SIZE 16 // Tamaño del buffer circular

// Definicion de pines para columnas
#define C1_PIN GPIO_PIN_2
#define C2_PIN GPIO_PIN_4
#define C3_PIN GPIO_PIN_5
#define C_PORT GPIOE

// Definicion de pines para filas
#define R1_PIN GPIO_PIN_6
#define R2_PIN GPIO_PIN_3
#define R3_PIN GPIO_PIN_8
#define R4_PIN GPIO_PIN_7
#define R_PORT GPIOF

// Mapeo de teclas del teclado matricial
static char keymap[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// Arreglos para manejar los pines de filas y columnas de forma conveniente
static uint16_t row_pins[ROWS] = {R1_PIN, R2_PIN, R3_PIN, R4_PIN};
static uint16_t col_pins[COLS] = {C1_PIN, C2_PIN, C3_PIN};

// Buffer circular para almacenar teclas presionadas
static char key_buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

// Delay para debounce
static delay_t debounce_delay;

// Inicialización del teclado matricial
void keypad_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitStruct.Pin = C1_PIN | C2_PIN | C3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;	// Modo interrupcion flanco descendente
    GPIO_InitStruct.Pull = GPIO_PULLUP;				// Pull-up interno habilitado
    HAL_GPIO_Init(C_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = R1_PIN | R2_PIN | R3_PIN | R4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(R_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(R_PORT, R1_PIN | R2_PIN | R3_PIN | R4_PIN, GPIO_PIN_SET);

    /* Habilitar interrupciones para EXTI2, EXTI4 y EXTI9_5 */
    HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);       // Prioridad más alta
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);               // Habilitar interrupción para PE2

    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);       // Para PE4
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);     // Para PE5
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);


    debounceFSM_init(); // Inicialización de la FSM de debounce
    delayInit(&debounce_delay, 40); // Inicialización del delay de debounce
}

// Buffer circular: agregar tecla
static void buffer_add(char key) {
    key_buffer[buffer_head] = key;
    buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    if (buffer_head == buffer_tail) {
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE; // Sobrescribe el más antiguo
    }
}

// Buffer circular: obtener tecla
char buffer_get(void) {
    if (buffer_head == buffer_tail) return 0;
    char key = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return key;
}

// Verifica si hay alguna tecla presionada
bool keypad_key_pressed(void) {
    for (int row = 0; row < ROWS; row++) {
        HAL_GPIO_WritePin(R_PORT, row_pins[row], GPIO_PIN_RESET);
        for (int col = 0; col < COLS; col++) {
            if (HAL_GPIO_ReadPin(C_PORT, col_pins[col]) == GPIO_PIN_RESET) {
                HAL_GPIO_WritePin(R_PORT, row_pins[row], GPIO_PIN_SET);
                return true;
            }
        }
        HAL_GPIO_WritePin(R_PORT, row_pins[row], GPIO_PIN_SET);
    }
    return false;
}

// Escanea el teclado y agrega teclas al buffer usando la FSM de debounce
static void scan_keypad(void) {
    for (int row = 0; row < ROWS; row++) {
        HAL_GPIO_WritePin(R_PORT, row_pins[row], GPIO_PIN_RESET);
        for (int col = 0; col < COLS; col++) {
            debounceFSM_update(HAL_GPIO_ReadPin(C_PORT, col_pins[col]));
            if (readKey()) { // Se detectó una pulsación estable
                buffer_add(keymap[row][col]);
            }
        }
        HAL_GPIO_WritePin(R_PORT, row_pins[row], GPIO_PIN_SET);
    }
}

// ISR para manejar interrupciones del teclado
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (delayRead(&debounce_delay)) { // Verifica el debounce antes de escanear
        scan_keypad();
    }
}

// Obtener tecla del buffer
char keypad_get_key(void) {
    return buffer_get();
}
