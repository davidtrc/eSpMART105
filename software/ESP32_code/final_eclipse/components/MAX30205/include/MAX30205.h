#ifndef MAX30205_H_
#define MAX30205_H_

#include "esp_system.h"
#include "driver/i2c.h"
#include "i2c_common_conf.h"

#define DELAY_CMD_US            3

#define MAX30205_CMD_TEMP    	0x00    /*!< Command to measure temp*/
#define MAX30205_CMD_CONF    	0x01    /*!< Command to configure MAX30205*/
#define MAX30205_CMD_THYS    	0x02    /*!< Command to configure THYST*/
#define MAX30205_CMD_TOS    	0x03    /*!< Command to configure TOS*/
#define MAX30205_SENSOR_ADDR   	0x90   /*!< slave address for MAX30205 sensor */
#define WARMUP_TEMP             36

esp_err_t max30205_init(uint8_t conf_value, uint8_t* conf_received);

esp_err_t max30205_gettemp(uint8_t* data_h, uint8_t* data_l);

esp_err_t max30205_poweroff();

esp_err_t max30205_poweron();

esp_err_t max30205_one_shot();

esp_err_t max30205_one_shot_and_read(uint8_t* data_h, uint8_t* data_l);

float convert_temp(uint8_t data_h, uint8_t data_l);

esp_err_t max30205_write_conf (uint8_t conf_value);

esp_err_t max30205_read_conf (uint8_t *value);

#endif
