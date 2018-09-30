#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"

#include "driver/i2c.h"

#include "MAX30102.h"
#include "kiss_fft.h"

#define MAX_TAG_RED			"MAX_RED"
#define MAX_TAG_IR          "MAX_IR"
/* VALUES FOR SETTING DURING STARTUP. UPDATED LATER IN EXECUTION TIME*/
static uint8_t fifo_almost_full_value = 10;      /* 0=0 empty samples; 1=1 empty sample; ...; 15=15 empty samples */
static uint8_t sample_averaging = 2;            /* 0=No averaging; 1=2 samples; 2=4 samples; ...; 5=32 samples */
static uint8_t spo2_adc_range = 2;              /* 0= more resolution, less range; ...; 3=less resolution, more range */
static uint8_t spo2_sample_rate = 1;            /* 0=50; 1=100; 2=200; 3=400; 4=800; 5=1000; 6=1600; 7=3200 */
static uint8_t spo2_led_pulse_width = 3;        /* 0=15 bits ADC resolution; 1=16 bits; 2=17 bits; 3=18 bits */
static RTC_DATA_ATTR uint8_t red_ld_pulse_amplitude;    /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps */
static RTC_DATA_ATTR uint8_t ir_ld_pulse_amplitude;     /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps */
static RTC_DATA_ATTR uint8_t prox_md_pulse_amplitude; //200   /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps. Detection usually starts at 180-200 */
static RTC_DATA_ATTR uint8_t prox_int_th;  //180          /* 8 MSBs of ADC */
uint8_t operation_mode = 0;						/* HR = 2; SPO2 = 3; Multi-mode LED = 7; */

static uint8_t red_ld_pulse_amplitude_at_beginning = 100;    /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps */
static uint8_t ir_ld_pulse_amplitude_at_beginning = 100;     /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps */
static uint8_t prox_md_pulse_amplitude_at_beginning = 0; //200   /* Current of LED from 0=0mA to 255=50mA in approx. 0.2mA steps. Detection usually starts at 180-200 */
static uint8_t prox_int_th_at_beginning = 0;  //180          /* 8 MSBs of ADC */

static uint8_t multi_led_slot_1 = 0;
static uint8_t multi_led_slot_2 = 0;
static uint8_t multi_led_slot_3 = 0;
static uint8_t multi_led_slot_4 = 0;

static uint8_t wrp_ptr = 0;
static uint8_t rd_ptr = 0;

static uint32_t ld_values[32];
static uint32_t ir_values[32];
static uint8_t  read_samples[96];
static uint16_t samples_out_of_range = 0;
static uint16_t samples_out_of_range_spo2 = 0;

max30102_err_t return_current_function;

static uint8_t* out_of_range = "ERR.Value is out of range= ";

esp_err_t max30102_check_received (uint8_t reg, uint8_t sent_cmd);

esp_err_t max30102_write_reg (i2c_port_t i2c_num, uint8_t reg, uint8_t conf_value){
	xSemaphoreTake(print_mux, portMAX_DELAY);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_WR_ADDR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, conf_value, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);
    if (ret) {return ret;}
	
	ets_delay_us(DELAY_CMD_US);
	//ret = max30102_check_received(reg, conf_value);

	if (ret) {
        printf("Data was not correctly received\n");
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_read_reg (i2c_port_t i2c_num, uint8_t reg, uint8_t *value){
	int ret;

	xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_WR_ADDR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_RD_ADDR, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, value, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    if (ret) {return ret;}
    return ESP_OK;
}

esp_err_t check_input_value(uint8_t value, uint8_t upp_th, uint8_t bttm_th){
    if(value>upp_th || value<bttm_th){ 
        printf("%s %d\n", out_of_range, value);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t max30102_read_intst1 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_INT_ST1_REG, value);
}

esp_err_t max30102_read_intst2 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_INT_ST2_REG, value);
}

esp_err_t max30102_read_inten1 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, value);
}

esp_err_t max30102_write_inten1 (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, value);
}

esp_err_t max30102_read_inten2 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, value);
}

esp_err_t max30102_write_inten2 (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, value);
}

esp_err_t max30102_read_fifo_wr_pointer (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_FIFO_WRP_REG, value);
}

esp_err_t max30102_write_fifo_wr_pointer (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_FIFO_WRP_REG, value);
}

esp_err_t max30102_read_fifo_ovf_counter (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_OVF_CNT_REG, value);
}

esp_err_t max30102_write_fifo_ovf_counter (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_OVF_CNT_REG, value);
}

esp_err_t max30102_read_fifo_rd_pointer (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_FIFO_RDP_REG, value);
}

esp_err_t max30102_write_fifo_rd_pointer (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_FIFO_RDP_REG, value);
}

esp_err_t max30102_read_fifo_data (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_FIFO_DATA_REG, value);
}

esp_err_t max30102_write_fifo_data (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_FIFO_DATA_REG, value);
}

esp_err_t max30102_read_fifo_conf (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, value);
}

esp_err_t max30102_write_fifo_conf (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, value);
}

