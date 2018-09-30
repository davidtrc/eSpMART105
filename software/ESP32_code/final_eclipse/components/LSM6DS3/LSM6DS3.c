#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include <time.h>
#include <sys/time.h>

#include "driver/i2c.h"

#include "LSM6DS3.h"

/* CONFIGURATION VARIABLES */
static uint8_t acc_data_rate = 3;
static uint8_t acc_full_scale = 1;
static uint8_t hp_acc_cuttoff_freq = 3; // ODR_XL/400
static uint8_t gyro_data_rate = 3;
static uint8_t gyro_full_scale = 3;
static uint8_t hp_gyro_cuttoff_freq = 1;

static uint8_t doubletap_threshold = 1; //From 0 to 32. Value*Full_scale/2^5 = 10*16/2^5 = 5G
static uint8_t doubletap_shock_time = 1; // value*8*ODR_XL (if 52 Hz) = value*0.1539 s. if 0 = 4*ODR_XL = 0.0769 s
static uint8_t doubletap_quiet_time = 1; // value*4*ODR_XL (if 52 Hz) = value*0.0769 s. if 0 = 2*ODR_XL = 0.0385 s
static uint8_t doubletap_duration_time = 1; // value*32*ODR_XL (if 52 Hz) = value*0.6154 s. if 0 = 16*ODR_XL = 0.3077 s

static uint8_t free_fall_duration = 2; // value/ODR_XL = 2/52 = 38.46 ms
static uint8_t free_fall_threshold = 5;// Look table in datasheet. 3 = 312 mg

static uint8_t pedo_threshold = 15; //value*32mg
static uint8_t pedo_debouncing_time = 31; //value*80ms
static uint8_t pedo_minimum_steps = 6;
static uint8_t FIFO_mode = 6; //Continuous mode
/* END OF CONFIGURATION VARIABLES */

lsm6ds3_err_t lsm6ds3_write_reg (uint8_t reg, uint8_t conf_value){
	xSemaphoreTake(print_mux, portMAX_DELAY);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LSM6DS3_ADDR<<1 | WRITE_BIT), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, conf_value, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	int ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);
    if (ret) {return ret;}
    return LSM6DS3_OK;
}

lsm6ds3_err_t lsm6ds3_read_reg (uint8_t reg, uint8_t *value){
	int ret;

	xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LSM6DS3_ADDR<<1 | WRITE_BIT), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LSM6DS3_ADDR<<1 | READ_BIT), ACK_CHECK_EN);
    i2c_master_read_byte(cmd, value, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    if (ret) {return ret;}
    return LSM6DS3_OK;
}

lsm6ds3_err_t lsm6ds3_enable_configuration(){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(FUNC_CFG_ACCESS, 128);

	return ret;
}

