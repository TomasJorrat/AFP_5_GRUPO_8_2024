/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "API_BT.h"				//Libreria para el manejo de modulo HC-05 USART2
#include "API_GPIO.h"			//Libreria para el manjeo de puertos GPIO (Sensores, Sirena y Leds)
#include "API_Debounce.h"		//Libreria para evitar el rebote mecanico en la pulsacion de botones
#include "API_Delay.h"			//Libreria para delay no bloqueante
#include "API_Teclado4x3.h"		//Libreria para manejo de teclado matricial 4x3
#include "API_LCD.h"			//Libreria para el manejo de pantalla LCD 2x16 I2C
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

/* USER CODE BEGIN PV */
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart2;

char currentPassword[5] = "1234";  // Contraseña inicial por defecto
char newPassword[5];                // Para almacenar la nueva contraseña
typedef enum {
	MAIN_MENU,
	ALARM_MENU,
	CHANGE_PASS_MENU,
	TEST_ALARM_MENU,
	ACTIVE_ALARM
} MenuState;
MenuState currentState = MAIN_MENU;
bool includeMotionSensor = false;
char inputBuffer[5];
uint8_t inputIndex = 0;
bool alarmActivated = false;   // Estado de la alarma
bool countdownStarted = false; // Temporizador interno
uint32_t startTime = 0;        // Momento en que inicia el temporizador interno
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/


/* USER CODE BEGIN PFP */
void DisplayMainMenu();
void HandleMainMenuInput(char key);
void DisplayAlarmMenu();
void HandleAlarmMenuInput(char key);
void RequestPassword(void (*onSuccess)(void), void (*onFailure)(void));
void ActivateAlarm();
void DeactivateAlarm();
void DisplayChangePassMenu();
void ConfirmNewPassword();
void HandleSubMenu();
void TestAlarm();
void AlarmTriggered();
void IncorrectPassword();
void HandleActiveAlarm(char key);
void CheckSensors();
void CheckAlarmDeactivation(char key);

char BT_ReceiveMessage();
void BT_SendMessage(char *message);
void BT_Test();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  keypad_init();
  BT_Test(); // Enviar mensaje de prueba al HC-05
  HAL_Delay(30);//
  lcd_init();
  DisplayMainMenu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  char key = keypad_get_key();

	  if (key != '\0'){
		  switch (currentState){
			  case MAIN_MENU:
				  HandleMainMenuInput(key);
				  break;
			  case ALARM_MENU:
				  HandleAlarmMenuInput(key);
				  break;
			  case CHANGE_PASS_MENU:
			  case TEST_ALARM_MENU:
			  case ACTIVE_ALARM:
				  CheckSensors(); 				// Revisa los sensores mientras la alarma está activa
				  break;
		  }
	  }
	  /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}


/* USER CODE BEGIN 4 */
// Función para mostrar el menú principal
void DisplayMainMenu() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("1.Activar *.Mas");
    lcd_set_cursor(1, 0);
    lcd_print("2.Cambiar Pass");
    currentState = MAIN_MENU;
}

// Manejo de la entrada del menú principal
void HandleMainMenuInput(char key) {
    switch (key) {
        case '1':
            DisplayAlarmMenu();
            break;
        case '2':
            DisplayChangePassMenu();
            break;
        case '*':
            HandleSubMenu();
            break;
    }
}

// Mostrar menú para activar alarma
void DisplayAlarmMenu() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("1.Sist Completo");
    lcd_set_cursor(1, 0);
    lcd_print("2.Sin Sensor Mov");
    currentState = ALARM_MENU;
}

// Manejo de la entrada del menú de alarma
void HandleAlarmMenuInput(char key) {
    switch (key) {
        case '1':  // Modo "Sist Completo"
            includeMotionSensor = true;
            RequestPassword(ActivateAlarm, DisplayAlarmMenu);
            break;
        case '2':  // Modo "Sin Sensor Mov"
            includeMotionSensor = false;
            RequestPassword(ActivateAlarm, DisplayAlarmMenu);
            break;
        case '*':  // Si el usuario presiona "*", volver al menú principal
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Operacion");
            lcd_set_cursor(1, 0);
            lcd_print("Cancelada");
            HAL_Delay(2000);
            DisplayMainMenu();  // Volver al menú principal
            break;
    }
}