esp_err_t max30102_read_mode_conf (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, value);
}

esp_err_t max30102_write_mode_conf (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, value);
}

esp_err_t max30102_read_sp02_conf (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, value);
}

esp_err_t max30102_write_sp02_conf (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, value);
}

esp_err_t max30102_read_redld_pulse_amplitude (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_LD1_PA_REG, value);
}

esp_err_t max30102_write_redld_pulse_amplitude (uint8_t value){
    int ret = max30102_write_reg (I2C_MASTER_NUM0, MAX30102_LD1_PA_REG, value);
    if(!ret){ red_ld_pulse_amplitude = value;}
	return ret;
}

esp_err_t max30102_read_irld_pulse_amplitude (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_LD2_PA_REG, value);
}

esp_err_t max30102_write_irld_pulse_amplitude (uint8_t value){
    int ret = max30102_write_reg (I2C_MASTER_NUM0, MAX30102_LD2_PA_REG, value);
    if(!ret){ ir_ld_pulse_amplitude = value; }
	return ret;
}

esp_err_t max30102_read_prox_mode_pulse_amplitude (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_PILOT_PA_REG, value);
}

esp_err_t max30102_write_prox_mode_pulse_amplitude (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_PILOT_PA_REG, value);
}

esp_err_t max30102_read_multiled_slot2and1 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, value);
}

esp_err_t max30102_write_multiled_slot2and1 (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, value);
}

esp_err_t max30102_read_multiled_slot4and3 (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, value);
}

esp_err_t max30102_write_multiled_slot4and3 (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, value);
}

esp_err_t max30102_read_die_temp_integer (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_TEMP_INT_REG, value);
}

esp_err_t max30102_read_die_temp_fraction (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_TEMP_FRA_REG, value);
}

esp_err_t max30102_measure_temp (){
    return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_TEMP_CONF_REG, 0x01);
}

esp_err_t max30102_read_die_temp_conf (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_TEMP_CONF_REG, value);
}

esp_err_t max30102_read_proximity_interr_th (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_PROX_TH_REG, value);
}

esp_err_t max30102_write_proximity_interr_th (uint8_t value){
	return max30102_write_reg (I2C_MASTER_NUM0, MAX30102_PROX_TH_REG, value);
}

esp_err_t max30102_read_revision_id (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_REV_ID_REG, value);
}

esp_err_t max30102_read_part_id (uint8_t *value){
	return max30102_read_reg (I2C_MASTER_NUM0, MAX30102_PART_ID_REG, value);
}

esp_err_t max30102_check_received (uint8_t reg, uint8_t sent_cmd){
	uint8_t conf_rcvd;

	int ret = max30102_read_reg (I2C_MASTER_NUM0, reg, &conf_rcvd);
    if (ret ) {
        return ret;
    }

	if(sent_cmd == conf_rcvd){
        printf("Change on MAX 30102 correctly processed.\n");
    	return ESP_OK;
    } else {
    	printf("Conf. register received (%d) mismatch with the sent register(%d). Something goes wrong...\n", conf_rcvd, sent_cmd);
    }
    return ret;
}

esp_err_t max30102_enable_almost_full_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }

    new_reg = old_reg | 128;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_almost_full_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 127;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_enable_new_data_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 64;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_new_data_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 191;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_enable_alc_overflow_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 32;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_alc_overflow_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_ST1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 223;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_enable_proximity_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 16;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_proximity_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 239;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN1_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_enable_die_temp_conv_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 2;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_die_temp_conv_int(){
	uint8_t old_reg;
    uint8_t new_reg;
    int ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 253;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_INT_EN2_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_change_sample_averaging(uint8_t samples){ //from 000 to 111
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    int ret = check_input_value(samples, 7, 0);
    if (ret) {return ret;}

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg << 3;
    temp_reg = temp_reg >> 3;
    new_reg = (samples<<5) | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, new_reg);
    if (ret) {return ret;}
    sample_averaging = samples;
    return ESP_OK;
}

esp_err_t max30102_enable_fifo_rollover(){
	uint8_t old_reg;
    uint8_t new_reg;
    uint8_t ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 16;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_disable_fifo_rollover(){
	uint8_t old_reg;
    uint8_t new_reg;
    uint8_t ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 239;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_change_FIFO_almost_full_value (uint8_t value){ //from 0 to F
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(value, 15, 0);
    if (ret) {return ret;}

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    temp_reg = old_reg >> 4;
    temp_reg = temp_reg << 4;
    new_reg = value | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    fifo_almost_full_value = value;
    return ESP_OK;
}

esp_err_t max30102_power_safe_on(){
	uint8_t old_reg;
    uint8_t new_reg;
    uint8_t ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg | 128;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_power_safe_off(){
	uint8_t old_reg;
    uint8_t new_reg;
    uint8_t ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, &old_reg);
    if (ret ) {
        return ret;
    }
    
    new_reg = old_reg & 127;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_reset(){
	uint8_t new_reg = 64;
	uint8_t ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, new_reg);
    if (ret ) {
        return ret;
    }
    return ESP_OK;
}

esp_err_t max30102_change_mode(uint8_t mode){ //from 000 to 111
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    if(mode != 2 && mode != 3 && mode != 7){
        printf("%s\n", out_of_range);
        return 1;
    }
    
    uint8_t ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_WRP_REG, 0x00);
    if (ret ) { return ret;}
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_OVF_CNT_REG, 0x00);
    if (ret ) { return ret;}
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_FIFO_RDP_REG, 0x00);
    if (ret ) { return ret;}
    
    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, &old_reg);
    if (ret ) { return ret;}
    old_reg = old_reg<<1;
    old_reg = old_reg>>1;
    temp_reg = old_reg >> 3;
    temp_reg = temp_reg << 3;
    new_reg = mode | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MODE_CONF_REG, new_reg);
    if (ret ) { return ret;}

    operation_mode = mode;

    return ESP_OK;
}

