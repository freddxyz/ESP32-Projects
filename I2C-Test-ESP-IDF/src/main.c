/* i2c - Simple example
   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.
   The sensor used in this example is a MPU9250 inertial measurement unit.
   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples
   See README.md file to get detailed usage of this example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "bme280.h"

static const char *TAG = "i2c-simple-example";

#define I2C_MASTER_SCL_IO           22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           23      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */

struct bme280_dev device;

int8_t bme280_bus_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t result = 0; //0 for success, nonzero for failure

    uint8_t device_addr = *(uint8_t*)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_CMD_WRITE, true);

    i2c_master_write_byte(cmd, reg_addr, true);
    //i2c_master_write_byte(cmd, )
    i2c_master_write(cmd, reg_data, len, true);
    i2c_master_stop(cmd);
    esp_err_t esp_result = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS);
    if (esp_result == ESP_OK) {
        result = BME280_OK;
    } else {
        result = BME280_E_COMM_FAIL;
    }

    i2c_cmd_link_delete(cmd);

    return result;
}

void bme280_delay_us(uint32_t period, void *intf_ptr) 
{
    vTaskDelay( (period / 1000) / portTICK_RATE_MS);
} 

int8_t bme280_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) 
{
    esp_err_t esp_result = ESP_OK;

    ESP_LOGI(TAG, "Called bus read function, len:%d", len);

    int8_t result = 0; //0 for success, nonzero for failure

    uint8_t device_address = *(uint8_t*)intf_ptr;

    ESP_LOGI(TAG, "address: %d", device_address);

    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    
    assert (handle != NULL);

    esp_result = i2c_master_start(handle);
    if (esp_result != ESP_OK) {
        goto end;
    }

    esp_result = i2c_master_write_byte(handle, (device_address << 1) | I2C_MASTER_WRITE, true);
    if (esp_result != ESP_OK) {
        goto end;
    }

    esp_result = i2c_master_write_byte(handle, reg_addr, true);
    if (esp_result != ESP_OK) {
        goto end;
    }

    esp_result = i2c_master_start(handle);
    if (esp_result != ESP_OK) {
        goto end;
    }

    esp_result = i2c_master_write_byte(handle, (device_address << 1) | I2C_MASTER_READ, true);
    if (esp_result != ESP_OK) {
        goto end;
    }

    esp_result = i2c_master_read(handle, reg_data, len, I2C_MASTER_LAST_NACK);
    if (esp_result != ESP_OK) {
        goto end;
    }

    i2c_master_stop(handle);
    esp_result = i2c_master_cmd_begin(I2C_NUM_0, handle, 1000 / portTICK_PERIOD_MS);

end:
    i2c_cmd_link_delete_static(handle);
    if (esp_result != ESP_OK) {
        result = BME280_E_COMM_FAIL;
    } else {
        result = BME280_OK;
    }

    ESP_LOGI(TAG, "READ WRITE ERROR: %s", esp_err_to_name(esp_result));

    return result; 
}

int8_t setup_bme280_driver(void) {
    int8_t result = BME280_OK;
    uint8_t device_addr = BME280_I2C_ADDR_PRIM;

    ESP_LOGI(TAG, "did the variable stuff");

    device.intf_ptr = &device_addr;
    device.intf = BME280_I2C_INTF;
    device.read = &bme280_bus_read;
    device.write = &bme280_bus_write;
    device.delay_us = &bme280_delay_us;

    ESP_LOGI(TAG, "set all the values");

    result = bme280_init(&device);

    ESP_LOGI(TAG, "%d", result);

    return result;
}

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

int8_t stream_sensor_data_normal_mode(void *pvParameters)
{   
    struct bme280_dev *dev = (struct bme280_dev *)pvParameters;
	int8_t rslt;
	uint8_t settings_sel;
	struct bme280_data comp_data;

	//Recommended mode of operation: Indoor navigation 
	dev->settings.osr_h = BME280_OVERSAMPLING_1X;
	dev->settings.osr_p = BME280_OVERSAMPLING_16X;
	dev->settings.osr_t = BME280_OVERSAMPLING_2X;
	dev->settings.filter = BME280_FILTER_COEFF_16;
	dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

    ESP_LOGI(TAG, "set oversampling settings");

	settings_sel = BME280_OSR_PRESS_SEL;
	settings_sel |= BME280_OSR_TEMP_SEL;
	settings_sel |= BME280_OSR_HUM_SEL;
	settings_sel |= BME280_STANDBY_SEL;
	settings_sel |= BME280_FILTER_SEL;
	rslt = bme280_set_sensor_settings(settings_sel, dev);
	rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);

    ESP_LOGI(TAG, "set settings");

	printf("Temperature, Pressure, Humidity\r\n");
	while (1) {
		// Delay while the sensor completes a measurement 
		dev->delay_us(7000, dev->intf_ptr);
		rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
		ESP_LOGI(TAG, "%lf", comp_data.temperature);
	}

	return rslt;
}

void app_main(void)
{
    uint8_t data[2];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    uint8_t addr = 0x76;
    bme280_bus_read(0xF7, data, 1, &addr);
    ESP_LOGI(TAG, "OTHER DATA: %X", data[0]);

    ESP_LOGI(TAG, "setting up bme280 driver: %d", setup_bme280_driver());

    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C de-initialized successfully");

    xTaskCreate(stream_sensor_data_normal_mode, "Stream bme280 data normal", 2048, &device, 6, NULL);    
}

















/*#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "bme280.h"

char *TAG = "I2C TEMPERATURE SENSOR TEST";

#define SDA_PIN GPIO_NUM_23
#define SCL_PIN GPIO_NUM_22

struct bme280_dev device;






void app_main(void) 
{
    ESP_LOGI(TAG, "setting up i2c...");
    ESP_ERROR_CHECK(i2c_master_init());
    //ESP_LOGI(TAG, "set up i2c, setting up the bme280 driver...");
    if (setup_bme280_driver() != BME280_OK) {
        ESP_LOGE(TAG, "Failed to setup bme280 driver.");
        return;
    }

    //uint8_t data, addr;
    //addr = 0x77;
    
    //bme280_bus_read(0xF7, &data, 1, &addr);

    //ESP_LOGI(TAG, "tried to read from register, data:%d", data);
    //ESP_LOGI(TAG, "set up the bme280 driver, tryign to stream data...");
    //xTaskCreate(stream_sensor_data_normal_mode, "Stream sensor data", 2048, &device, 6, NULL);
}*/