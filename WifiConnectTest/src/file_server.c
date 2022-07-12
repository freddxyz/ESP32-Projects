// #include <sys/param.h>
// #include <stdio.h>
// #include "esp_err.h"
// #include "esp_http_server.h"
// #include "esp_log.h"
// #include "esp_ota_ops.h"
 #include "ota_updater.h"

#define MAX_PATH_LENGTH (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define REQUEST_CHUNK_SIZE 1024
#define BASE_PATH "/storage"

//Creates a path by joining the partition name with the uri and stores it in "buf"
void get_uri_path(const char *uri, char *buf) 
{
    snprintf(buf, MAX_PATH_LENGTH, "%s%s", BASE_PATH, uri);
}

int file_has_extention(const char *filename, const char *extention) 
{
    int test = strcasecmp(&filename[strlen(filename) - strlen(extention)],extention);
    ESP_LOGI(TAG, "Actual ending: %s", &filename[strlen(filename) - sizeof(extention)]);
    ESP_LOGI(TAG, "extention: %d, filenamelgth:%d", sizeof(extention), strlen(filename));
    ESP_LOGI(TAG, "Extention argument: %s", extention);
    return (test == 0);
}

esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (file_has_extention(filename, ".html")) {
        ESP_LOGI(TAG, "html");
        return httpd_resp_set_type(req, "text/html");
    } else if (file_has_extention(filename, ".js")) {
        ESP_LOGI(TAG, "js");
        return httpd_resp_set_type(req, "application/javascript");
    };
    ESP_LOGI(TAG, "plain");
    ESP_LOGI(TAG, "Extention:%s, compared to:%s", &filename[strlen(filename) - sizeof(".html") + 1], ".html");
    return httpd_resp_set_type(req, "text/plain");
}

esp_err_t http_get_file_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    char filepath[MAX_PATH_LENGTH];
    get_uri_path(uri, filepath);
    
    FILE *filestream;
    filestream = fopen(filepath, "r");

    ESP_LOGI(TAG, "Filepath generated:%s", filepath);
    ESP_LOGI(TAG, "URI:%s", uri);

    char chunk[REQUEST_CHUNK_SIZE];
    size_t chunksize;
    const char* resp = "Unable to open file!";

    set_content_type_from_file(req, (const char*) filepath);

    if (!filestream) {
        httpd_resp_send(req, resp, strlen(resp));
        ESP_LOGE(TAG, "Unable open file!");
        return ESP_FAIL;
    }  

    do {
        chunksize = fread(chunk, 1, REQUEST_CHUNK_SIZE, filestream);
        if (chunksize > 0 ) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
    } while (chunksize != 0);

    httpd_resp_send_chunk(req, NULL, 0);

    fclose(filestream);

    return ESP_OK;
}

esp_err_t http_post_ota_update_handler(httpd_req_t *req) 
{
    const esp_partition_t *update_partition;
    update_partition = esp_ota_get_next_update_partition(NULL);

    esp_ota_handle_t update_handle;
    esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

    char read_buffer[REQUEST_CHUNK_SIZE];
    
    int data_remaining = req->content_len;
    
    while (data_remaining > 0) {
        int data_read = httpd_req_recv(req, read_buffer, MIN(data_remaining, REQUEST_CHUNK_SIZE));
        esp_ota_write(update_handle, (const void*)read_buffer, data_read);

        data_remaining -= data_read;
    }

    esp_ota_end(update_handle);
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
}

httpd_uri_t uri_get = {
    .uri = "/*",
    .handler = http_get_file_handler,
    .method = HTTP_GET,
    .user_ctx = NULL
};

httpd_uri_t uri_ota_post = {
    .uri = "/ota_upload",
    .handler = http_post_ota_update_handler,
    .method = HTTP_POST,
    .user_ctx = NULL
};

httpd_handle_t start_http_server(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_ota_post);
    }

    return server;
}