// Solicitud de contraseña
void RequestPassword(void (*onSuccess)(void), void (*onFailure)(void)) {
    while (1) { // Bucle para reintentar si la contraseña es incorrecta
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Contrasena:____");
        memset(inputBuffer, 0, sizeof(inputBuffer));
        inputIndex = 0;

        uint32_t startTime = HAL_GetTick();  // Guardar el tiempo de inicio

        while (1) {
            if (HAL_GetTick() - startTime > 20000) { // Si pasan más de 20 segundos sin entrada
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("Tiempo agotado");
                lcd_set_cursor(1, 0);
                lcd_print("Volviendo...");
                HAL_Delay(2000);
                DisplayMainMenu();  // Volver al menú principal
                return;
            }

            char key = keypad_get_key();

            if (key != '\0') {
                startTime = HAL_GetTick();  // Reiniciar el temporizador

                if (key == '*') {  // Si presiona "*", vuelve al menú principal
                    lcd_clear();
                    lcd_set_cursor(0, 0);
                    lcd_print("Operacion");
                    lcd_set_cursor(1, 0);
                    lcd_print("Cancelada");
                    HAL_Delay(2000);
                    DisplayMainMenu();  // Volver al menú principal
                    return;
                }

                if (key >= '0' && key <= '9' && inputIndex < 4) {
                    inputBuffer[inputIndex++] = key;
                    lcd_set_cursor(1, 10 + inputIndex - 1);
                    lcd_print("*");
                } else if (key == '#') { //  Cuando se presiona "#", verifica la clave
                    inputBuffer[inputIndex] = '\0';
                    if (strcmp(inputBuffer, currentPassword) == 0) {
                        onSuccess(); // Si la clave es correcta, sale de la función
                        return;
                    } else {
                        //  Si la contraseña es incorrecta, mostrar mensaje y volver a pedirla
                        lcd_clear();
                        lcd_set_cursor(0, 0);
                        lcd_print("Contrasena");
                        lcd_set_cursor(1, 0);
                        lcd_print("Incorrecta");
                        HAL_Delay(5000);
                        break;  //  Sale de este while pero vuelve al inicio del while externo
                    }
                }
            }
        }
    }
}


// Activar la alarma
void ActivateAlarm() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Alarma Activada");
    lcd_set_cursor(1, 0);
    lcd_print("Corran: 20 seg");

    BT_SendMessage("⚠️ Alarma activada. Corran 20s... \r\n");

    uint32_t countdownStart = HAL_GetTick();
    uint32_t remainingTime = 20;

    while (remainingTime > 0) {
        if (HAL_GetTick() - countdownStart >= 1000) {
            countdownStart = HAL_GetTick();
            remainingTime--;

            lcd_set_cursor(1, 7);
            lcd_print("   ");
            lcd_set_cursor(1, 7);
            char buffer[3];
            sprintf(buffer, "%2lu", (unsigned long)remainingTime);
            lcd_print(buffer);
        }
    }

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Alarma");
    lcd_set_cursor(1, 0);
    lcd_print("Activa!");

    BT_SendMessage("🚨 Alarma activada! \r\n");

    alarmActivated = true;
    currentState = ACTIVE_ALARM;

    while (alarmActivated) {
        char key = keypad_get_key();
        if (key != '\0') {
            CheckAlarmDeactivation(key);  // Nueva función para manejar la desactivación con temporizador
        }
    }
}

// Manejar la alarma activa
void HandleActiveAlarm(char key) {
    if (key == '#') { // Botón para intentar desactivar
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Ingrese");
        lcd_set_cursor(1, 0);
        lcd_print("Contrasena:");
        RequestPassword(DeactivateAlarm, IncorrectPassword);
    }

    //  Verificar si recibe el código de desactivación por Bluetooth
       char btData;
       if (HAL_UART_Receive(&huart2, (uint8_t *)&btData, 1, 100) == HAL_OK) {
           if (btData == '#') {  // Si el usuario envía "#" desde la app, desactiva la alarma
               RequestPassword(DeactivateAlarm, IncorrectPassword);
           }
       }
}

