#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* -------------------- Handle -------------------- */
TaskHandle_t ledTaskHandle;       // LED PA0 (b?t/t?t b?ng n�t)
TaskHandle_t ctrlTaskHandle;      // �i?u khi?n n�t
TaskHandle_t blinkTaskHandle;     // LED PC13 lu�n nh�y
QueueHandle_t ledStateQueueHandle;
QueueHandle_t buttonQueueHandle;

/* -------------------- Prototype -------------------- */
void led_config(void);
void button_irq_config(void);
void led_task(void *pvParameters);
void ctrl_task(void *pvParameters);
void blink_task(void *pvParameters);

/* ====================================================== */
/* ====================== MAIN ========================== */
/* ====================================================== */
int main(void)
{
    led_config();
    button_irq_config();

    /* T?o Queue cho LED state v� n�t nh?n */
    ledStateQueueHandle = xQueueCreate(1, sizeof(uint8_t));
    buttonQueueHandle   = xQueueCreate(1, sizeof(uint8_t));

    if (ledStateQueueHandle == NULL || buttonQueueHandle == NULL)
        while (1);

    /* T?o task LED (PA0, b?t/t?t b?ng n�t) */
    xTaskCreate(led_task, "LED_TASK", 128, NULL, 1, &ledTaskHandle);

    /* T?o task di?u khi?n (x? l� n�t nh?n) */
    xTaskCreate(ctrl_task, "CTRL_TASK", 128, NULL, 2, &ctrlTaskHandle);

    /* T?o task LED PC13 nh�y d?c l?p */
    xTaskCreate(blink_task, "BLINK_TASK", 128, NULL, 1, &blinkTaskHandle);

    /* Kh?i d?ng scheduler */
    vTaskStartScheduler();

    while (1);
}

/* ====================================================== */
/* =============== C?U H�NH PH?N C?NG =================== */
/* ====================================================== */

/* --- LED PA0 & PC13 output push-pull --- */
void led_config(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    /* PA0: LED ch�nh (di?u khi?n b?ng n�t) */
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* PC13: LED nh�y d?c l?p */
    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);
}

/* --- N�t PA1 d�ng ng?t EXTI1 --- */
void button_irq_config(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    /* PA1 input pull-up */
    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);

    /* G�n PA1 -> EXTI1 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    /* C?u h�nh EXTI1 k�ch c?nh xu?ng (?n n�t) */
    exti.EXTI_Line = EXTI_Line1;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    /* C?u h�nh NVIC cho EXTI1 */
    nvic.NVIC_IRQChannel = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 5;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

/* ====================================================== */
/* ===================== ISR HANDLER ==================== */
/* ====================================================== */

/* ISR c?a PA1 (EXTI1) */
void EXTI1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t flag = 1;

    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        /* G?i t�n hi?u sang task di?u khi?n */
        xQueueSendFromISR(buttonQueueHandle, &flag, &xHigherPriorityTaskWoken);

        /* X�a c? ng?t */
        EXTI_ClearITPendingBit(EXTI_Line1);
    }

    /* Cho ph�p chuy?n ng? c?nh n?u task cao hon s?n s�ng */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ====================================================== */
/* ====================== TASKS ========================= */
/* ====================================================== */

/* Task LED PA0: b?t/t?t theo tr?ng th�i du?c g?i t? CTRL */
void led_task(void *pvParameters)
{
		(void) pvParameters;
    const uint32_t blink_delay = 200;  // ms
    uint8_t led_state = 0;
    uint8_t received_state;

    while (1)
    {
        /* Nh?n tr?ng th�i m?i (n?u c�) */
        if (xQueueReceive(ledStateQueueHandle, &received_state, 0) == pdPASS)
        {
            led_state = received_state;
        }

        if (led_state)
        {
            /* LED nh�y */
            GPIOA->ODR ^= GPIO_Pin_0;
        }
        else
        {
            /* LED t?t ho�n to�n */
            GPIO_ResetBits(GPIOA, GPIO_Pin_0);
        }

        vTaskDelay(pdMS_TO_TICKS(blink_delay));
    }
}

/* Task di?u khi?n: b?t/t?t LED khi c� t�n hi?u t? n�t nh?n */
void ctrl_task(void *pvParameters)
{
		(void) pvParameters;
    uint8_t led_on = 0;
    uint8_t button_msg;

    /* G?i tr?ng th�i ban d?u (LED OFF) */
    xQueueOverwrite(ledStateQueueHandle, &led_on);

    while (1)
    {
        /* Ch? t�n hi?u t? ISR (n�t nh?n) */
        if (xQueueReceive(buttonQueueHandle, &button_msg, portMAX_DELAY) == pdPASS)
        {
            /* �?o tr?ng th�i LED ON/OFF */
            led_on = !led_on;
            xQueueOverwrite(ledStateQueueHandle, &led_on);

            /* ch?ng d?i n�t */
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

/* Task LED PC13: lu�n nh�y d?c l?p */
void blink_task(void *pvParameters)
{
		(void) pvParameters;
    const uint32_t blink_delay = 300; // ms

    while (1)
    {
        GPIOC->ODR ^= GPIO_Pin_13;
        vTaskDelay(pdMS_TO_TICKS(blink_delay));
    }
}
