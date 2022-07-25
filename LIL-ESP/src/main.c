#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "lil.h"
#include "lil-esp.h"

char *TAG = "ESP-LIL TEST";

void execute_lil_code(char *code, size_t codelen) 
{
    lil_t lil = lil_new();
    lil_value_t val = lil_parse(lil, code, codelen, 0);
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
    ESP_LOGI(TAG, "DEBUGGG");
    free(buffer);
}

void app_main() 
{
    nvs_flash_init();
    mount_storage();
    wifi_sta_init();
    start_http_server();

    FILE *lil_main_filestream = fopen("/storage/main.lil", "r");
    execute_lil_file(lil_main_filestream);
    fclose(lil_main_filestream);
}