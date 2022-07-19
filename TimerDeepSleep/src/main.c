#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"

char *TAG = "SLEEPER";

void app_main() 
{
    vTaskDelay(5 * portTICK_PERIOD_MS);

    //Find if device has woken up from sleep
    switch(esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "woken up from timer");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Not a wakeup.");
    }

    ESP_LOGI(TAG, "I'm going to sleep.");
    esp_sleep_enable_timer_wakeup(20 * 1000000);
    esp_deep_sleep_start();
}