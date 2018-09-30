#include "esp_system.h"
#include "driver/i2c.h"
#include "i2c_common_conf.h"
#include "kiss_fft.h"

#define DELAY_CMD_US            3

#define MAX30102_INT_ST1_REG    0x00    /* Register with interrupt status 1: A_FULL, PPG_RDY, ALC_OVF, PROX_INT, PWR_RDY */
#define MAX30102_INT_ST2_REG    0x01    /* Register with interrupt status 2: DIE_TEMP_RDY */
#define MAX30102_INT_EN1_REG    0x02    /* Register with interrupt enable 1: A_FULL, PPG_RDY, ALC_OVF, PROX_INT, PWR_RDY */
#define MAX30102_INT_EN2_REG    0x03    /* Register with interrupt enable 2: DIE_TEMP_RDY */

#define MAX30102_FIFO_WRP_REG   0x04    /* Register with FIFO write pointer  */
#define MAX30102_OVF_CNT_REG    0x05    /* Register with FIFO overflow counter */
#define MAX30102_FIFO_RDP_REG   0x06    /* Register with FIFO read pointer */
#define MAX30102_FIFO_DATA_REG  0x07    /* Register with FIFO currently data  */
#define MAX30102_FIFO_CONF_REG  0x08    /* Register with FIFO configuration: SMP_AVE, FIFO_ROLLOVER_EN, FIFO_A_FULL */

#define MAX30102_MODE_CONF_REG  0x09    /* Register with operation mode configuration: SHDN, RESET, MODE  */
#define MAX30102_SP02_CONF_REG  0x0A    /* Register with Sp02 configuration: SPO2_ADC_RGE. SPO2_SR, LED_PW  */
#define MAX30102_LD1_PA_REG     0x0C    /* Register with the pulse amplitude for the led 1 (RED) */
#define MAX30102_LD2_PA_REG     0x0D    /* Register with the pulse amplitude for the led 1 (IR) */
#define MAX30102_PILOT_PA_REG   0x10    /* Register with the proximity mode LED pulse amplitude */
#define MAX30102_MLED_SL21_REG  0x11    /* Register with the multi-LED pulse amplitude slots 2 and 1 */
#define MAX30102_MLED_SL43_REG  0x12    /* Register with the multi-LED pulse amplitude slots 4 and 3 */
#define MAX30102_TEMP_INT_REG   0x1F    /* Register with the integer part of the die temperature */
#define MAX30102_TEMP_FRA_REG   0x20    /* Register with the fractionary part of the die temperature  */
#define MAX30102_TEMP_CONF_REG  0x21    /* Register with the die temperature configuration: TEMP_EN  */
#define MAX30102_PROX_TH_REG    0x30    /* Register with the proximity interrupt threshold  */
#define MAX30102_REV_ID_REG     0xFE    /* Register with the product revision ID  */
#define MAX30102_PART_ID_REG    0xFF    /* Register with the product part ID */

#define MAX30102_WR_ADDR        0xAE   	/* Slave address for writing to MAX30205 sensor */
#define MAX30102_RD_ADDR        0xAF   	/* Slave address for reading from MAX30205 sensor */

