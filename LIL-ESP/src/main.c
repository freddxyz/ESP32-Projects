#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "idf_lil_wrapper.h"
#include "lil_esp_general.h"

char *TAG = "ESP-LIL TEST";

void handle_exit(lil_t lil, lil_value_t arg) 
{
    if (arg) {
        ESP_LOGI(TAG, "Exited lil script. Args: %s", lil_to_string(arg));
    }
    ESP_LOGI(TAG, "Exited lil script");
}

void handle_error(lil_t lil, size_t pos, const char* msg) 
{
    ESP_LOGI(TAG, "Error in LIL code: %s", msg);
}

void execute_lil_code(char *code, size_t codelen) 
{
    lil_t lil = lil_new();

    register_wrapper_cmds(lil);

    lil_callback(
        lil,
        LIL_CALLBACK_EXIT, 
        (lil_callback_proc_t)handle_exit
    );
    
    lil_callback(
        lil,
        LIL_CALLBACK_ERROR, 
        (lil_callback_proc_t)handle_error
    );

    lil_value_t val = lil_parse(lil, code, codelen, 1);
    ESP_LOGI(TAG, "LIL Code Output: %s", lil_to_string(val));
    lil_free_value(val);
} 

int get_file_size(FILE *fstream) 
{
    int rtn = 0;
    fseek(fstream, 0, SEEK_END);
    rtn = ftell(fstream);
    fseek(fstream, 0, SEEK_SET);
    return rtn;
}

void execute_lil_file(FILE *fstream) 
{
    //get size of file
    int fsize = get_file_size(fstream);

    char *buffer = (char*) malloc(fsize+1);

    ESP_LOGI(TAG, "Read %d characters from main.lil.", fread(buffer, 1, fsize, fstream));
    buffer[fsize] = '\0';
    ESP_LOGI(TAG, "Code: \n %s", buffer);
    execute_lil_code(buffer, fsize);
    free(buffer);
}

void app_main() 
{
    nvs_flash_init();
    mount_storage();
    wifi_softap_init();
    start_http_server();

    FILE *lil_main_filestream = fopen("/storage/main.lil", "r");
    execute_lil_file(lil_main_filestream);
    fclose(lil_main_filestream);
}