uint8_t max30102_get_mode(){
	return operation_mode;
}

esp_err_t max30102_change_spo2_adc_range(uint8_t range){ //from 00 to 11
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(range, 3, 0);
    if (ret) {return ret;}

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, &old_reg);
    if (ret ) {return ret;}
    temp_reg = old_reg << 3;
    temp_reg = temp_reg >> 3;
    new_reg = (range<<5) | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, new_reg);
    if (ret) {return ret;}
    spo2_adc_range = range;
    return ESP_OK;
}

esp_err_t max30102_change_spo2_sample_rate(uint8_t sample_rate){ //from 000 to 111

	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(sample_rate, 7, 0);
    if (ret) {return ret;}

    if(operation_mode == 2){
        if((spo2_led_pulse_width == 3) && (sample_rate>5)){return 1;}
        if((spo2_led_pulse_width == 2) && (sample_rate>6)){return 1;}
        if((spo2_led_pulse_width == 1) && (sample_rate>6)){return 1;}
    }

    else if(operation_mode == 3){
        if((spo2_led_pulse_width == 3) && (sample_rate>3)){return 1;}
        if((spo2_led_pulse_width == 2) && (sample_rate>4)){return 1;}
        if((spo2_led_pulse_width == 1) && (sample_rate>5)){return 1;}
        if((spo2_led_pulse_width == 0) && (sample_rate>6)){return 1;}
    }

    else if(operation_mode == 7){
        if((spo2_led_pulse_width == 3) && (sample_rate>5)){return 1;}
        if((spo2_led_pulse_width == 2) && (sample_rate>6)){return 1;}
        if((spo2_led_pulse_width == 1) && (sample_rate>6)){return 1;}
    }

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, &old_reg);
    if (ret ) {return ret;}

    temp_reg = old_reg & 227;
    new_reg = (sample_rate<<2) | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, new_reg);
    if (ret) {return ret;}
    spo2_sample_rate = sample_rate;
    return ESP_OK;
}

esp_err_t max30102_change_spo2_led_pulse_width(uint8_t pw){ //from 00 to 11

	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(pw, 3, 0);
    if (ret) {return ret;}

    if(operation_mode == 2){
        if((spo2_sample_rate == 7) && (pw>0)){return 1;}
        if((spo2_sample_rate == 6) && (pw>2)){return 1;}
    }

    else if(operation_mode == 3){
        if((spo2_sample_rate == 6) && (pw>0)){return 1;}
        if((spo2_sample_rate == 5) && (pw>1)){return 1;}
        if((spo2_sample_rate == 4) && (pw>2)){return 1;}
    }

    else if(operation_mode == 7){
        if((spo2_sample_rate == 7) && (pw>0)){return 1;}
        if((spo2_sample_rate == 6) && (pw>2)){return 1;}
    }

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg >> 2;
    temp_reg = temp_reg << 2;
    new_reg = pw | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_SP02_CONF_REG, new_reg);
    if (ret) {return ret;}
    spo2_led_pulse_width = pw;
    return ESP_OK;
}

void max30102_red_reset_current(){
	red_ld_pulse_amplitude = red_ld_pulse_amplitude_at_beginning;
}

void max30102_ir_reset_current(){
	ir_ld_pulse_amplitude = ir_ld_pulse_amplitude_at_beginning;
}

uint8_t max30102_get_led_pulse_width(){
	return spo2_led_pulse_width;
}

esp_err_t max30102_multiled_change_slot1(uint8_t slot){ //from 000 to 110
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(slot, 6, 0);
    if (ret) {return ret;}
    if((multi_led_slot_2 != 0) || (multi_led_slot_2 != 3) || (multi_led_slot_2 != 4) ||
        (multi_led_slot_3 != 0) || (multi_led_slot_3 != 3) || (multi_led_slot_3 != 4) ||
        (multi_led_slot_4 != 0) || (multi_led_slot_4 != 3) || (multi_led_slot_4 != 4)){
        printf("%s\n", out_of_range);
        return 1;
    }

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg >> 4;
    temp_reg = temp_reg << 4;
    new_reg = slot | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, new_reg);
    if (ret) {return ret;}
    multi_led_slot_1 = slot;
    return ESP_OK;
}