// Desactivar la alarma si la contraseña es correcta
void DeactivateAlarm() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Alarma");
    lcd_set_cursor(1, 0);
    lcd_print("Desactivada");

    BT_SendMessage("✅ Alarma desactivada. Todo en orden. \r\n"); // Notificación de desactivación

    alarmActivated = false;

    // Asegurar que el buzzer se apaga
    HAL_GPIO_WritePin(Sirena_GPIO_Port, Sirena_Pin, GPIO_PIN_RESET);

    HAL_Delay(2000);
    DisplayMainMenu();
}


// Contraseña incorrecta: mensaje y reinicio de intento
void IncorrectPassword() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Contrasena");
    lcd_set_cursor(1, 0);
    lcd_print("Incorrecta");
    HAL_Delay(5000); // Mostrar mensaje durante 5 segundos

    // Iniciar temporizador interno si aún no está activo
    if (!countdownStarted) {
        countdownStarted = true;
        startTime = HAL_GetTick(); // Guardar tiempo actual
    }

    // Solicitar nuevamente la contraseña
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Ingrese");
    lcd_set_cursor(1, 0);
    lcd_print("Contrasena:");
}

// Activar alarma sonora
void AlarmTriggered() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("ALERTA!!!");
    lcd_set_cursor(1, 0);
    lcd_print("Ingrese clave");

    BT_SendMessage("⚠️ Alarma activada! \r\n"); // Enviar mensaje por Bluetooth

    uint32_t lastToggleTime = HAL_GetTick();  // Tiempo de referencia para el buzzer

    while (alarmActivated) {
        // Alternar el buzzer cada 500 ms sin bloquear el sistema
        if (HAL_GetTick() - lastToggleTime >= 500) {
            lastToggleTime = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_13);
        }

        // Permitir que el usuario intente apagar la alarma
        char key = keypad_get_key();
        if (key != '\0') {
            RequestPassword(DeactivateAlarm, IncorrectPassword);
        }

        char btData;
                if (HAL_UART_Receive(&huart2, (uint8_t *)&btData, 1, 100) == HAL_OK) {
                    if (btData == '#') {  // 🔹 Si recibe "#" por Bluetooth, intenta desactivar
                        RequestPassword(DeactivateAlarm, IncorrectPassword);
                    }
                }
    }

    // Apagar el buzzer cuando la alarma se desactiva
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_RESET);
}

// Cambiar contraseña
void DisplayChangePassMenu() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Contrasena");
    lcd_set_cursor(1, 0);
    lcd_print("Actual:");

    char key;
    while (1) {  // Bucle para capturar la tecla antes de solicitar la contraseña
        key = keypad_get_key();
        if (key == '*') {  //  Si el usuario presiona "*", volver al menú principal
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Operacion");
            lcd_set_cursor(1, 0);
            lcd_print("Cancelada");
            HAL_Delay(2000);
            DisplayMainMenu();  //  Volver al menú principal
            return;
        }
        if (key != '\0') {  // Si presiona otra tecla, salir del bucle y continuar con la contraseña
            break;
        }
    }

    RequestPassword(ConfirmNewPassword, DisplayChangePassMenu);
}

