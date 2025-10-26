#include "stm32f10x.h"                  
#include "FreeRTOS.h"                  
#include "task.h"                     

void USART_Config(void);
void UART_SendChar(char ch);
void UART_SendString(char *str);
void Task1(void *pvParameters);
void Task2(void *pvParameters);


int main(void) {
    SystemInit();
    USART_Config();

    xTaskCreate(Task1, "T1", 256, NULL, 1, NULL);
    xTaskCreate(Task2, "T2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1);
}

void USART_Config(void) {
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // PA9 = TX
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // PA10 = RX
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = 9600;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    USART_Cmd(USART1, ENABLE);
}


void UART_SendChar(char ch) {
    USART_SendData(USART1, ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendString(char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}


void Task1(void *pvParameters) {
		(void) pvParameters;
    while (1) {
      UART_SendString("AAAAAAAAAAAAAAAAAAAAAAA\r\n");
			vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Task2(void *pvParameters) {
		(void) pvParameters;
    while (1) {
      UART_SendString("BBBBBBBBBBBBBBBBBBBBBBB\r\n");
			vTaskDelay(pdMS_TO_TICKS(100));
    }
}
