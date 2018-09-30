#include "esp_system.h"
#include "driver/i2c.h"
#include "i2c_common_conf.h"

#define DELAY_CMD_US            3

#define LSM6DS3_ADDR        0x6A   	/* Slave address for writing to MAX30205 sensor */

typedef int32_t lsm6ds3_err_t;

/* Possible errors*/
#define LSM6DS3_OK												0
#define LSM6DS3_IMPACT_ORDER1_CONFIRMED							1
#define LSM6DS3_IMPACT_ORDER2_CONFIRMED							2
#define LSM6DS3_IMPACT_ORDER1_AND_MOTIONLESS_ORDER1_CONFIRMED	3
#define LSM6DS3_IMPACT_ORDER1_AND_MOTIONLESS_ORDER2_CONFIRMED	4
#define LSM6DS3_IMPACT_ORDER2_AND_MOTIONLESS_ORDER1_CONFIRMED	5
#define LSM6DS3_IMPACT_ORDER2_AND_MOTIONLESS_ORDER2_CONFIRMED	6
#define LSM6DS3_FALL_NOTIFIED_BY_PATIENT						8
#define LSM6DS3_FALSE_ALARM										9
#define LSM6DS3_FAIL											10

/* LSM6DS3 REGISTERS */
#define FUNC_CFG_ACCESS				0x01
#define FIFO_CTRL1					0x06
#define FIFO_CTRL2					0x07
#define FIFO_CTRL3					0x08
#define FIFO_CTRL4					0x09
#define FIFO_CTRL5					0x0A
#define ORIENT_CFG_G				0x0B
#define INT1_CTRL					0x0D
#define INT2_CTRL					0x0E
#define CTRL1_XL					0x10
#define CTRL2_G						0x11
#define CTRL3_C						0x12
#define CTRL4_C						0x13
#define CTRL5_C						0x14
#define CTRL6_C						0x15
#define CTRL7_G						0x16
#define CTRL8_XL					0x17
#define CTRL9_XL					0x18
#define CTRL10_C					0x19
#define MASTER_CONFIG				0x1A //REGISTER NOT IMPLEMENTED (Master configuration when sensor hub)
#define WAKE_UP_SRC					0x1B
#define TAP_SRC						0x1C
#define D6D_SRC						0x1D
#define STATUS_REG					0x1E
#define OUT_TEMP_L					0x20
#define OUT_TEMP_H					0x21
#define OUTX_L_G					0x22
#define OUTX_H_G					0x23
#define OUTY_L_G					0x24
#define OUTY_H_G					0x25
#define OUTZ_L_G					0x26
#define OUTZ_H_G					0x27
#define OUTX_L_XL					0x28
#define OUTX_H_XL					0x29
#define OUTY_L_XL					0x2A
#define OUTY_H_XL					0x2B
#define OUTZ_L_XL					0x2C
#define OUTZ_H_XL					0x2D
#define SENSORHUB1_REG				0x2E //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB2_REG				0x2F //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB3_REG				0x30 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB4_REG				0x31 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB5_REG				0x32 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB6_REG				0x33 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB7_REG				0x34 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB8_REG				0x35 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB9_REG				0x36 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB10_REG				0x37 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB11_REG				0x38 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB12_REG				0x39 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define FIFO_STATUS1				0x3A
#define FIFO_STATUS2				0x3B
#define FIFO_STATUS3				0x3C
#define FIFO_STATUS4				0x3D
#define FIFO_DATA_OUT_L				0x3E
#define FIFO_DATA_OUT_H				0x3F
#define TIMESTAMP0_REG				0x40
#define TIMESTAMP1_REG				0x41
#define TIMESTAMP2_REG				0x42
#define STEP_TIMESTAMP_L			0x49
#define STEP_TIMESTAMP_H			0x4A
#define STEP_COUNTER_L				0x4B
#define STEP_COUNTER_H				0x4C
#define SENSORHUB13_REG				0x4D //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB14_REG				0x4E //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB15_REG				0x4F //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB16_REG				0x50 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB17_REG				0x51 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define SENSORHUB18_REG				0x52 //REGISTER NOT IMPLEMENTED (Sensorhub stuff)
#define FUNC_SRC					0x53
#define TAP_CFG						0x58
#define TAP_THS_6D					0x59
#define INT_DUR2					0x5A
#define WAKE_UP_THS					0x5B
#define WAKE_UP_DUR					0x5C
#define FREE_FALL					0x5D
#define MD1_CFG						0x5E
#define MD2_CFG						0x5E
#define OUT_MAG_RAW_X_L				0x66
#define OUT_MAG_RAW_X_H				0x66
#define OUT_MAG_RAW_Y_L				0x66
#define OUT_MAG_RAW_Y_H				0x66
#define OUT_MAG_RAW_Z_L				0x66
#define OUT_MAG_RAW_Z_H				0x66

