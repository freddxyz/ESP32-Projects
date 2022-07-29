#include "lil/lil.incc"

#include "driver/gpio.h"
#include "esp_log.h"

#include "lil_esp_general.h"
#include "idf_lil_wrapper.h"

typedef struct {
    int pin;
} gpio_func_extra_t;

lil_value_t gpio_handler(lil_t lil, size_t argc, lil_value_t* argv) 
{
    //--gpio-pin--
    lil_env_t env = lil->env;
    lil_func_t func = env->func;
    gpio_func_extra_t *extra = (gpio_func_extra_t*) func->extra;
    int pin = extra->pin;
    const char *lvl = lil_to_string(argv[0]);
    if (isdigit(lvl[0])) {
        int lvl_int = lil_to_integer(argv[0]);
        gpio_set_level(pin, lvl_int);
    } else {
        if (!strcasecmp(lvl, "on")) {
            gpio_set_level(pin, 1);
        } else if (!strcasecmp(lvl, "off")) {
            gpio_set_level(pin, 0);
        }
    }
    return NULL;
}

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
        if (!strcasecmp(dir, "out")) {
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
            ESP_LOGI(TAG, "set gpio pin out");
        } else if (!strcasecmp(dir, "in")) {
            gpio_set_direction(pin, GPIO_MODE_INPUT);
            ESP_LOGI(TAG, "set gpio pin in");
        } else {
            const char *msg = "invalid pin direction. Please use \"in\" or \"out\".";
            lil_set_error_at(lil, lil->head, msg);
            return NULL;
        }
        if (argc > 2) {
            const char *pullmode = lil_to_string(argv[2]);
            if (!strcasecmp(pullmode, "pullup")) {
                gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
                ESP_LOGI(TAG, "set pullup on");
            } else if (!strcasecmp(pullmode, "pulldown")) {
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
    
    //create new func and return it
    lil_func_t cmd = add_func(lil, lil_to_string(lil_unused_name(lil, "gpio")));
    cmd->proc = gpio_handler;
    return lil_alloc_string(cmd->name);
}

void register_wrapper_cmds(lil_t lil) 
{
    lil_register(lil, "gpio", fnc_gpio);
    ESP_LOGI(TAG, "registered custom commands");
}