/* Heart Rate related defines */
#define HR_DELETED_SAMPLES 2 //When starting a measure, at the beggining, some samples have very rare values. This number indicates how many of this samples will be throw away. 
#define FILTER_SAMPLES_FOR_LEARNING 200 // Filter should learn to obtain a better result. This parameter fixes the number of samples for learning (more samples, more comp. time, better result)
#define SECONDS_MEASURE_HR      10 		// Seconds of every heart rate measure
#define HR_ADC_TO_FIX_CURRENT   120000//155000 	// Value of the ADC which a measure is considered in good range to obtain a readable lecture
#define HR_ADC_TO_FIX_CURRENT_DIF 15000  // Tolerance of the previous value
#define HR_ADC_TO_FIX_CURRENT_UP_TH   (HR_ADC_TO_FIX_CURRENT+HR_ADC_TO_FIX_CURRENT_DIF) // Upper threshold
#define HR_ADC_TO_FIX_CURRENT_BT_TH (HR_ADC_TO_FIX_CURRENT-HR_ADC_TO_FIX_CURRENT_DIF)   // Bottom threshold
#define RED_LD_MINIMUM_PA_VALUE	20
#define MAXIMUM_SECONDS_OUT_OF_RANGE_TO_RESTART_MEASURE	 1 //THIS VALUE SHOULD BE MODIFIED IN FUNCTION OF THE SAMPLE RATE AND AVERAGING
#define MAX30102_18_BITS_ADC_SATURATION	262143 //IMPORTANT change in code the use of this in function of the actually ADC resolution
#define MAX30102_17_BITS_ADC_SATURATION	131071 //IMPROTANT choosing between this or the other one
#define PEAK_THRESHOLD		206 //Peaks over this threshold will be considered beats. This is the percentage wrt maximum value of the hr array of samples
#define SAMPLES_TO_CHECK_PERCENTAGE 3.75		//When HR in time domain, every this number we check if the signal is positive or negative sloping. Percentage wrt sample rate
#define SPACE_BETWEEN_PEAKS_PERCENTAGE 49	//Minimum space between peaks in percentage wrt sample rate
#define BPM_CORRECT_TH			49 //Minimum heart rate possible. Below this value, the lecture is considered incorrect
#define BPM_CORRECT_TH_UP		200 //Maximum heart rate possible.
#define MAX_DIFF_HR_TIME_FREQ	10 //Maximum difference between HR obtained by time method and frequency method. If the difference is higher that this value, only freq HR will be considered
#define TIMES_FOR_RETRY			2
/* SPO2 related defines */
#define SPO2_DELETED_SAMPLES 2 //When starting a measure, at the beggining, some samples have very rare values. This number indicates how many of this samples will be throw away.
#define SECONDS_MEASURE_SPO2    10 		// Seconds of every oxigen saturation measure
#define SPO2_ADC_TO_FIX_CURRENT   110000 //155000 	// Value of the ADC which a measure is considered in good range to obtain a readable lecture
#define SPO2_ADC_TO_FIX_CURRENT_DIF 15000  // Tolerance of the previous value
#define SPO2_ADC_TO_FIX_CURRENT_UP_TH   (SPO2_ADC_TO_FIX_CURRENT+SPO2_ADC_TO_FIX_CURRENT_DIF) // Upper threshold
#define SPO2_ADC_TO_FIX_CURRENT_BT_TH (SPO2_ADC_TO_FIX_CURRENT-SPO2_ADC_TO_FIX_CURRENT_DIF)   // Bottom threshold
#define IR_LD_MINIMUM_PA_VALUE	20
#define SPO2_THRESHOLD				85
#define SPO2_TEMP_THRESHOLD			30 //If temperature during measure is higher than this value, the ratio will be increased by 1%, to compensate RED LED wavelength deviations

typedef int32_t max30102_err_t;

/* Possible errors*/
#define MAX30102_OK					0
#define MAX30102_FAIL				1
#define MAX30102_HR_OUT_OF_RANGE	2
#define MAX30102_ENOUGH_SAMPLES		3
#define MAX30102_NO_SKIN_DETECTED	4
#define MAX30102_SPO2_OUT_OF_RANGE	5
#define MAX30102_SATURATED_OUTPUT	6
#define MAX30102_HR_NOT_PERFORMED	7
#define MAX30102_SPO2_NOT_PERFORMED	8
#define MAX30102_ERROR_ALLOC_MEM	9
#define MAX30102_SPO2_OK_HR_FAIL	10
#define MAX30102_SPO2_FAIL_HR_OK	11

esp_err_t max30102_write_reg (i2c_port_t i2c_num, uint8_t reg, uint8_t conf_value);

esp_err_t max30102_read_reg (i2c_port_t i2c_num, uint8_t reg, uint8_t *value);

esp_err_t max30102_read_intst1 (uint8_t *value);

esp_err_t max30102_read_intst2 (uint8_t *value);

esp_err_t max30102_read_inten1 (uint8_t *value);

esp_err_t max30102_write_inten1 (uint8_t value);

esp_err_t max30102_read_inten2 (uint8_t *value);

esp_err_t max30102_write_inten2 (uint8_t value);

esp_err_t max30102_read_fifo_wr_pointer (uint8_t *value);

esp_err_t max30102_write_fifo_wr_pointer (uint8_t value);

esp_err_t max30102_read_fifo_ovf_counter (uint8_t *value);

esp_err_t max30102_write_fifo_ovf_counter (uint8_t value);

esp_err_t max30102_read_fifo_rd_pointer (uint8_t *value);