esp_err_t max30102_multiled_change_slot2(uint8_t slot){ //from 000 to 110
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(slot, 6, 0);
    if (ret) {return ret;}
    if((multi_led_slot_3 != 0) || (multi_led_slot_3 != 3) || (multi_led_slot_3 != 4) ||
        (multi_led_slot_4 != 0) || (multi_led_slot_4 != 3) || (multi_led_slot_4 != 4)){
        printf("%s\n", out_of_range);
        return 1;
    }

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg << 4;
    temp_reg = temp_reg >> 4;
    new_reg = (slot<<4) | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL21_REG, new_reg);
    if (ret) {return ret;}
    multi_led_slot_2 = slot;
    return ESP_OK;
}

esp_err_t max30102_multiled_change_slot3(uint8_t slot){ //from 000 to 110
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(slot, 6, 0);
    if (ret) {return ret;}
    if((multi_led_slot_4 != 0) || (multi_led_slot_4 != 3) || (multi_led_slot_4 != 4)){
        printf("%s\n", out_of_range);
        return 1;
    }

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg >> 4;
    temp_reg = temp_reg << 4;
    new_reg = slot | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, new_reg);
    if (ret) {return ret;}
    multi_led_slot_3 = slot;
    return ESP_OK;
}

esp_err_t max30102_multiled_change_slot4(uint8_t slot){ //from 000 to 110
	uint8_t old_reg;
	uint8_t temp_reg;
    uint8_t new_reg;
    uint8_t ret = check_input_value(slot, 6, 0);
    if (ret) {return ret;}

    ret = max30102_read_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, &old_reg);
    if (ret) {return ret;}
    temp_reg = old_reg << 4;
    temp_reg = temp_reg >> 4;
    new_reg = (slot<<4) | temp_reg;
    ret = max30102_write_reg(I2C_MASTER_NUM0, MAX30102_MLED_SL43_REG, new_reg);
    if (ret) {return ret;}
    multi_led_slot_4 = slot;
    return ESP_OK;
}

uint16_t get_MAX30102_sample_rate(uint8_t output_format){
    if(output_format){
        return spo2_sample_rate;
    } else{
        switch (spo2_sample_rate){
            case 0:
                return 50;
            case 1:
                return 100;
            case 2:
                return 200;
            case 3:
                return 400;
            case 4:
                return 800;
            case 5:
                return 1000;
            case 6:
                return 1600;
            case 7:
                return 3200;
            default:
                return 0;
        }
    }
    return 0;
}

uint8_t get_MAX30102_sample_averaging(uint8_t output_format){
    if(output_format){
        return sample_averaging;
    } else{
        switch (sample_averaging){
            case 0:
                return 1;
            case 1:
                return 2;
            case 2:
                return 4;
            case 3:
                return 8;
            case 4:
                return 16;
            case 5:
                return 32;
            default:
                return 255;
        }
    }
    return 255;
}

uint8_t max30102_calculateBeats(float *hr_array, uint16_t hr_array_size, float* hr_variability){
	float* first_element = hr_array;
	bool increasing = false;
	bool decreasing = false;
	uint16_t samples_counter = 1;
	uint8_t beats_per_minute = 0;
	float hr_variability_array_size = 0;
	uint16_t* peaks_indexes;
	float current_val = 0;
	float acc = 0;
	uint16_t temp_index;
	float good_distance_between_peaks;
	float real_variability;
	uint16_t samples_to_check = round(SAMPLES_TO_CHECK_PERCENTAGE*(get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0))/100);
	uint16_t space_between_peaks = round(SPACE_BETWEEN_PEAKS_PERCENTAGE*(get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0))/100);
	float seconds_of_measure = (float) hr_array_size/(get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0));
	hr_variability_array_size = (hr_array_size/(get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0))*4)+0.5; //4 max beats per second
	peaks_indexes = (uint16_t *)calloc(round(hr_variability_array_size), sizeof(uint16_t));

	float previous_val = *(first_element++);
	for(int i=0; i<hr_array_size-1; i++){
		current_val = *(first_element++);
		if(current_val >= previous_val){
			acc += (current_val-previous_val);
		} else {
			acc -= (previous_val-current_val);
		}
		samples_counter++;
		if(samples_counter >= samples_to_check){
			if(acc>0){
				increasing = true;
				decreasing = false;
			} else if(acc<0){
				decreasing = true;
				if(increasing == true){
					if(beats_per_minute != 0){
						if(peaks_indexes){
							temp_index = *(peaks_indexes);
							if((i-temp_index>=space_between_peaks)){
								peaks_indexes++;
								beats_per_minute++;
								*(peaks_indexes)= i-samples_to_check;
							}
						} else {
							beats_per_minute++;
						}
					} else {
						beats_per_minute++;
						if(peaks_indexes){
							*(peaks_indexes)= i-samples_to_check;
						}
					}
				}
				increasing = false;
			}
			acc=0;
			samples_counter = 0;
		}
		previous_val = current_val;
	}

	good_distance_between_peaks = hr_array_size/beats_per_minute;
	real_variability = 0;
	if(peaks_indexes){
		peaks_indexes -= beats_per_minute-1;
		for(int i = 0; i<beats_per_minute-1; i++){
			current_val = *(++peaks_indexes)-*(--peaks_indexes);
			current_val -=good_distance_between_peaks;
			current_val = pow(current_val,2);
			current_val = sqrt(current_val);
			real_variability = real_variability + (current_val/(beats_per_minute-1));
			peaks_indexes++;
		}
	}
	peaks_indexes -= beats_per_minute-1;
	free(peaks_indexes);
	*(hr_variability) = real_variability/get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0)*1000;
	beats_per_minute = (uint8_t) round(beats_per_minute*60/seconds_of_measure);

	return beats_per_minute;
}

