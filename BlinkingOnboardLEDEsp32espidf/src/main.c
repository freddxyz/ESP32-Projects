#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define BUILT_IN_LED 13

void blink_light(void *pvParameters)
{
    gpio_pad_select_gpio(BUILT_IN_LED);
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);

    uint8_t param = *(uint8_t*) pvParameters;

    while (1) {
        gpio_set_level(BUILT_IN_LED, 1);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(BUILT_IN_LED, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("[%u]", param);
    }
}

void app_main(void) 
{
    int i = 0;

    uint8_t parameterPastTest = 5;

    xTaskCreate(&blink_light, "LEDBLINK", configMINIMAL_STACK_SIZE, &parameterPastTest, 5 , NULL);

    while (1) {
        printf("[%d] Hello World! \n", 1);
        i++;
        vTaskDelay(500);
    }

}