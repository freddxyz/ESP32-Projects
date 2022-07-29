#include <sys/param.h>
#include <stdio.h>
#include "esp_ota_ops.h"
#include "esp_vfs.h"
#include "esp_log.h"

#include "lil_esp_general.h"

#define MAX_PATH_LENGTH (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define REQUEST_CHUNK_SIZE 1024
#define MAX_UPLOAD_REQUEST_SIZE 1024
#define BASE_PATH "/storage"

//Creates a path by joining the partition name with the uri and stores it in "buf"
void get_path_from_uri(const char *uri, char *buf) 
{
    snprintf(buf, MAX_PATH_LENGTH, "%s%s", BASE_PATH, uri);
}

int file_has_extention(const char *filename, const char *extention) 
{
    int test = strcasecmp(&filename[strlen(filename) - strlen(extention)],extention);
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
    return httpd_resp_set_type(req, "text/plain");
}

esp_err_t http_get_file_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    char filepath[MAX_PATH_LENGTH];
    get_path_from_uri(uri, filepath);
    
    FILE *filestream;
    filestream = fopen(filepath, "r");

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

esp_err_t file_upload_post_handler(httpd_req_t *req) 
{
    const char *uri = req->uri;

    char content[MAX_UPLOAD_REQUEST_SIZE];

    char filepath[MAX_PATH_LENGTH];
    get_path_from_uri(uri + sizeof("/upload") - 1, filepath);

    FILE *uploaded_file_stream = fopen(filepath, "w");

    int recv = 0;
    while (recv < req->content_len) {
        int chars_read = httpd_req_recv(req, content, MAX_UPLOAD_REQUEST_SIZE);
        ESP_LOGI(TAG, "Recieved %d byte chunk from file %s", chars_read, filepath + sizeof("/storage/")-1);
        ESP_LOGI(TAG, "Storing in flash...");
        int written = fwrite(content, 1, chars_read, uploaded_file_stream);
        ESP_LOGI(TAG, "Wrote %d bytes to flash.", written);

        if (chars_read != written) {
            fclose(uploaded_file_stream);
            ESP_LOGE(TAG, "Error writing to flash or recieving data!");
            return ESP_FAIL;
        }

        recv += chars_read;
    }

    ESP_LOGI(TAG, "Recieved all chunks.");    
    fclose(uploaded_file_stream);
    ESP_LOGI(TAG, "Stored in flash!");

    //todo: load the file into flash and over-ride existing file
    return ESP_OK;
}

esp_err_t execute_lil_post_handler(httpd_req_t *req) 
{
    const char *uri = req->uri;
    char filepath[MAX_PATH_LENGTH];
    get_path_from_uri(uri + sizeof("/execute")-1, filepath);
    FILE *lil_fs = fopen(filepath, "r");
    if (!lil_fs) {
        ESP_LOGI(TAG, "Invalid request, could not find file: %s", filepath);
        return ESP_FAIL;
    }
    execute_lil_file(lil_fs);
    fclose(lil_fs);

    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri = "/*",
    .handler = http_get_file_handler,
    .method = HTTP_GET,
    .user_ctx = NULL
};

httpd_uri_t uri_post_upload = {
    .uri = "/upload/*",
    .handler = file_upload_post_handler,
    .method = HTTP_POST,
    .user_ctx = NULL
};

httpd_uri_t uri_execute_post = {
    .uri = "/execute/*",
    .handler = execute_lil_post_handler,
    .method = HTTP_POST,
    .user_ctx = NULL
};

httpd_handle_t start_http_server(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_LOGI(TAG, "starting http server...");
    esp_err_t err = httpd_start(&server, &server_config);
    if (err == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post_upload);
        httpd_register_uri_handler(server, &uri_execute_post);
        ESP_LOGI(TAG, "started server successfully!");
    } else {
        ESP_LOGI(TAG, "Failed to start server: %s", esp_err_to_name(err));
    }

    return server;
}