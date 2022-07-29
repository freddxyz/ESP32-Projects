#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

extern char *TAG;

void wifi_sta_init(void);
void wifi_softap_init(void);
esp_err_t mount_storage(void);
httpd_handle_t start_http_server(void);
void execute_lil_file(FILE *fstream);
void execute_lil_code(char *code, size_t codelen);
