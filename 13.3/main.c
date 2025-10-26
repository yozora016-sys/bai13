#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xMutex;

void UART_Config(void);
void UART_SendString(char *str);
void Task1(void *pvParameters);
void Task2(void *pvParameters);

int main(void) {
    UART_Config();
    xMutex = xSemaphoreCreateMutex();
    xTaskCreate(Task1, "T1", 128, NULL, 1, NULL);
    xTaskCreate(Task2, "T2", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    while(1);
}

void UART_Config(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef usart;
    usart.USART_BaudRate = 9600;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendString(char *str) {
    while(*str) {
        while(!(USART1->SR & USART_SR_TXE));
        USART_SendData(USART1, *str++);
    }
}

void Task1(void *pvParameters) {
		(void) pvParameters;
    while(1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
            UART_SendString("A\r\n");
            vTaskDelay(pdMS_TO_TICKS(50));
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void Task2(void *pvParameters) {
		(void) pvParameters;
    while(1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
            UART_SendString("B\r\n");
            vTaskDelay(pdMS_TO_TICKS(50));
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
