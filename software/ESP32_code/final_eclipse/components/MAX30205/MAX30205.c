#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/i2c.h"

#include "MAX30205.h"

esp_err_t max30205_check_conf (uint8_t sent_cmd);

esp_err_t max30205_init(uint8_t conf_value, uint8_t* conf_received){
    esp_err_t ret = max30205_write_conf (conf_value);
    if(ret){
        return ret;
    }

    vTaskDelay(1 / portTICK_RATE_MS);

    ret = max30205_read_conf (conf_received);
    if(ret){
        return ret;
    }

    if(conf_value == *conf_received){
        printf("MAX30205 correctly initialized.\n");
    	return ESP_OK;
    } else {
    	printf("Conf. register of MAX30205 received (%d) mismatch with the sent register. Initialization failed...\n", conf_received);
    }
    return ESP_FAIL;
}

esp_err_t max30205_gettemp(uint8_t* data_h, uint8_t* data_l){
	xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30205_SENSOR_ADDR | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30205_CMD_TEMP, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30205_SENSOR_ADDR | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_h, ACK_VAL);
    i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t max30205_poweroff(){
    uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30205_read_conf(&old_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    new_reg = old_reg | 1;
    ret = max30205_write_conf(new_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30205_poweron(){
    uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30205_read_conf(&old_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    new_reg = old_reg & 254;
    ret = max30205_write_conf(new_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30205_one_shot(){
    uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30205_read_conf(&old_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    new_reg = old_reg | 128;
    ret = max30205_write_conf(new_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30205_one_shot_and_read(uint8_t* data_h, uint8_t* data_l){
    uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30205_read_conf(&old_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    new_reg = old_reg | 128;
    
    ret = max30205_write_conf(new_reg);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    
    vTaskDelay( 50 / portTICK_RATE_MS); //Max. conversion time
    
    ret = max30205_gettemp(data_h, data_l);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

float convert_temp(uint8_t data_h, uint8_t data_l){
    uint8_t temp1 = 0;
    uint8_t temp3 = 0;

    float temp2d = 0;
    float temp3d = 0;
    
    int i = 0;
    int j = 8;
    for(i = 0; i<8; i++){
        temp3 = pow(2,i);
        temp1 = data_h & temp3;
        if(temp1 == temp3){
            temp2d = temp2d + temp3;
        }
    }

    for(i = 0; i<8; i++){
        temp3 = pow(2,i);
        temp3d = pow(2,-j);
        temp1 = data_l & temp3;
        if(temp1 == temp3){
            temp2d = temp2d + temp3d;
        }
        j--;
    }
    
    return temp2d;
}

esp_err_t max30205_write_conf (uint8_t conf_value){
	xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30205_SENSOR_ADDR | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30205_CMD_CONF, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, conf_value, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS); //was 1000 / portTICK_RATE_MS
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);
    if (ret == ESP_FAIL) {
        return ret;
    }
    
    ets_delay_us(DELAY_CMD_US);

    ret = max30205_check_conf(conf_value);

    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30205_read_conf (uint8_t *value){
	xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30205_SENSOR_ADDR | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30205_CMD_CONF, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30205_SENSOR_ADDR | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, value, NACK_VAL);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30205_check_conf (uint8_t sent_cmd){
    uint8_t conf_rcvd;

    int ret = max30205_read_conf(&conf_rcvd);
    if (ret == ESP_FAIL) {
        return ret;
    }

    if(sent_cmd == conf_rcvd){
        //printf("Change on MAX30205 correctly processed.\n");
        return ESP_OK;
    } else {
        printf("Conf. register received (%d) mismatch with the sent register(%d). Something goes wrong...\n", conf_rcvd, sent_cmd);
    }
    return ESP_FAIL;
}