esp_err_t max30102_init(){
    esp_err_t ret;

    red_ld_pulse_amplitude = red_ld_pulse_amplitude_at_beginning;
    ir_ld_pulse_amplitude = ir_ld_pulse_amplitude_at_beginning;
    prox_md_pulse_amplitude = prox_md_pulse_amplitude_at_beginning;
    prox_int_th = prox_int_th_at_beginning;

    uint8_t int1_v = 0;
    max30102_read_intst1(&int1_v); //In order to clear the PWR_RDY interruption

    ret = max30102_enable_almost_full_int();
    if (ret) { return ret;}

    ret = max30102_change_FIFO_almost_full_value (fifo_almost_full_value); //Throw interrupt when there are 10 empty data samples in the FIFO
    if (ret) { return ret;}

    ret = max30102_disable_new_data_int();
    if (ret) { return ret;}

    ret = max30102_enable_alc_overflow_int();
    if (ret) { return ret;}

    ret = max30102_disable_proximity_int();
    if (ret) { return ret;}

    ret = max30102_enable_die_temp_conv_int();
    if (ret) { return ret;}

    ret = max30102_change_sample_averaging(sample_averaging); //4 averaging
    if (ret) { return ret;}
 
    ret = max30102_enable_fifo_rollover();
    if (ret) { return ret;}

    ret = max30102_change_spo2_adc_range(spo2_adc_range);
    if (ret) { return ret;}

    ret = max30102_change_spo2_sample_rate(spo2_sample_rate); //100 samples
    if (ret) { return ret;}

    ret = max30102_change_spo2_led_pulse_width(spo2_led_pulse_width); //18 bits ADC resolution
    if (ret) { return ret;}

    ret = max30102_write_redld_pulse_amplitude (red_ld_pulse_amplitude); //25.4 mA
    if (ret) { return ret;}

    ret = max30102_write_irld_pulse_amplitude (ir_ld_pulse_amplitude);
    if (ret) { return ret;}

    ret = max30102_write_prox_mode_pulse_amplitude (prox_md_pulse_amplitude); 
    if (ret) { return ret;}

    ret = max30102_write_proximity_interr_th (prox_int_th); //was 63 = 32767 pulses, around 5 mm of distance. MAYBE VERY OPTIMISTIC VALUE
    if (ret) { return ret;}

    ret = max30102_power_safe_on();
    if (ret) { return ret;}

    return ret;
}

