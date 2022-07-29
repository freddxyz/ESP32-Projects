#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"

#include "lil.h"
#include "lil_esp_general.h"
#include "idf_lil_wrapper.h"

#define CALLBACKS 8

struct _lil_t
{
    const char* code; /* need save on parse */
    const char* rootcode;
    size_t clen; /* need save on parse */
    size_t head; /* need save on parse */
    int ignoreeol;
    lil_func_t* cmd;
    size_t cmds;
    size_t syscmds;
    char* catcher;
    int in_catcher;
    char* dollarprefix;
    lil_env_t env;
    lil_env_t rootenv;
    lil_env_t downenv;
    lil_value_t empty;
    int error;
    size_t err_head;
    char* err_msg;
    lil_callback_proc_t callback[CALLBACKS];
    size_t parse_depth;
    void* data;
};

lil_value_t fnc_gpio(lil_t lil, size_t argc, lil_value_t* argv)
{
    //gpio pin mode pullup/pulldown
    int pin;
    if (argc > 1) {
        pin = lil_to_integer(argv[0]);
        ESP_LOGI(TAG, "pin: %d", pin);
        gpio_pad_select_gpio(pin);
        ESP_LOGI(TAG, "selected pin!");
        const char *dir = lil_to_string(argv[1]);
        ESP_LOGI(TAG, "Direction: %s", dir);
        if (!strcmp(dir, "out")) {
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
            ESP_LOGI(TAG, "set gpio pin out");
        } else if (!strcmp(dir, "in")) {
            gpio_set_direction(pin, GPIO_MODE_INPUT);
            ESP_LOGI(TAG, "set gpio pin in");
        } else {
            const char *msg = "invalid pin direction. Please use \"in\" or \"out\".";
            lil_set_error_at(lil, lil->head, msg);
            return NULL;
        }
        if (argc > 2) {
            const char *pullmode = lil_to_string(argv[2]);
            if (strcmp(pullmode, "pullup")) {
                gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
                ESP_LOGI(TAG, "set pullup on");
            } else if (strcmp(pullmode, "pulldown")) {
                gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
                ESP_LOGI(TAG, "set pulldown on");
            } else {
                const char *msg = "invalid pull mode. Please use \"pullup\" or \"pulldown\".";
                lil_set_error_at(lil, lil->head, msg);
                return NULL;
            }
        }
    } else {
        const char *msg = "invalid params for gpio. Use \"gpio [pin] [in/out] [pullup/pulldown](optional)\"";
        lil_set_error_at(lil, lil->head, msg);
        return NULL;
    }
    
    return lil_alloc_integer(pin);
}

void register_wrapper_cmds(lil_t lil) 
{
    lil_register(lil, "gpio", fnc_gpio);
    ESP_LOGI(TAG, "registered custom commands");
}
