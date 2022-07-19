#include "esp_log.h"
#include "driver/i2c.h"

#include "bme280.h"

char *TAG = "I2C TEMPERATURE SENSOR TEST";

#define SDA_PIN GPIO_NUM_23
#define SCL_PIN GPIO_NUM_22

struct bme280_dev device;

void i2c_master_init(void) 
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000 //Max: 3400000 - 3.4 mhz from bme280 datasheet
    };
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0 ,0);
}

int8_t bme280_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) 
{
    ESP_LOGI(TAG, "Called bus read function, len:%d", len);

    int8_t result = 0; //0 for success, nonzero for failure

    uint8_t device_addr = *(uint8_t*)intf_ptr;

    ESP_LOGI(TAG, "address: %d", device_addr);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr<<1) | I2C_CMD_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t esp_result = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    
    ESP_LOGI(TAG, "Result from bus write main.c:%s", esp_err_to_name(esp_result));

    
    if (esp_result == ESP_OK) {
        result = BME280_OK;
    } else {
        result = BME280_E_COMM_FAIL;
    }
    //bm280 data sheet 6.2.2 I2C read outlines format for reading register data

    /*i2c_master_start(cmd); //Send device address and register address
    i2c_master_write_byte(cmd, (device_addr<<1) | I2C_CMD_WRITE, true); //(dev_addr<<1) | I2C_CMD_READ shifts dev_addr to the left one bit and makes the last bit a 0 (cmd_read)
    i2c_master_write_byte(cmd, reg_addr, true);
   // i2c_master_stop(cmd);

    //esp_err_t esp_result_w = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS); //Send commands
    
    //ESP_LOGI(TAG, "Result from bus write main.c:%s", esp_err_to_name(esp_result_w));

    i2c_master_start(cmd); //Start getting data
    i2c_master_write_byte(cmd, (device_addr<<1) | I2C_CMD_READ, true);
    
    //if (len > 1) { //Read more than one address, starting at reg_addr
    //    i2c_master_read(cmd, reg_data, len-1, I2C_MASTER_ACK); //Read until the last address
    //}
    i2c_master_read_byte(cmd, reg_data+len-1, I2C_MASTER_LAST_NACK); //Read the last byte
    i2c_master_stop(cmd);

    ESP_LOGI(TAG, "added all commands, beginning the command...");

    esp_err_t esp_result_r = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS); //Send commands
    
    ESP_LOGI(TAG, "Result from bus read main.c:%s", esp_err_to_name(esp_result_r));


    if (esp_result_r == ESP_OK) {
        result = BME280_OK;
    } else {
        result = BME280_E_COMM_FAIL;
    }

    i2c_cmd_link_delete(cmd);
    */
    return result; 
}
//signed char (*)(unsigned char,  const unsigned char *, unsigned int,  void *)'} from incompatible pointer type 'int8_t (*)(uint8_t,  uint8_t *, uint32_t,  void *)' {aka 
//signed char (*)(unsigned char,  const unsigned char *, unsigned int,  void *)'} [-Wincompatible-pointer-types]
     //device.write = &bme280_bus_write;
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
    esp_err_t esp_result = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_RATE_MS);
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

int8_t setup_bme280_driver(void) {
    int8_t result = BME280_OK;
    uint8_t device_addr = BME280_I2C_ADDR_SEC;

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

int8_t stream_sensor_data_normal_mode(void *pvParameters)
{   
    struct bme280_dev *dev = (struct bme280_dev *)pvParameters;
	int8_t rslt;
	uint8_t settings_sel;
	struct bme280_data comp_data;

	/* Recommended mode of operation: Indoor navigation */
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
		/* Delay while the sensor completes a measurement */
		dev->delay_us(7000, dev->intf_ptr);
		rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
		ESP_LOGI(TAG, "%lf", comp_data.temperature);
	}

	return rslt;
}

/*s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
    s32 iError = BME280_INIT_VALUE;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    //bm280 data sheet 6.2.2 I2C read outlines format for reading register data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr<<1) | I2C_CMD_WRITE, true); //(dev_addr<<1) | I2C_CMD_READ shifts dev_addr to the left one bit and makes the last bit a 0 (cmd_read)
    i2c_master_write_byte(cmd, reg_addr, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr<<1) | I2C_CMD_READ, true);
    
    if (cnt > 1) {
        i2c_master_read(cmd, reg_data, cnt-1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_RATE_MS);

    return (s8)iError;
}*/



void app_main(void) 
{
    ESP_LOGI(TAG, "setting up i2c...");
    i2c_master_init();
    //ESP_LOGI(TAG, "set up i2c, setting up the bme280 driver...");
    //if (setup_bme280_driver() != BME280_OK) {
    //    ESP_LOGE(TAG, "Failed to setup bme280 driver.");
    //    return;
    //}

    uint8_t data, addr;
    addr = 0x77;
    
    bme280_bus_read(0xF7, &data, 1, &addr);

    ESP_LOGI(TAG, "tried to read from register, data:%d", data);
    //ESP_LOGI(TAG, "set up the bme280 driver, tryign to stream data...");
    //xTaskCreate(stream_sensor_data_normal_mode, "Stream sensor data", 2048, &device, 6, NULL);
}