max30102_err_t max30102_multiple_fifo_read(kiss_fft_cpx *array_to_store, kiss_fft_cpx *array_to_store_spo2, uint16_t* array_position, uint16_t* array_position_spo2, uint8_t* samples_added, uint8_t* samples_added_spo2, uint16_t limit_of_samples, bool calibrating){

    uint8_t ret;
    int samples = 0;
    int times = 0;
    int i = 0;
    int j = 0;
    uint8_t read_ptr = 0;

    uint8_t* failure = "Fail in burst read";
    *(samples_added) = 0;
    if(samples_added_spo2){
    	*(samples_added_spo2) = 0;
    }
    uint8_t next_measure = 0; // 0 = next time measuring will be stored in (RED) array; 1 = next measure in (IR);
    //First check. If we already have enough samples then do nothing
    if(operation_mode == 2){
    	if(*(array_position) >limit_of_samples){
    		return MAX30102_ENOUGH_SAMPLES;
    	}
    } else if (operation_mode == 3){
    	if(*(array_position) >limit_of_samples){
    		return MAX30102_ENOUGH_SAMPLES;
    	}
    	if(*(array_position_spo2) >limit_of_samples){
    		return MAX30102_ENOUGH_SAMPLES;
    	}
    }
    uint16_t maximum_samples_outofrange_to_restart_measure = MAXIMUM_SECONDS_OUT_OF_RANGE_TO_RESTART_MEASURE*get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0);

    /* GET WR POINTER*/
    xSemaphoreTake(print_mux, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_WR_ADDR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30102_FIFO_WRP_REG, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_RD_ADDR, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &wrp_ptr, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    if (ret){ printf("%s\n", failure);}
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);
    
    /* CALCULATE NUMBER OF SAMPLES TO READ*/
    times = wrp_ptr - rd_ptr;
    if(rd_ptr>wrp_ptr){
        times = wrp_ptr + (31 -rd_ptr);
    }
    if (times <= 0){return -1;}
    samples = times;
    times = times*3;

    /*MAKE TO KNOW THE DEVICE WHICH REGISTER WE WANT TO READ */
    xSemaphoreTake(print_mux, portMAX_DELAY);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_WR_ADDR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30102_FIFO_DATA_REG, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_RD_ADDR, ACK_CHECK_EN);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    if (ret){ printf("%s\n", failure);}
    i2c_cmd_link_delete(cmd);
    /* READ THAT NUMBER OF SAMPLES*/
    cmd = i2c_cmd_link_create();
    if(times == 3){
        i2c_master_read(cmd, &read_samples[0], 2, ACK_VAL);
        i2c_master_read(cmd, &read_samples[2], 1, NACK_VAL);
    } else {
        i2c_master_read(cmd, &read_samples[0], times-1 , ACK_VAL);
        i2c_master_read(cmd, &read_samples[times-1], 1, NACK_VAL);
    }
    ret = i2c_master_stop(cmd);

    //I2C OP 2
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    if (ret){ printf("%s\n", failure);}
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    /* JOIN THE READ SPLITTED UINT8_T SAMPLES IN UINT32_T SAMPLES*/
    j = 0;
    if(operation_mode == 2){ //HR mode
        for (i = 0; i < (times/3); i++){
            if(spo2_led_pulse_width == 3){
                read_samples[j] = read_samples[j] & 3;
            } else if (spo2_led_pulse_width == 2){
                read_samples[j] = read_samples[j] & 1;
            } else if (spo2_led_pulse_width == 1){
                read_samples[j] = read_samples[j] & 0;
            } else if (spo2_led_pulse_width == 0){
                read_samples[j] = read_samples[j] & 0;
                read_samples[j+1] = read_samples[j+1] & 127;
            }
            
            uint32_t temp_val = ((uint32_t)read_samples[j++] << 16) | ((uint16_t)read_samples[j++] <<8) | read_samples[j++];
            ld_values[i] = temp_val;

			if(ld_values[i] == MAX30102_17_BITS_ADC_SATURATION || ld_values [i]<(HR_ADC_TO_FIX_CURRENT_BT_TH)){
				samples_out_of_range++;
			}
			if(samples_out_of_range>=maximum_samples_outofrange_to_restart_measure){
				samples_out_of_range = 0;
				if(!calibrating){
					printf("Excess of samples out of range \n");
					return MAX30102_HR_OUT_OF_RANGE;
				}
			}
			if(*(array_position)<limit_of_samples){
				array_to_store[*(array_position)].r = (float) ld_values[i];
				*(array_position) +=1;
				*(samples_added) +=1;
			}
        }
        if( (*(array_position)>=limit_of_samples) ){
           return MAX30102_ENOUGH_SAMPLES;
        }

    } else if (operation_mode == 3){ //SPO2 mode
    	if(samples % 2 != 0){
    		samples--;
    	}
        for (i = 0; i < samples; i++){
            if(spo2_led_pulse_width == 3){
                read_samples[j] = read_samples[j] & 3;
            } else if (spo2_led_pulse_width == 2){
                read_samples[j] = read_samples[j] & 1;
            } else if (spo2_led_pulse_width == 1){
                read_samples[j] = read_samples[j] & 0;
            } else if (spo2_led_pulse_width == 0){
                read_samples[j] = read_samples[j] & 0;
                read_samples[j+1] = read_samples[j+1] & 127;
            }
            uint32_t temp_val = ((uint32_t)read_samples[j++] << 16) | ((uint16_t)read_samples[j++] <<8) | read_samples[j++];
            if(next_measure == 0){
					ld_values[i] = temp_val;
					if(ld_values[i] == MAX30102_17_BITS_ADC_SATURATION || ld_values [i]<(HR_ADC_TO_FIX_CURRENT_BT_TH)){
						samples_out_of_range++;
					}
					if(samples_out_of_range>=maximum_samples_outofrange_to_restart_measure){
						samples_out_of_range = 0;
						samples_out_of_range_spo2 = 0;
						if(!calibrating){
							printf("Excess of samples out of range \n");
							return MAX30102_HR_OUT_OF_RANGE;
						}
					}
					if(*(array_position)<limit_of_samples){
						array_to_store[*(array_position)].r = (float) ld_values[i];
						*(array_position) +=1;
						*(samples_added) +=1;
					}
					next_measure = 1;
            } else {
				ir_values[i] = temp_val;
				if(ir_values[i] == MAX30102_17_BITS_ADC_SATURATION || ir_values [i]<(SPO2_ADC_TO_FIX_CURRENT_BT_TH)){
					samples_out_of_range_spo2++;
				}
				if(samples_out_of_range>=maximum_samples_outofrange_to_restart_measure){
					samples_out_of_range_spo2 = 0;
					samples_out_of_range = 0;
					if(!calibrating){
						printf("Excess of samples out of range \n");
						return MAX30102_SPO2_OUT_OF_RANGE;
					}
				}
				if(*(array_position_spo2)<limit_of_samples){
					array_to_store_spo2[*(array_position_spo2)].r = (float) ir_values[i];
					*(array_position_spo2) +=1;
					*(samples_added_spo2) +=1;
				}
				next_measure = 0;
			}
        }
        if( (*(array_position_spo2)>=limit_of_samples) && (*(array_position)>=limit_of_samples) ){
        	return MAX30102_ENOUGH_SAMPLES;
        }
    }
    if(calibrating){
    	next_measure = 0;
    	samples_out_of_range = 0;
    	samples_out_of_range_spo2 = 0;
    }

    /* GET RD POINTER */
    xSemaphoreTake(print_mux, portMAX_DELAY);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_WR_ADDR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MAX30102_FIFO_RDP_REG, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX30102_RD_ADDR, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &rd_ptr, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM0, cmd, 1000 / portTICK_RATE_MS);
    if (ret){ printf("%s\n", failure);}

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(print_mux);

    return MAX30102_OK;

}