void ConfirmNewPassword() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Nueva Clave:");

    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputIndex = 0;
    uint32_t startTime = HAL_GetTick();  // Tiempo de inicio para evitar bucles infinitos

    while (HAL_GetTick() - startTime < 20000) {  // Tiempo límite de 20 segundos
        char key = keypad_get_key();

        if (key >= '0' && key <= '9' && inputIndex < 4) {
            inputBuffer[inputIndex++] = key;
            lcd_set_cursor(1, inputIndex - 1);
            lcd_print("*");
        } else if (key == '#') {
            if (inputIndex == 4) {  // Validar que la clave tenga 4 dígitos
                inputBuffer[inputIndex] = '\0';
                strcpy(newPassword, inputBuffer);

                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("Confirmar: ");
                lcd_set_cursor(1, 0);
                lcd_print("*.Si   #.No");

                uint32_t confirmStart = HAL_GetTick();
                while (HAL_GetTick() - confirmStart < 10000) {  // Espera 10 segundos para confirmar
                    char confirmKey = keypad_get_key();
                    if (confirmKey == '*') {
                        strcpy(currentPassword, newPassword);
                        lcd_clear();
                        lcd_set_cursor(0, 0);
                        lcd_print("Clave Actualizada");
                        HAL_Delay(2000);
                        DisplayMainMenu();
                        return;
                    } else if (confirmKey == '#') {
                        ConfirmNewPassword();  // Volver a solicitar la clave
                        return;
                    }
                }
            } else {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("Debe ser 4 ");
                lcd_set_cursor(1, 0);
                lcd_print("digitos ");
                HAL_Delay(2000);
                ConfirmNewPassword();  // Reiniciar el proceso
                return;
            }
        }
    }

    // Si el usuario no ingresa nada en 20 segundos, volver al menú principal
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Tiempo ");
    lcd_set_cursor(1, 0);
    lcd_print("Excedido ");
    HAL_Delay(2000);
    DisplayMainMenu();
}

// Submenú "Más"
void HandleSubMenu() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("#.Volver ");
    lcd_set_cursor(1, 0);
    lcd_print("3.Prueba");

    while (1) {
        char key = keypad_get_key();

        if (key != '\0') {
            if (key == '#') {
                DisplayMainMenu();
                return;
            } else if (key == '3') {
                TestAlarm();
                return;
            }
        }
    }
}

// Prueba de alarma
void TestAlarm() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Prueba de");
    lcd_set_cursor(1, 0);
    lcd_print("Alarma...");

    // Activar el buzzer en PA13
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(5000);  // Mantener el buzzer encendido 5 segundos
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_RESET);  // Apagar buzzer

    HAL_Delay(1000);  // Pequeña pausa antes de volver al menú
    DisplayMainMenu();
}


void CheckSensors() {
    if (alarmActivated) {
        // Verificar si se abre una puerta o ventana (sensor magnético)
        bool doorOpened = (HAL_GPIO_ReadPin(GPIOA, Sensor_Magnetico_1_Pin) == GPIO_PIN_RESET);
        bool motionDetected = false;

        // Solo verificar el sensor PIR si está activado en "Sist Completo"
        if (includeMotionSensor) {
            motionDetected = HAL_GPIO_ReadPin(GPIOA, Sensor_PIR_Pin);

            // Filtro por software para evitar falsas detecciones
            HAL_Delay(50);
            if (HAL_GPIO_ReadPin(GPIOA, Sensor_PIR_Pin) != motionDetected) {
                motionDetected = false; // Ignorar si el estado cambió muy rápido
            }
        }

        // Evaluar si se debe activar la alarma
        if (doorOpened || (includeMotionSensor && motionDetected)) {
            AlarmTriggered();
        }
    }
}

//Enviar datos al HC-05
void BT_SendMessage(char *message) {
    HAL_UART_Transmit(&huart2, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
}


//Recibir datos desde el HC-05
char BT_ReceiveMessage() {
    char receivedChar;
    HAL_UART_Receive(&huart2, (uint8_t *)&receivedChar, 1, HAL_MAX_DELAY);
    return receivedChar;
}

//Para probar si el STM32 está enviando datos correctamente al módulo Bluetooth HC-05
void BT_Test() {
    char message[] = "✅ HC-05 conectado con STM32\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
}


void CheckAlarmDeactivation(char key) {
    static bool countdownStarted = false;  // Variable local para evitar reiniciar el temporizador

    if (!countdownStarted) {
        countdownStarted = true;
        startTime = HAL_GetTick();  //  Iniciar temporizador solo cuando se intenta desactivar la alarma
    }

    RequestPassword(DeactivateAlarm, IncorrectPassword);

    // Si pasan 31 segundos sin ingresar la clave correcta, activar la alarma
    if (HAL_GetTick() - startTime >= 31000) {
        AlarmTriggered();
    }
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

#ifdef  USE_FULL_ASSERT
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