lsm6ds3_err_t lsm6ds3_disable_configuration(){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(FUNC_CFG_ACCESS, 0);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold(uint8_t value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(FIFO_CTRL1, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_threshold(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_CTRL1, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_pedometer_and_step_counter_on_FIFO(){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp | 128;

	ret = lsm6ds3_write_reg(FIFO_CTRL2, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_disable_pedometer_and_step_counter_on_FIFO(){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 127;

	ret = lsm6ds3_write_reg(FIFO_CTRL2, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_basedon_XLGIRO_dataready(){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 191;

	ret = lsm6ds3_write_reg(FIFO_CTRL2, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_basedon_step_detected(){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp | 64;

	ret = lsm6ds3_write_reg(FIFO_CTRL2, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_on_CTRL2(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp >>4;
	value_temp = value_temp <<4;

	value_temp = value_temp | value;

	ret = lsm6ds3_write_reg(FIFO_CTRL1, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_threshold_on_CTRL2(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_CTRL2, value);

	*value = (*value)<<4;
	*value = (*value)>>4;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_gyro_decimation(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL3, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp <<5;
	value_temp = value_temp >>5;

	value = value <<3;
	value_temp = value_temp | value;

	ret = lsm6ds3_write_reg(FIFO_CTRL3, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_gyro_decimation(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_CTRL3, value);
	*value = (*value) >>3;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_acc_decimation(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL3, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp >>3;
	value_temp = value_temp <<3;

	value_temp = value_temp | value;

	ret = lsm6ds3_write_reg(FIFO_CTRL3, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_acc_decimation(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_CTRL3, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_FIFO_store_only_MSB(bool do_or_not){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	ret = lsm6ds3_read_reg(FIFO_CTRL4, &value_temp);
	if(ret){return ret;}
	value_temp = value_temp <<2;
	value_temp = value_temp >>2;
	if(do_or_not){
		ret = lsm6ds3_write_reg(FIFO_CTRL4, (value_temp | 0b01000000));
	} else{
		ret = lsm6ds3_write_reg(FIFO_CTRL4, (value_temp & 0b10111111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_pedo_decimation(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL4, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp >>3;
	value_temp = value_temp <<3;

	value_temp = value_temp | (value<<3);

	ret = lsm6ds3_write_reg(FIFO_CTRL4, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_pedo_decimation(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_CTRL4, value);
	*value = (*value)>>3;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_mode(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL5, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp >>3;
	value_temp = value_temp <<3;

	ret = lsm6ds3_write_reg(FIFO_CTRL5, (value_temp | value));

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_operational_data_rate(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FIFO_CTRL5, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp <<5;
	value_temp = value_temp >>5;

	ret = lsm6ds3_write_reg(FIFO_CTRL5, (value_temp | (value<<3)));

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_user_orientation(uint8_t value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(ORIENT_CFG_G, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_user_orientation(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, value);

	*value = (*value)<<4;
	*value = (*value)>>4;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_X_pitch(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, value);

	*value = (*value)>>5;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_X_pitch(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp | (value <<5);

	ret = lsm6ds3_write_reg(ORIENT_CFG_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_Y_roll(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, value);

	*value = (*value)<<3;
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_Y_roll(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp | (value <<4);

	ret = lsm6ds3_write_reg(ORIENT_CFG_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_Z_yaw(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, value);

	*value = (*value)<<4;
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_Z_yam(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(ORIENT_CFG_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp | (value <<3);

	ret = lsm6ds3_write_reg(ORIENT_CFG_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_int1(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(INT1_CTRL, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_step_interruption(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 128;
	}else{
		value_temp = value_temp & 127;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_significant_motion_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 64;
	}else{
		value_temp = value_temp & 191;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_full_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 32;
	}else{
		value_temp = value_temp & 223;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_overrun_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 16;
	}else{
		value_temp = value_temp & 239;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 8;
	}else{
		value_temp = value_temp & 247;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_boot_status_available_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 4;
	}else{
		value_temp = value_temp & 251;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_ready_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 2;
	}else{
		value_temp = value_temp & 253;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_data_ready_interruption_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT1_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 1;
	}else{
		value_temp = value_temp & 254;
	}

	ret = lsm6ds3_write_reg(INT1_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_int2(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(INT2_CTRL, value);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_step_on_delta_time_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 128;
	}else{
		value_temp = value_temp & 127;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_step_counter_overflow_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 64;
	}else{
		value_temp = value_temp & 191;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_full_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 32;
	}else{
		value_temp = value_temp & 223;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_overrun_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 16;
	}else{
		value_temp = value_temp & 239;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 8;
	}else{
		value_temp = value_temp & 247;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_temperature_data_ready_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 4;
	}else{
		value_temp = value_temp & 251;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_gyroscope_data_ready_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 2;
	}else{
		value_temp = value_temp & 253;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_data_ready_interruption_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT2_CTRL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 1;
	}else{
		value_temp = value_temp & 254;
	}

	ret = lsm6ds3_write_reg(INT2_CTRL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_output_data_rate(uint8_t data_rate){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL1_XL, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp<<4;
	value_temp = value_temp>>4;

	value_temp = value_temp | (data_rate<<4);

	ret = lsm6ds3_write_reg(CTRL1_XL, value_temp);
	acc_data_rate = data_rate;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_full_scale(uint8_t scale){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;

	ret = lsm6ds3_read_reg(CTRL1_XL, &value_temp);
	if(ret){ return ret;}

	value_temp2 = value_temp>>5;
	value_temp = value_temp<<6;
	value_temp = value_temp>>6;
	value_temp = (value_temp2<<4) | (scale<<2) | (value_temp);

	ret = lsm6ds3_write_reg(CTRL1_XL, value_temp);
	acc_full_scale = scale;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_antialiasing_filter_bw(uint8_t bw){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL1_XL, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp>>2;
	value_temp = value_temp<<2;
	value_temp = value_temp | bw;

	ret = lsm6ds3_write_reg(CTRL1_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_gyroscope_data_rate(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL2_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp<<4;
	value_temp = value_temp>>4;
	value_temp = value_temp | (value<<4);

	ret = lsm6ds3_write_reg(CTRL2_G, value_temp);
	gyro_data_rate = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_gyroscope_fullscale_selection(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;

	ret = lsm6ds3_read_reg(CTRL2_G, &value_temp);
	if(ret){ return ret;}

	value_temp2 = value_temp>>4;
	value_temp = value_temp<<6;
	value_temp = value_temp>>6;
	value_temp = (value_temp2<<4) | value | (value_temp);

	ret = lsm6ds3_write_reg(CTRL2_G, value_temp);
	gyro_full_scale = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_gyroscope_fullscale_at_125dps(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL2_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp>>2;
	value_temp = value_temp<<2;
	value_temp = value_temp | (value<<1);

	ret = lsm6ds3_write_reg(CTRL2_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_reboot_memory_content(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp<<1;
	value_temp = value_temp>>1;
	value_temp = value_temp | (value<<7);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_block_data_update(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp2 = value_temp>>7;
	value_temp = value_temp<<2;
	value_temp = value_temp>>2;
	value_temp = value_temp2 | (value<<6) | value_temp;

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_interuption_activation_level(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 223;
	value_temp = value_temp | (value<<5);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_push_or_opendrain_in_int_lines(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 239;
	value_temp = value_temp | (value<<4);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_autoincrement_register_address_when_burst(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 251;
	value_temp = value_temp | (value<<2);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_big_or_little_endian(uint8_t value){ //0 = LSB, 1 = MSB
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 253;
	value_temp = value_temp | (value<<1);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_software_reset(uint8_t value){ //1=reset
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL3_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 254;
	value_temp = value_temp | (value);

	ret = lsm6ds3_write_reg(CTRL3_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_bw_selection(uint8_t value){ //0 =determined by ODR selection, 1 = determined by BW_XL in CTRL1_XL
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 127;
	value_temp = value_temp | (value<<7);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyro_sleep_mode(uint8_t value){ //0=disabled, 1=enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 191;
	value_temp = value_temp | (value<<6);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_all_int_on_int1(uint8_t value){ //0= divided between two pads, 1=all in INT1
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 223;
	value_temp = value_temp | (value<<5);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_temperature_in_FIFO(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 239;
	value_temp = value_temp | (value<<4);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_data_masking_until_correct_initialization(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 247;
	value_temp = value_temp | (value<<3);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_disable_i2c(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 251;
	value_temp = value_temp | (value<<2);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_FIFO_threshold(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL4_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 253;
	value_temp = value_temp | (value);

	ret = lsm6ds3_write_reg(CTRL4_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_burst_mode_rounding(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL5_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 31;
	value_temp = value_temp | (value<<5);

	ret = lsm6ds3_write_reg(CTRL5_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_angular_self_test_enable(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL5_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 243;
	value_temp = value_temp | (value<<2);

	ret = lsm6ds3_write_reg(CTRL5_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_self_test_enable(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL5_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 252;
	value_temp = value_temp | (value);

	ret = lsm6ds3_write_reg(CTRL5_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_edge_sensitive_trigger(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL6_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 127;
	value_temp = value_temp | (value<<7);

	ret = lsm6ds3_write_reg(CTRL6_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_level_sensitive_trigger(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL6_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 191;
	value_temp = value_temp | (value<<6);

	ret = lsm6ds3_write_reg(CTRL6_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_latched_sensitive_trigger(uint8_t value){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL6_C, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 223;
	value_temp = value_temp | (value<<5);

	ret = lsm6ds3_write_reg(CTRL6_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_high_performance_mode(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL6_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 16;
	} else{
		value_temp = value_temp & 239;
	}

	ret = lsm6ds3_write_reg(CTRL6_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_high_performance_mode(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL7_G, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 128;
	} else{
		value_temp = value_temp & 127;
	}

	ret = lsm6ds3_write_reg(CTRL7_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_gyroscope_hp_filter(bool en_or_dis){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL7_G, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 64;
	} else{
		value_temp = value_temp & 191;
	}

	ret = lsm6ds3_write_reg(CTRL7_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_reset_gyroscope_hp_filter(bool en_or_dis){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL7_G, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 8;
	} else{
		value_temp = value_temp & 247;
	}

	ret = lsm6ds3_write_reg(CTRL7_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_source_rounding(bool en_or_dis){ //Source register rounding function enable on STATUS_REG (1Eh), FUNC_SRC(53h) and WAKE_UP_SRC (1Bh) registers
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL7_G, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 4;
	} else{
		value_temp = value_temp & 251;
	}

	ret = lsm6ds3_write_reg(CTRL7_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyroscope_hp_cutoff_frequency(uint8_t value){ //Source register rounding function enable on STATUS_REG (1Eh), FUNC_SRC(53h) and WAKE_UP_SRC (1Bh) registers
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL7_G, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 207;
	value_temp = value_temp |(value<<4);

	ret = lsm6ds3_write_reg(CTRL7_G, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_LPF2_selection(bool en_or_dis){ //0=slope_fds, 1 = func_en
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL8_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
			value_temp = value_temp | 128;
	} else{
		value_temp = value_temp & 127;
	}

	ret = lsm6ds3_write_reg(CTRL8_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_LPF2_set_slope_and_cuttoff_freq(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL8_XL, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 159;
	value_temp = value_temp | (value << 5);

	ret = lsm6ds3_write_reg(CTRL8_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_LPF2_selection_slope_or_hpf(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL8_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 4;
	} else{
		value_temp = value_temp & 251;
	}

	ret = lsm6ds3_write_reg(CTRL8_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_acc_LPF_on_6D6(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL8_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 1;
	} else{
		value_temp = value_temp & 254;
	}

	ret = lsm6ds3_write_reg(CTRL8_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_Z_output_enable(bool en_or_dis){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL9_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
			value_temp = value_temp | 32;
	} else{
		value_temp = value_temp & 223;
	}

	ret = lsm6ds3_write_reg(CTRL9_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_Y_output_enable(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL9_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 16;
	} else{
		value_temp = value_temp & 239;
	}

	ret = lsm6ds3_write_reg(CTRL9_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_X_output_enable(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL9_XL, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 8;
	} else{
		value_temp = value_temp & 247;
	}

	ret = lsm6ds3_write_reg(CTRL9_XL, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyro_Z_output_enable(bool en_or_dis){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
			value_temp = value_temp | 32;
	} else{
		value_temp = value_temp & 223;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyro_Y_output_enable(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 16;
	} else{
		value_temp = value_temp & 239;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_gyro_X_output_enable(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 8;
	} else{
		value_temp = value_temp & 247;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_enable_embedded_functions_and_acc_filters(bool en_or_dis){ //0=disabled, 1 = enabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 4;
	} else{
		value_temp = value_temp & 251;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_reset_pedometer(bool en_or_dis){ //1=enabled, 0 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 2;
	} else{
		value_temp = value_temp & 253;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_significat_motion_function(bool en_or_dis){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(CTRL10_C, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 1;
	} else{
		value_temp = value_temp & 254;
	}

	ret = lsm6ds3_write_reg(CTRL10_C, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_wake_up_src(uint8_t *value){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(WAKE_UP_SRC, value);
	return ret;
}

lsm6ds3_err_t lsm6ds3_get_tap_src(uint8_t *value){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(TAP_SRC, value);
	return ret;
}

lsm6ds3_err_t lsm6ds3_get_d6d_src(uint8_t *value){ //0=enabled, 1 = disabled
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(D6D_SRC, value);
	return ret;
}

lsm6ds3_err_t lsm6ds3_get_temperature(uint16_t *temperature){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(OUT_TEMP_L, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUT_TEMP_H, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*temperature = (value_temp3<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_pitch_X(int16_t *pitch){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	int16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTX_L_G, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTX_H_G, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*pitch = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_roll_Y(int16_t *roll){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	int16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTY_L_G, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTY_H_G, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*roll = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_yaw_Z(int16_t *yaw){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	int16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTZ_L_G, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTZ_H_G, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*yaw = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_acc_X(int16_t *acc){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTX_L_XL, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTX_H_XL, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*acc = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_acc_Y(int16_t *acc){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTY_L_XL, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTY_H_XL, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*acc = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_acc_Z(int16_t *acc){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(OUTZ_L_XL, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(OUTZ_H_XL, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*acc = (value_temp3<<8) |value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_unread_samples(uint16_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(FIFO_STATUS1, &value_temp);
	printf("VT: %d\n", value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(FIFO_STATUS2, &value_temp2);
	printf("VT2: %d\n", value_temp2);
	if(ret){ return ret;}
	value_temp2 = value_temp2 & 0x0F;
	value_temp3 = value_temp2;

	*value = (value_temp3<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_watermark_status(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_STATUS2, value);
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_overrun_status(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_STATUS2, value);
	*value = (*value)<<1;
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_if_FIFO_is_full(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_STATUS2, value);
	*value = (*value)<<2;
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_if_FIFO_is_empty(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FIFO_STATUS2, value);
	*value = (*value)<<3;
	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_pattern(uint16_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(FIFO_STATUS3, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(FIFO_STATUS4, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*value = (value_temp3<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_data(uint16_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(FIFO_DATA_OUT_L, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(FIFO_DATA_OUT_H, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*value = (value_temp3<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_timestamp(uint32_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint8_t value_temp3;
	uint16_t value_temp4;
	uint32_t value_temp5;

	ret = lsm6ds3_read_reg(TIMESTAMP0_REG, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(TIMESTAMP1_REG, &value_temp2);
	if(ret){ return ret;}
	value_temp4 = value_temp2;

	ret = lsm6ds3_read_reg(TIMESTAMP2_REG, &value_temp3);
	if(ret){ return ret;}
	value_temp5 = value_temp3;

	*value = (value_temp5<<16) | (value_temp4<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_reset_FIFO_timestamp(){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(TIMESTAMP2_REG, 0xAA);
	if(ret){ return ret;}

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_step_timestamp(uint16_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(STEP_TIMESTAMP_L, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(STEP_TIMESTAMP_H, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*value = (value_temp3<<8) | value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_FIFO_step_counter(uint16_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;
	uint16_t value_temp3;

	ret = lsm6ds3_read_reg(STEP_COUNTER_L, &value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(STEP_COUNTER_H, &value_temp2);
	if(ret){ return ret;}
	value_temp3 = value_temp2;

	*value = ((value_temp3<<8) | value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_func_src(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FUNC_SRC, value);
	if(ret){ return ret;}

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_step_recognition_on_deltatime(uint8_t *value){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_read_reg(FUNC_SRC, value);
	if(ret){ return ret;}

	*value = (*value)>>7;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_significant_motion_detection(uint8_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FUNC_SRC, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b01000000;
	value_temp = value_temp>>6;
	*value = value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_tilt_detection(uint8_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FUNC_SRC, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b00100000;
	value_temp = value_temp>>5;
	*value = value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_step_detection(uint8_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FUNC_SRC, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b00010000;
	value_temp = value_temp>>4;
	*value = value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_step_overflow(uint8_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FUNC_SRC, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b00010000;
	value_temp = value_temp>>3;
	*value = value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_get_hard_softiron_calculation_status(uint8_t *value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FUNC_SRC, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b00000010;
	value_temp = value_temp>>1;
	*value = value_temp;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_timestap_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b10000000));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b01111111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_pedometer_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b01000000));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b10111111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_tilt_calculation_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00100000));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11011111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_acc_filters_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00010000));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11101111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_X_tap_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00001000));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11110111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_Y_tap_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00000100));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11111011));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_Z_tap_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00000010));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11111101));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_latched_interruption_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_CFG, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp | 0b00000001));
	} else {
		ret = lsm6ds3_write_reg(TAP_CFG, (value_temp & 0b11111110));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_4D_orientation_detection_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_THS_6D, &value_temp);
	if(ret){ return ret;}
	if(en_or_dis){
		ret = lsm6ds3_write_reg(TAP_THS_6D, (value_temp | 0b10000000));
	} else {
		ret = lsm6ds3_write_reg(TAP_THS_6D, (value_temp & 0b011111111));
	}

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_D6D_function_threshold(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_THS_6D, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b10011111;
	ret = lsm6ds3_write_reg(TAP_THS_6D, (value_temp | (value<<5)));

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_tap_threshold(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(TAP_THS_6D, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b11100000;
	ret = lsm6ds3_write_reg(TAP_THS_6D, (value_temp | value));
	doubletap_threshold = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_doubletap_time_gap(uint8_t value){ //time = value*32*ODR_XL
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT_DUR2, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b00001111;
	ret = lsm6ds3_write_reg(INT_DUR2, (value_temp | (value<<4)));
	doubletap_duration_time = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_doubletap_quiet_time(uint8_t value){ //time = value*4*ODR_XL
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT_DUR2, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b11110011;
	ret = lsm6ds3_write_reg(INT_DUR2, (value_temp | (value<<2)));
	doubletap_quiet_time = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_doubletap_max_time_overthreshold_event(uint8_t value){ //time = value*8*ODR_XL
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(INT_DUR2, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b11111100;
	ret = lsm6ds3_write_reg(INT_DUR2, (value_temp | value) );
	doubletap_shock_time = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_doubletap_wakeup_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_THS, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b10000000;
	} else {
		value_temp = value_temp & 0b01111111;
	}
	ret = lsm6ds3_write_reg(WAKE_UP_THS, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_inactivity_wakeup_enable(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_THS, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b01000000;
	} else {
		value_temp = value_temp & 0b10111111;
	}
	ret = lsm6ds3_write_reg(WAKE_UP_THS, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_tap_and_doubletap_threshold(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_THS, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 0b00111111;
	ret = lsm6ds3_write_reg(WAKE_UP_THS, (value_temp | value) );

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_wake_up_duration_event(uint8_t value){ //Time = value*ODR_time
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_DUR, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 0b10011111;
	ret = lsm6ds3_write_reg(WAKE_UP_DUR, (value_temp | value) );

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_timestamp_register_resolution(bool ms_6_dot_4_or_us_25){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_DUR, &value_temp);
	if(ret){ return ret;}

	if(ms_6_dot_4_or_us_25){
		value_temp = value_temp | 0b00010000;
	} else {
		value_temp = value_temp & 0b11101111;
	}
	ret = lsm6ds3_write_reg(WAKE_UP_DUR, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_duration_togo_in_sleep_mode(uint8_t value){ //Time = value*ODR_time
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(WAKE_UP_DUR, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 0b11110000;
	ret = lsm6ds3_write_reg(WAKE_UP_DUR, (value_temp | value) );

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_free_fall_duration(uint8_t value){ //Time = value*ODR_time
	lsm6ds3_err_t ret;
	uint8_t value_temp;
	uint8_t value_temp2;

	ret = lsm6ds3_read_reg(WAKE_UP_DUR, &value_temp);
	if(ret){ return ret;}
	value_temp = value_temp & 0b01111111;
	value_temp2 = (value>>5);
	value_temp2 = (value_temp2<<7);

	value_temp = value_temp | value_temp2;
	ret = lsm6ds3_write_reg(WAKE_UP_DUR, value_temp);
	if(ret){ return ret;}

	ret = lsm6ds3_read_reg(FREE_FALL, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 0b00000111;
	ret = lsm6ds3_write_reg(FREE_FALL, (value_temp | (value<<3)) );

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_free_fall_threshold(uint8_t value){ //Time = value*ODR_time
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(FREE_FALL, &value_temp);
	if(ret){ return ret;}

	value_temp = value_temp & 0b11111000;
	ret = lsm6ds3_write_reg(FREE_FALL, (value_temp | value) );

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_inactivity_mode_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b10000000;
	} else {
		value_temp = value_temp & 0b01111111;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_single_tap_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b01000000;
	} else {
		value_temp = value_temp & 0b10111111;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_wakeup_event_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00100000;
	} else {
		value_temp = value_temp & 0b11011111;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_free_fall_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00010000;
	} else {
		value_temp = value_temp & 0b11101111;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_double_tap_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00001000;
	} else {
		value_temp = value_temp & 0b11110111;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_6D_event_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00000100;
	} else {
		value_temp = value_temp & 0b11111011;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_tilt_event_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00000010;
	} else {
		value_temp = value_temp & 0b11111101;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_end_of_counter_INT1(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00000001;
	} else {
		value_temp = value_temp & 0b11111110;
	}
	ret = lsm6ds3_write_reg(MD1_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_inactivity_mode_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b10000000;
	} else {
		value_temp = value_temp & 0b01111111;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_single_tap_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b01000000;
	} else {
		value_temp = value_temp & 0b10111111;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_wakeup_event_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00100000;
	} else {
		value_temp = value_temp & 0b11011111;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_free_fall_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00010000;
	} else {
		value_temp = value_temp & 0b11101111;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_double_tap_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00001000;
	} else {
		value_temp = value_temp & 0b11110111;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_6D_event_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD2_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00000100;
	} else {
		value_temp = value_temp & 0b11111011;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_tilt_event_INT2(bool en_or_dis){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(MD1_CFG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b00000010;
	} else {
		value_temp = value_temp & 0b11111101;
	}
	ret = lsm6ds3_write_reg(MD2_CFG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_pedo_fullscale_4G(bool en_or_dis){//0 = 2G, 1 = 4G
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(PEDO_THS_REG, &value_temp);
	if(ret){ return ret;}

	if(en_or_dis){
		value_temp = value_temp | 0b10000000;
	} else {
		value_temp = value_temp & 0b01111111;
	}
	ret = lsm6ds3_write_reg(PEDO_THS_REG, value_temp);

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_pedo_threshold(uint8_t threshold){ //threshold*32mg if 4G = 1, else threshold*16mg
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(PEDO_THS_REG, &value_temp);
	if(ret){ return ret;}

	value_temp = (value_temp & 0b11100000) | threshold;

	ret = lsm6ds3_write_reg(PEDO_THS_REG, value_temp);
	pedo_threshold = threshold;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_significant_motion_threshold(uint8_t threshold){ //threshold*32mg if 4G = 1, else threshold*16mg
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(SM_THS, threshold);
	return ret;
}

lsm6ds3_err_t lsm6ds3_set_pedo_debouncing_time(uint8_t time){ //time*80ms
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(PEDO_DEB_REG, &value_temp);
	if(ret){ return ret;}

	value_temp = (value_temp & 0b00000111) | time;

	ret = lsm6ds3_write_reg(PEDO_DEB_REG, value_temp);
	pedo_debouncing_time = time;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_pedo_minimum_steps_to_increase_counter(uint8_t value){
	lsm6ds3_err_t ret;
	uint8_t value_temp;

	ret = lsm6ds3_read_reg(PEDO_DEB_REG, &value_temp);
	if(ret){ return ret;}

	value_temp = (value_temp & 0b11111000) | value;

	ret = lsm6ds3_write_reg(PEDO_DEB_REG, value_temp);
	pedo_minimum_steps = value;

	return ret;
}

lsm6ds3_err_t lsm6ds3_set_step_count_delta(uint8_t delta_time){ //delta_time*1.6384 s
	lsm6ds3_err_t ret;

	ret = lsm6ds3_write_reg(STEP_COUNT_DELTA, delta_time);
	return ret;
}

double lsm6ds3_normalize_acc_read(int16_t value){
	uint16_t full_scale = 0xFFFF;
	switch (acc_full_scale){
		case 0:
			return (double)value*4/full_scale;
		case 1:
			return (double)value*32/full_scale;
		case 2:
			return (double)value*8/full_scale;
		case 3:
			return (double)value*16/full_scale;
		default:
			return 0;
	}
	return 0;
}

double lsm6ds3_normalize_gyro_read(int16_t value){
	uint16_t full_scale = 0xFFFF;
	switch (gyro_full_scale){
		case 0:
			return (double)value*250/full_scale;
		case 1:
			return (double)value*500/full_scale;
		case 2:
			return (double)value*1000/full_scale;
		case 3:
			return (double)value*2000/full_scale;
		default:
			return 0;
	}
	return 0;
}

lsm6ds3_err_t lsm6ds3_run_falls_algorithm(void){

	uint16_t fifo_pattern = 0;
	uint16_t fifo_data = 0;
	uint16_t unread_samples = 0;
	bool x_acc_sample = false;
	bool y_acc_sample = false;
	bool z_acc_sample = false;
	bool x_gyro_sample = false;
	bool y_gyro_sample = false;
	bool z_gyro_sample = false;
	int16_t x_acc_sample_value = 0;
	int16_t y_acc_sample_value = 0;
	int16_t z_acc_sample_value = 0;
	int16_t x_gyro_sample_value = 0;
	int16_t y_gyro_sample_value = 0;
	int16_t z_gyro_sample_value = 0;
	int64_t acc_sum = 0;
	int64_t gyro_sum = 0;
	struct timeval time_starting_measure;
	struct timeval current_measure_time;
	struct timeval time_for_detect_run;
	uint64_t time_measuring_ms = 0;
	uint64_t time_measuring_bounces = 0;
	bool impact_detected = false;
	bool impact_from_gyro_detected = false;
	bool motionless_detected = false;
	bool motionless_from_gyro_detected = false;
	uint8_t gyro_motionless_samples = 0;
	uint8_t acc_motionless_samples = 0;
	uint64_t current_acc_sample = 0;
	uint64_t previous_acc_sample = 0;
	uint64_t current_gyro_sample = 0;
	uint64_t previous_gyro_sample = 0;
	uint8_t fifo_status_2_val;

	gettimeofday(&time_starting_measure, NULL);

	while(time_measuring_ms<TIME_TH_FOR_DETECT_IMPACT_MS){
	    gettimeofday(&current_measure_time, NULL);
	    time_measuring_ms = (current_measure_time.tv_sec - time_starting_measure.tv_sec) * 1000 + (current_measure_time.tv_usec - time_starting_measure.tv_usec) / 1000;
	    lsm6ds3_get_FIFO_unread_samples(&unread_samples);
	    if(unread_samples == 0){
	    	unread_samples = 6;
	    	lsm6ds3_read_reg(FIFO_STATUS2, &fifo_status_2_val);
	    	if(fifo_status_2_val & 0b01100000){
	    		unread_samples = 3994;
	    	}
	    }
	    printf("US: %d\n", unread_samples);
	    for(int i = 0; i<unread_samples-6; i++){
	        lsm6ds3_get_FIFO_pattern(&fifo_pattern);
	        lsm6ds3_get_FIFO_data(&fifo_data);
	        switch(fifo_pattern){ //FIFO Pattern FOR THIS SPECIFIC CONFIGURATION (52 HZ gyro, 52 Hz acc, 52 Hz FIFO. No decimation)
	            case 0: //gyro_X
	                x_gyro_sample = true;
	                x_gyro_sample_value = fifo_data;
	                break;
	            case 1: //gyro_Y
	                y_gyro_sample = true;
	                y_gyro_sample_value = fifo_data;
	                break;
	            case 2: //gyro_Z
	                z_gyro_sample = true;
	                z_gyro_sample_value = fifo_data;
	                break;
	            case 3: //acc_X
	                x_acc_sample = true;
	                x_acc_sample_value = fifo_data;
	                break;
	            case 4: //acc_Y
	                y_acc_sample = true;
	                y_acc_sample_value = fifo_data;
	                break;
	            case 5: //acc_Z
	                z_acc_sample = true;
	                z_acc_sample_value = fifo_data;
	                break;
	            default:
	                break;

	        }

	        if(x_acc_sample && y_acc_sample && z_acc_sample){
	            current_acc_sample++;
	            acc_sum = abs(x_acc_sample_value) + abs(y_acc_sample_value) + abs(z_acc_sample_value);
	            //printf("%" PRId64 "\n", acc_sum);
	            if(acc_sum>=IMPACT_G_TH){
	                impact_detected = true;
	            }
	            x_acc_sample = false;
	            y_acc_sample = false;
	            z_acc_sample = false;
	        }
	        if(x_gyro_sample && y_gyro_sample && z_gyro_sample){
	            current_gyro_sample++;
	            gyro_sum = abs(x_gyro_sample_value) + abs(y_gyro_sample_value) + abs(z_gyro_sample_value);
	            //printf("%" PRId64 "\n", gyro_sum);
	            if(gyro_sum>=GYRO_TH_IMPACT){
	                impact_from_gyro_detected = true;
	            }
	            x_gyro_sample = false;
	            y_gyro_sample = false;
	            z_gyro_sample = false;
	        }
	        if(impact_detected){
	            printf("IMPACT!!!\n");
	            break;
	        }
	    }

	    if(impact_detected){
	        break;
	    }

	    vTaskDelay(10 / portTICK_PERIOD_MS);
	}

	if(!impact_detected){
	    return LSM6DS3_FALSE_ALARM;
	}

	printf("Looking for motionless...\n");
	gettimeofday(&time_for_detect_run, NULL);

	while(time_measuring_bounces<=TIME_TH_FOR_CONSIDER_RUN_MS){
	    gettimeofday(&current_measure_time, NULL);
	    time_measuring_bounces = (current_measure_time.tv_sec - time_for_detect_run.tv_sec) * 1000 + (current_measure_time.tv_usec - time_for_detect_run.tv_usec) / 1000;
	    lsm6ds3_get_FIFO_unread_samples(&unread_samples);
	    if(unread_samples == 0){
			unread_samples = 6;
			lsm6ds3_read_reg(FIFO_STATUS2, &fifo_status_2_val);
			if(fifo_status_2_val & 0b01100000){
				unread_samples = 3994;
			}
		}
	    printf("US: %d\n", unread_samples);
	    for(int i = 0; i<unread_samples-6; i++){
	        lsm6ds3_get_FIFO_pattern(&fifo_pattern);
	        lsm6ds3_get_FIFO_data(&fifo_data);
	        switch(fifo_pattern){ //FIFO Pattern FOR THIS SPECIFIC CONFIGURATION (52 HZ gyro, 52 Hz acc, 52 Hz FIFO. No decimation)
	            case 0: //gyro_X
	                x_gyro_sample = true;
	                x_gyro_sample_value = fifo_data;
	                break;
	            case 1: //gyro_Y
	                y_gyro_sample = true;
	                y_gyro_sample_value = fifo_data;
	                break;
	            case 2: //gyro_Z
	                z_gyro_sample = true;
	                z_gyro_sample_value = fifo_data;
	                break;
	            case 3: //acc_X
	                x_acc_sample = true;
	                x_acc_sample_value = fifo_data;
	                break;
	            case 4: //acc_Y
	                y_acc_sample = true;
	                y_acc_sample_value = fifo_data;
	                break;
	            case 5: //acc_Z
	                z_acc_sample = true;
	                z_acc_sample_value = fifo_data;
	                break;
	            default:
	                break;

	        }

	        if(x_acc_sample && y_acc_sample && z_acc_sample){
	            current_acc_sample++;
	            acc_sum = abs(x_acc_sample_value) + abs(y_acc_sample_value) + abs(z_acc_sample_value);
	            if(acc_sum<=MOTIONLESS_G_TH){
	                if((current_acc_sample-previous_acc_sample) == 1){
	                    acc_motionless_samples++;
	                    if(acc_motionless_samples >= CONSECUTIVE_SAMPLES_CONFIRM_MOTIONLESS){
	                        printf("Motionless from acc confirmed\n");
	                        motionless_detected = true;
	                    }
	                } else {
	                    acc_motionless_samples = 0;
	                }
	                previous_acc_sample = current_acc_sample;
	            }
	            x_acc_sample = false;
	            y_acc_sample = false;
	            z_acc_sample = false;

	        }

	        if(x_gyro_sample && y_gyro_sample && z_gyro_sample){
	            current_gyro_sample++;
	            gyro_sum = abs(x_gyro_sample_value) + abs(y_gyro_sample_value) + abs(z_gyro_sample_value);
	            if(gyro_sum<=GYRO_TH_MOTIONLESS){

	                if((current_gyro_sample-previous_gyro_sample) == 1){
	                    gyro_motionless_samples++;
	                    if(gyro_motionless_samples >= CONSECUTIVE_SAMPLES_CONFIRM_MOTIONLESS){
	                        printf("Motionless from gyro confirmed\n");
	                        motionless_from_gyro_detected = true;
	                    }
	                } else {
	                    gyro_motionless_samples = 0;
	                }

	                previous_gyro_sample = current_gyro_sample;
	            }

	            x_gyro_sample = false;
	            y_gyro_sample = false;
	            z_gyro_sample = false;
	        }

	        if(time_measuring_bounces>=TIME_TH_FOR_CONSIDER_RUN_MS){
	            return LSM6DS3_FALSE_ALARM;
	        }

	        if(motionless_detected && motionless_from_gyro_detected){
	            break;
	        }
	    }

	    if(motionless_detected && motionless_from_gyro_detected){
	    	break;
	    }
	    vTaskDelay(10 / portTICK_PERIOD_MS);
	}

	if(motionless_detected){
	    if(motionless_from_gyro_detected){
	        if(impact_from_gyro_detected){
	            return LSM6DS3_IMPACT_ORDER2_AND_MOTIONLESS_ORDER2_CONFIRMED;
	        } else {
	            return LSM6DS3_IMPACT_ORDER1_AND_MOTIONLESS_ORDER2_CONFIRMED;
	        }
	    } else {
	        if(impact_from_gyro_detected){
	            return LSM6DS3_IMPACT_ORDER2_AND_MOTIONLESS_ORDER1_CONFIRMED;
	        } else {
	            return LSM6DS3_IMPACT_ORDER1_AND_MOTIONLESS_ORDER1_CONFIRMED;
	        }
	    }

	} else {
	    if(impact_from_gyro_detected){
	        return LSM6DS3_IMPACT_ORDER2_CONFIRMED;
	    } else {
	        return LSM6DS3_IMPACT_ORDER1_CONFIRMED;
	    }
	}

	return LSM6DS3_FAIL;
}

lsm6ds3_err_t lsm6ds3_init(){
	lsm6ds3_err_t ret;

	ret = lsm6ds3_set_acc_output_data_rate(0); //Disable acc in order to configure it
	if(ret){ return ret;}

	ret = lsm6ds3_gyroscope_data_rate(0); //Disable gyro in order to configure it
	if(ret){ return ret;}

	//ACC CONFIGURATION
	ret = lsm6ds3_set_acc_full_scale(acc_full_scale);
	if(ret){ return ret;}

	ret = lsm6ds3_acc_bw_selection(true); //ACC BW is selected using CTRL1_XL register
	if(ret){ return ret;}

	ret = lsm6ds3_enable_data_masking_until_correct_initialization(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_acc_high_performance_mode(true); //true is disabled
	if(ret){ return ret;}

	// ACC FILTERS CONFIGURATION //
	ret = lsm6ds3_enable_embedded_functions_and_acc_filters(true); //MANDATORY if only acc in low/normal power mode
	if(ret){ return ret;}

	ret = lsm6ds3_set_acc_filters_enable(true); //Both functions enables the acc filters. First also enables the embedded functions. Second controls wake-up source (1=HP filter, 0=slope filter)
	if(ret){ return ret;}

	ret = lsm6ds3_acc_LPF2_selection_slope_or_hpf(false); //false = data directly to output. True, output filtering depends on lsm6ds3_acc_LPF2_selection(false);
	if(ret){ return ret;}

	ret = lsm6ds3_acc_LPF2_set_slope_and_cuttoff_freq(hp_acc_cuttoff_freq); //configures slope and HPF filters
	if(ret){ return ret;}

	ret = lsm6ds3_acc_LPF2_selection(false); //true = LPF, false = slope or HPF
	if(ret){ return ret;}

	ret = lsm6ds3_acc_LPF_on_6D6(true);
	if(ret){ return ret;}
	// END OF ACC FILTERS CONFIGURATION //
	//END OF ACC CONFIGURATION

	//GYRO CONFIGURATION
	ret = lsm6ds3_gyroscope_fullscale_selection(gyro_full_scale);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyro_sleep_mode(false); //no activate sleep mode. sleep mode =/= power-down.
	//From power-off to sleep/low power modes = 80 ms. From sleep-mode to low-power = 38.46 ms.
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyroscope_data_edge_sensitive_trigger(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyroscope_data_level_sensitive_trigger(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyroscope_data_latched_sensitive_trigger(false); //This function and the two above allows to configure gyro data adquisition INT2 dependant
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyroscope_high_performance_mode(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyro_X_output_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyro_Y_output_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyro_Z_output_enable(true);
	if(ret){ return ret;}

	// GYRO FILTERS CONFIGURATION //
	ret = lsm6ds3_enable_gyroscope_hp_filter(false); //Only valid in High-Performance mode
	if(ret){ return ret;}
	ret = lsm6ds3_set_gyroscope_hp_cutoff_frequency(hp_gyro_cuttoff_freq); //Only valid in High-Performance mode
	if(ret){ return ret;}
	// END OF GYRO FILTERS CONFIGURATION //
	//END OF GYRO CONFIGURATION

	//GENERAL CONFIGURATION
	ret = lsm6ds3_set_block_data_update(1); //don't udpate LSB or MSB registers until the read is complete (avoid read bytes from different samples)
	if(ret){ return ret;}

	ret = lsm6ds3_set_burst_mode_rounding(3); //Circular reading of the FIFO data of acc and gyro
	if(ret){ return ret;}

	ret = lsm6ds3_set_user_orientation(0);//[7:6] = 0, [5:3]=Sign(X;Y;Z), [2:0] set which axis is yaw, pitch and roll. Look table
	if(ret){ return ret;}

	//INTERRUPTIONS CONFIGURATION
	ret = lsm6ds3_set_interuption_activation_level(1); //Interruption low-level activated
	if(ret){ return ret;}

	ret = lsm6ds3_set_latched_interruption_enable(true); //Interruption pin register must be read to clear the interruption
	if(ret){ return ret;}

	ret = lsm6ds3_set_step_interruption(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_significant_motion_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_full_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_overrun_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_threshold_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_boot_status_available_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_gyroscope_data_ready_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_acc_data_ready_interruption_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_inactivity_mode_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_single_tap_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_wakeup_event_INT1(false); //Wakeup events: whenever the acc output from the axes exceeds a certain threshold during a certain time
	if(ret){ return ret;}

	ret = lsm6ds3_set_free_fall_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_double_tap_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_6D_event_INT1(false); //6D detects changes in all possible orientations
	if(ret){ return ret;}

	ret = lsm6ds3_set_tilt_event_INT1(false); //Detect changes in the position of the user (seat to stand-up and viceversa)
	if(ret){ return ret;}

	ret = lsm6ds3_set_end_of_counter_INT1(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_step_on_delta_time_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_step_counter_overflow_interruption_INT2(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_full_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_overrun_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_threshold_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_temperature_data_ready_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_gyroscope_data_ready_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_acc_data_ready_interruption_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_inactivity_mode_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_single_tap_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_wakeup_event_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_6D_event_INT2(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_tilt_event_INT2(false);
	if(ret){ return ret;}
	//END OF INTERRUPTIONS CONFIGURATION

	ret = lsm6ds3_set_free_fall_duration(free_fall_duration);
	if(ret){ return ret;}

	ret = lsm6ds3_set_free_fall_threshold(free_fall_threshold);
	if(ret){ return ret;}

	ret = lsm6ds3_set_free_fall_INT2(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_X_tap_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_Y_tap_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_Z_tap_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_tap_threshold(doubletap_threshold);
	if(ret){ return ret;}

	ret = lsm6ds3_set_doubletap_time_gap (doubletap_duration_time);
	if(ret){ return ret;}

	ret = lsm6ds3_set_doubletap_quiet_time (doubletap_quiet_time);
	if(ret){ return ret;}

	ret = lsm6ds3_set_doubletap_max_time_overthreshold_event(doubletap_shock_time);
	if(ret){ return ret;}

	ret = lsm6ds3_set_doubletap_wakeup_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_double_tap_INT2(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_pedometer_enable(true);
	if(ret){ return ret;}

	ret = lsm6ds3_reset_pedometer(true);
	if(ret){ return ret;}

	//FIFO CONFIGURATION
	lsm6ds3_disable_pedometer_and_step_counter_on_FIFO();
	if(ret){ return ret;}

	lsm6ds3_set_FIFO_basedon_XLGIRO_dataready();
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_acc_decimation(1);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_gyro_decimation(1); //No decimation
	if(ret){ return ret;}

	ret = lsm6ds3_FIFO_store_only_MSB(false);
	if(ret){ return ret;}

	ret = lsm6ds3_enable_FIFO_threshold(false);
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_pedo_decimation(0); //Dont store pedometer data in FIFO
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_operational_data_rate(3);// 52 Hz
	if(ret){ return ret;}

	ret = lsm6ds3_set_FIFO_mode(FIFO_mode); //Continuos mode
	if(ret){ return ret;}

	ret = lsm6ds3_set_autoincrement_register_address_when_burst(true); //Mandatory to use the FIFO
	if(ret){ return ret;}

	ret = lsm6ds3_set_acc_output_data_rate(3); //3 = 52 Hz = 45 uA; 7 = 833 Hz = 240 uA
	if(ret){ return ret;}

	ret = lsm6ds3_gyroscope_data_rate(3);
	if(ret){ return ret;}
	//END OF FIFO CONFIGURATION

	ret = lsm6ds3_enable_configuration(); //Give access to the embedded functions registers
	if(ret){ return ret;}

	ret = lsm6ds3_set_pedo_fullscale_4G(true);
	if(ret){ return ret;}

	ret = lsm6ds3_set_pedo_threshold(pedo_threshold);
	if(ret){ return ret;}

	ret = lsm6ds3_set_pedo_minimum_steps_to_increase_counter(pedo_minimum_steps); //default is 6
	if(ret){ return ret;}

	ret = lsm6ds3_set_pedo_debouncing_time(pedo_debouncing_time); //default is 13
	if(ret){ return ret;}

	lsm6ds3_disable_configuration();
	if(ret){ return ret;}

	return ret;
}