max30102_err_t max30102_set_current_when_starting_measure(){
	uint16_t array_pos_counter = 0;
    uint16_t i = 0;
    float max_val = 0;
    uint8_t samples_added = 0;
    uint16_t array_size = (uint16_t) ((get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0))/4);

    kiss_fft_cpx* hr_red_input;
    hr_red_input = (kiss_fft_cpx *) calloc(array_size, sizeof(kiss_fft_cpx));

    uint32_t difference = 0;
    int new_value = 0;
    if(hr_red_input == NULL){
		printf("ERR allocating mem\n");
		return MAX30102_ERROR_ALLOC_MEM;
	} else {
		float current_val = 0;
		while( (red_ld_pulse_amplitude != 255) || (red_ld_pulse_amplitude !=RED_LD_MINIMUM_PA_VALUE)){
			while(array_pos_counter<array_size){
				max30102_multiple_fifo_read(hr_red_input, NULL, &array_pos_counter, NULL, &samples_added, NULL, array_size, true);
				vTaskDelay(5 / portTICK_PERIOD_MS);
			}
			for(i=0; i<array_size; i++){
				current_val = hr_red_input[i].r/array_size;
				max_val = max_val + current_val;
			}

			array_pos_counter = 0;
			if((max_val>HR_ADC_TO_FIX_CURRENT_BT_TH) && (max_val<HR_ADC_TO_FIX_CURRENT_UP_TH)){
				free(hr_red_input);
				return MAX30102_OK;
			} else {
				if (max_val<HR_ADC_TO_FIX_CURRENT_BT_TH){
					ESP_LOGI(MAX_TAG_RED, "Mean of %f . Increasing current.\n", max_val);
					if (red_ld_pulse_amplitude == 255){
						printf("Current already at maximum level\n");
						free(hr_red_input);
						red_ld_pulse_amplitude = 100;
						return MAX30102_NO_SKIN_DETECTED;
					} else{
						difference = HR_ADC_TO_FIX_CURRENT_BT_TH-max_val;
						if(difference>=30000 && difference<=40000){
							new_value = red_ld_pulse_amplitude+20;
						} else if(difference>40000){
							new_value = red_ld_pulse_amplitude+30;
						} else if (difference < 30000 && difference >= 20000){
							new_value = red_ld_pulse_amplitude+10;
						} else if (difference<20000){
						   new_value = red_ld_pulse_amplitude+5;
						}
						if(new_value>255){
							new_value = 255;
						}
						ESP_LOGI(MAX_TAG_RED, "New current value= %d\n", new_value);
						max30102_write_redld_pulse_amplitude(new_value);
					}
				} else if (max_val>HR_ADC_TO_FIX_CURRENT_UP_TH){
					ESP_LOGI(MAX_TAG_RED, "Mean of %f . Decreasing current.\n", max_val);
					if (red_ld_pulse_amplitude == RED_LD_MINIMUM_PA_VALUE){
						printf("Current already at minimum level\n");
						free(hr_red_input);
						return MAX30102_SATURATED_OUTPUT;
					} else{
						difference = max_val-HR_ADC_TO_FIX_CURRENT_UP_TH;
						if(difference>=30000 && difference<=40000){
							new_value = red_ld_pulse_amplitude-20;
						} else if(difference>40000){
							new_value = red_ld_pulse_amplitude-30;
						} else if (difference < 30000 && difference >= 20000){
							new_value = red_ld_pulse_amplitude-10;
						} else if (difference<20000){
						   new_value = red_ld_pulse_amplitude-5;
						}
						if(new_value<RED_LD_MINIMUM_PA_VALUE){
							new_value = RED_LD_MINIMUM_PA_VALUE;
						}
						ESP_LOGI(MAX_TAG_RED, "New current value= %d\n", new_value);
						max30102_write_redld_pulse_amplitude(new_value);
					}
				}
			}
			max_val = 0;
		}

    }

    return MAX30102_OK;
}