esp_err_t max30102_write_fifo_rd_pointer (uint8_t value);

esp_err_t max30102_read_fifo_data (uint8_t *value);

esp_err_t max30102_write_fifo_data (uint8_t value);

esp_err_t max30102_read_fifo_conf (uint8_t *value);

esp_err_t max30102_write_fifo_conf (uint8_t value);

esp_err_t max30102_read_mode_conf (uint8_t *value);

esp_err_t max30102_write_mode_conf (uint8_t value);

esp_err_t max30102_read_sp02_conf (uint8_t *value);

esp_err_t max30102_write_sp02_conf (uint8_t value);

esp_err_t max30102_read_redld_pulse_amplitude (uint8_t *value);

esp_err_t max30102_write_redld_pulse_amplitude (uint8_t value);

esp_err_t max30102_read_irld_pulse_amplitude (uint8_t *value);

esp_err_t max30102_write_irld_pulse_amplitude (uint8_t value);

esp_err_t max30102_read_prox_mode_pulse_amplitude (uint8_t *value);

esp_err_t max30102_write_prox_mode_pulse_amplitude (uint8_t value);

esp_err_t max30102_read_multiled_slot2and1 (uint8_t *value);

esp_err_t max30102_write_multiled_slot2and1 (uint8_t value);

esp_err_t max30102_read_multiled_slot4and3 (uint8_t *value);

esp_err_t max30102_write_multiled_slot4and3 (uint8_t value);

esp_err_t max30102_read_die_temp_integer (uint8_t *value);

esp_err_t max30102_read_die_temp_fraction (uint8_t *value);

esp_err_t max30102_measure_temp ();

esp_err_t max30102_read_die_temp_conf (uint8_t *value);

esp_err_t max30102_read_proximity_interr_th (uint8_t *value);

esp_err_t max30102_write_proximity_interr_th (uint8_t value);

esp_err_t max30102_read_revision_id (uint8_t *value);

esp_err_t max30102_read_part_id (uint8_t *value);

esp_err_t max30102_enable_almost_full_int();

esp_err_t max30102_disable_almost_full_int();

esp_err_t max30102_enable_new_data_int();

esp_err_t max30102_disable_new_data_int();

esp_err_t max30102_enable_alc_overflow_int();

esp_err_t max30102_disable_alc_overflow_int();

esp_err_t max30102_enable_proximity_int();

esp_err_t max30102_disable_proximity_int();

esp_err_t max30102_enable_die_temp_conv_int();

esp_err_t max30102_disable_die_temp_conv_int();

esp_err_t max30102_change_sample_averaging(uint8_t samples);

esp_err_t max30102_enable_fifo_rollover();

esp_err_t max30102_disable_fifo_rollover();

esp_err_t max30102_change_FIFO_almost_full_value (uint8_t value);

esp_err_t max30102_power_safe_on();

esp_err_t max30102_power_safe_off();

esp_err_t max30102_reset();

esp_err_t max30102_change_mode(uint8_t mode);

void max30102_red_reset_current();

void max30102_ir_reset_current();

uint8_t max30102_get_mode();

esp_err_t max30102_change_spo2_adc_range(uint8_t range);

esp_err_t max30102_change_spo2_sample_rate(uint8_t sample_rate);

esp_err_t max30102_change_spo2_led_pulse_width(uint8_t pw);

esp_err_t max30102_multiled_change_slot1(uint8_t slot);

esp_err_t max30102_multiled_change_slot2(uint8_t slot);

esp_err_t max30102_multiled_change_slot3(uint8_t slot);

esp_err_t max30102_multiled_change_slot4(uint8_t slot);

uint16_t get_MAX30102_sample_rate(uint8_t output_format);

uint8_t get_MAX30102_sample_averaging(uint8_t output_format);

uint8_t max30102_calculateBeats(float *hr_array, uint16_t hr_array_size, float* hr_variability);

esp_err_t max30102_init();

max30102_err_t max30102_multiple_fifo_read(kiss_fft_cpx *array_to_store, kiss_fft_cpx *array_to_store_spo2, uint16_t* array_position, uint16_t* array_position_spo2, uint8_t* samples_added, uint8_t* samples_added_spo2, uint16_t limit_of_samples, bool calibrating);

max30102_err_t max30102_set_current_when_starting_measure();

max30102_err_t max30102_IR_set_current_when_starting_measure();

void max30102_equalize_currents();