/* LSM6DS3 EMBEDDED FUNCTIONS REGISTERS */
#define PEDO_THS_REG				0x0F
#define SM_THS						0x13
#define PEDO_DEB_REG				0x14
#define STEP_COUNT_DELTA			0x15

//DEFINES AND CONSTANT VALUES

#define TIME_TH_FOR_DETECT_IMPACT_MS		  800
#define TIME_TH_FOR_DETECT_ANOTHER_IMPACT_MS  800
#define	TIME_TH_FOR_DETECT_MOTIONLESS_MS	  1000
#define TIME_TH_FOR_CONSIDER_RUN_MS			  5000
#define	IMPACT_G_TH								7168 // 3.5 g = 3.5*32766/16
#define	MOTIONLESS_G_TH							3482 // 1.7 g = 1.7*32766/16
#define	GYRO_TH_IMPACT							65532 // 4000 dps = 32766*2 (2000 is the full scale)
#define GYRO_TH_MOTIONLESS						8192 //500 dps = 32766*500/2000
#define CONSECUTIVE_SAMPLES_CONFIRM_MOTIONLESS	30

lsm6ds3_err_t lsm6ds3_write_reg (uint8_t reg, uint8_t conf_value);

lsm6ds3_err_t lsm6ds3_read_reg (uint8_t reg, uint8_t *value);

lsm6ds3_err_t lsm6ds3_enable_configuration();

lsm6ds3_err_t lsm6ds3_disable_configuration();

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_FIFO_threshold(uint8_t *value);

lsm6ds3_err_t lsm6ds3_enable_pedometer_and_step_counter_on_FIFO();

lsm6ds3_err_t lsm6ds3_disable_pedometer_and_step_counter_on_FIFO();

lsm6ds3_err_t lsm6ds3_set_FIFO_basedon_XLGIRO_dataready();