max30102_err_t max30102_IR_set_current_when_starting_measure(){
    uint16_t array_pos_counter = 0;
    uint16_t array_pos_counter_spo2 = 0;
    uint16_t i = 0;
    float max_val = 0;
    uint8_t samples_added = 0;
    uint8_t samples_added_spo2 = 0;
    max30102_err_t ret;
    uint16_t array_size = (uint16_t) (get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0)/4);

    kiss_fft_cpx* spo2_red_input;
    kiss_fft_cpx* spo2_ir_input;
    spo2_red_input = (kiss_fft_cpx *) calloc(array_size, sizeof(kiss_fft_cpx));

    uint32_t difference = 0;
    int new_value = 0;
    if(spo2_red_input == NULL){
		printf("ERR allocating mem\n");
		return_current_function = MAX30102_ERROR_ALLOC_MEM;
		return return_current_function;
	} else {
	    spo2_ir_input = (kiss_fft_cpx *) calloc(array_size, sizeof(kiss_fft_cpx));
		if(spo2_ir_input == NULL){
			printf("ERR allocating mem\n");
			free(spo2_red_input);
			return MAX30102_ERROR_ALLOC_MEM;
		} else {
			float current_val = 0;
			while( (ir_ld_pulse_amplitude != 255) || (ir_ld_pulse_amplitude !=IR_LD_MINIMUM_PA_VALUE)){

				while(array_pos_counter<array_size){
					max30102_multiple_fifo_read(spo2_red_input, spo2_ir_input, &array_pos_counter, &array_pos_counter_spo2, &samples_added, &samples_added_spo2, array_size, true);
					vTaskDelay(1 / portTICK_PERIOD_MS);
				}

				for(i=0; i<array_pos_counter_spo2; i++){
					current_val = spo2_ir_input[i].r/array_size;
					max_val = max_val + current_val;
				}
				array_pos_counter = 0;
				array_pos_counter_spo2 = 0;

				if((max_val>SPO2_ADC_TO_FIX_CURRENT_BT_TH) && (max_val<SPO2_ADC_TO_FIX_CURRENT_UP_TH)){
					free(spo2_red_input);
					free(spo2_ir_input);
					return_current_function = MAX30102_OK;
					return return_current_function;
				} else {
					if (max_val<SPO2_ADC_TO_FIX_CURRENT_BT_TH){
						ESP_LOGI(MAX_TAG_IR, "Mean of %f . Increasing current.\n", max_val);
						if (ir_ld_pulse_amplitude == 255){
							printf("Current already at maximum level\n");
							free(spo2_red_input);
							free(spo2_ir_input);
							ir_ld_pulse_amplitude = 100;
							return MAX30102_NO_SKIN_DETECTED;
						} else{
							difference = SPO2_ADC_TO_FIX_CURRENT_BT_TH-max_val;
							if(difference>=30000 && difference<=40000){
								new_value = ir_ld_pulse_amplitude+20;
							} else if(difference>40000){
								new_value = ir_ld_pulse_amplitude+30;
							} else if (difference < 30000 && difference >= 20000){
								new_value = ir_ld_pulse_amplitude+10;
							} else if (difference<20000){
							   new_value = ir_ld_pulse_amplitude+5;
							}
							if(new_value>255){
								new_value = 255;
							}
							ESP_LOGI(MAX_TAG_IR, "New current value= %d\n", new_value);
							max30102_write_irld_pulse_amplitude(new_value);
						}
					} else if (max_val>SPO2_ADC_TO_FIX_CURRENT_UP_TH){
						ESP_LOGI(MAX_TAG_IR, "Mean of %f . Decreasing current.\n", max_val);
						if (ir_ld_pulse_amplitude == IR_LD_MINIMUM_PA_VALUE){
							printf("Current already at minimum level\n");
							free(spo2_red_input);
							free(spo2_ir_input);
							return MAX30102_SATURATED_OUTPUT;
						} else{
							difference = max_val-SPO2_ADC_TO_FIX_CURRENT_UP_TH;
							if(difference>=30000 && difference<=40000){
								new_value = ir_ld_pulse_amplitude-20;
							} else if(difference>40000){
								new_value = ir_ld_pulse_amplitude-30;
							} else if (difference < 30000 && difference >= 20000){
								new_value = ir_ld_pulse_amplitude-10;
							} else if (difference<20000){
							   new_value = ir_ld_pulse_amplitude-5;
							}
							if(new_value<IR_LD_MINIMUM_PA_VALUE){
								new_value = IR_LD_MINIMUM_PA_VALUE;
							}
							ESP_LOGI(MAX_TAG_IR, "New current value= %d\n", new_value);
							max30102_write_irld_pulse_amplitude(new_value);
						}
					}
				}
				max_val = 0;
			}
		}
	}

    return MAX30102_OK;

}

void max30102_equalize_currents(){
	if(ir_ld_pulse_amplitude>red_ld_pulse_amplitude){
		red_ld_pulse_amplitude = ir_ld_pulse_amplitude;
	} else {
		ir_ld_pulse_amplitude = red_ld_pulse_amplitude;
	}
	ESP_LOGI("Equalize Current", "RED= %d IR = %d\n", red_ld_pulse_amplitude, ir_ld_pulse_amplitude);
}