lsm6ds3_err_t lsm6ds3_set_FIFO_basedon_step_detected();

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_on_CTRL2(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_FIFO_threshold_on_CTRL2(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_FIFO_gyro_decimation(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_FIFO_gyro_decimation(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_FIFO_acc_decimation(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_FIFO_acc_decimation(uint8_t *value);

lsm6ds3_err_t lsm6ds3_FIFO_store_only_MSB(bool do_or_not);

lsm6ds3_err_t lsm6ds3_set_FIFO_pedo_decimation(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_FIFO_pedo_decimation(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_FIFO_mode(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_FIFO_operational_data_rate(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_user_orientation(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_user_orientation(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_X_pitch(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_X_pitch(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_Y_roll(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_Y_roll(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_Z_yaw(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_Z_yam(uint8_t value);

lsm6ds3_err_t lsm6ds3_get_int1(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_step_interruption(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_significant_motion_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_full_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_overrun_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_boot_status_available_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_ready_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_acc_data_ready_interruption_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_get_int2(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_step_on_delta_time_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_step_counter_overflow_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_full_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_overrun_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_FIFO_threshold_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_temperature_data_ready_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_gyroscope_data_ready_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_acc_data_ready_interruption_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_output_data_rate(uint8_t data_rate);

lsm6ds3_err_t lsm6ds3_set_acc_full_scale(uint8_t scale);

lsm6ds3_err_t lsm6ds3_set_antialiasing_filter_bw(uint8_t bw);

lsm6ds3_err_t lsm6ds3_gyroscope_data_rate(uint8_t value);

lsm6ds3_err_t lsm6ds3_gyroscope_fullscale_selection(uint8_t value);

lsm6ds3_err_t lsm6ds3_gyroscope_fullscale_at_125dps(uint8_t value);

lsm6ds3_err_t lsm6ds3_reboot_memory_content(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_block_data_update(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_interuption_activation_level(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_push_or_opendrain_in_int_lines(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_autoincrement_register_address_when_burst(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_big_or_little_endian(uint8_t value);

lsm6ds3_err_t lsm6ds3_software_reset(uint8_t value);

lsm6ds3_err_t lsm6ds3_acc_bw_selection(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_gyro_sleep_mode(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_all_int_on_int1(uint8_t value);

lsm6ds3_err_t lsm6ds3_enable_temperature_in_FIFO(uint8_t value);

lsm6ds3_err_t lsm6ds3_enable_data_masking_until_correct_initialization(uint8_t value);

lsm6ds3_err_t lsm6ds3_disable_i2c(uint8_t value);

lsm6ds3_err_t lsm6ds3_enable_FIFO_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_burst_mode_rounding(uint8_t value);

lsm6ds3_err_t lsm6ds3_angular_self_test_enable(uint8_t value);

lsm6ds3_err_t lsm6ds3_acc_self_test_enable(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_edge_sensitive_trigger(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_level_sensitive_trigger(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_gyroscope_data_latched_sensitive_trigger(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_acc_high_performance_mode(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyroscope_high_performance_mode(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_enable_gyroscope_hp_filter(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_reset_gyroscope_hp_filter(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_enable_source_rounding(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyroscope_hp_cutoff_frequency(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_acc_Z_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_acc_Y_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_acc_X_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyro_Z_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyro_Y_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_gyro_X_output_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_enable_embedded_functions(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_reset_pedometer(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_significat_motion_function(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_get_wake_up_src(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_tap_src(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_d6d_src(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_temperature(uint16_t *temperature);

lsm6ds3_err_t lsm6ds3_get_pitch_X(int16_t *pitch);

lsm6ds3_err_t lsm6ds3_get_roll_Y(int16_t *roll);

lsm6ds3_err_t lsm6ds3_get_yaw_Z(int16_t *yaw);

lsm6ds3_err_t lsm6ds3_get_acc_X(int16_t *acc);

lsm6ds3_err_t lsm6ds3_get_acc_Y(int16_t *acc);

lsm6ds3_err_t lsm6ds3_get_acc_Z(int16_t *acc);

lsm6ds3_err_t lsm6ds3_get_FIFO_unread_samples(uint16_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_watermark_status(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_overrun_status(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_if_FIFO_is_full(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_if_FIFO_is_empty(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_pattern(uint16_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_data(uint16_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_timestamp(uint32_t *value);

lsm6ds3_err_t lsm6ds3_reset_FIFO_timestamp();

lsm6ds3_err_t lsm6ds3_get_FIFO_step_timestamp(uint16_t *value);

lsm6ds3_err_t lsm6ds3_get_FIFO_step_counter(uint16_t *value);

lsm6ds3_err_t lsm6ds3_get_func_src(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_step_recognition_on_deltatime(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_significant_motion_detection(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_tilt_detection(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_step_detection(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_step_overflow(uint8_t *value);

lsm6ds3_err_t lsm6ds3_get_hard_softiron_calculation_status(uint8_t *value);

lsm6ds3_err_t lsm6ds3_set_timestap_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_pedometer_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_tilt_calculation_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_acc_filters_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_X_tap_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_Y_tap_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_Z_tap_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_latched_interruption_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_4D_orientation_detection_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_D6D_function_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_tap_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_doubletap_time_gap(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_doubletap_quiet_time_(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_doubletap_max_time_overthreshold_event(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_doubletap_wakeup_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_inactivity_wakeup_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_tap_and_doubletap_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_free_fall_duration_enable(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_wake_up_duration_event(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_timestamp_register_resolution(bool ms_6_dot_4_or_us_25);

lsm6ds3_err_t lsm6ds3_set_duration_togo_in_sleep_mode(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_free_fall_duration(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_free_fall_threshold(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_inactivity_mode_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_single_tap_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_wakeup_event_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_free_fall_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_double_tap_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_6D_event_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_tilt_event_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_end_of_counter_INT1(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_inactivity_mode_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_single_tap_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_wakeup_event_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_free_fall_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_double_tap_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_6D_event_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_tilt_event_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_tilt_event_INT2(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_pedo_fullscale_4G(bool en_or_dis);

lsm6ds3_err_t lsm6ds3_set_pedo_threshold(uint8_t threshold);

lsm6ds3_err_t lsm6ds3_set_significant_motion_threshold(uint8_t threshold);

lsm6ds3_err_t lsm6ds3_set_pedo_debouncing_time(uint8_t time);

lsm6ds3_err_t lsm6ds3_set_pedo_minimum_steps_to_increase_counter(uint8_t value);

lsm6ds3_err_t lsm6ds3_set_step_count_delta(uint8_t delta_time);

double lsm6ds3_normalize_acc_read(int16_t value);

double lsm6ds3_normalize_gyro_read(int16_t value);

lsm6ds3_err_t lsm6ds3_run_falls_algorithm(void);

lsm6ds3_err_t lsm6ds3_init();
