/*************************************************************************
TODO:
--Implements BT security
--Add a sensor errors indicator (every time that a measure cannot be performed or the sensor can not be initiated increment a counter that is send in the BT array). Reset the counter with every measure
--Change temperature sensor with a contactless sensor
--Implement ANC (Active Noise Canceling) filters
--Implement another HR+SPO2 sensors with lower power consumption (if any).
--Make a study of the feasibility of include another measures such as blood pressure.
--Change the PMIC with another with lower power consumption (by now we are on 4 mA when idle).
--Improve the battery level percentage algorithm
--Perform long-term tests of the whole wearable
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_heap_caps.h"
#include <arpa/inet.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "gatts_tables.h"

#include "sdkconfig.h"

#include "driver/i2c.h"
#include "i2c_common_conf.h"
#include "MAX30205.h"
#include "MAX30102.h"
#include "LSM6DS3.h"
//#include "FIR_filter.h" //uncomment if FIR filter is desired to use
#include "IIR_filter.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include "esp_adc_cal.h"
#include "esp_sleep.h"
#include "driver/uart.h"

//Defines and variables for I2C
SemaphoreHandle_t print_mux = NULL;

//Defines and variables for ADC (battery level measurement)
#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64          //Multisampling
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;
static const adc_atten_t atten = ADC_ATTEN_DB_2_5;
static const adc_unit_t unit = ADC_UNIT_1;
#define ADC_SLOPE_1 0.1357
#define ADC_SLOPE_2	1
#define HUNDRED_PERCENT_BATTERY_MV	1260
#define TEN_PERCENT_BATTERY_MV 1080
#define ZERO_PERCENT_BATTERY_MV 1050
#define ADC_SHUTDOWN_LEVEL    5 //3.3 V
uint64_t check_battery_time_when_battery_is_low_us = 300000000L;
esp_adc_cal_characteristics_t characteristics;
#define V_REF 1100
static bs_service bs_srv1 = {false};

//Defines and variables for MAX30102
#define GPIO_14  14
#define GPIO_19  19
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_14) | (1ULL<<GPIO_19))
#define MAX30102_INT_PIN        36
#define LOWER_FREQ_FFT 		 0.9 //in HZ
#define UPPER_FREQ_FFT 		 5   //in HZ. Both values set the limits to search for the HR frequency peak
#define PO_STORED_SAMPLES     10	//Oxygen saturation size for non volatile array
#define HR_STORED_SAMPLES     10	//Heart rate size for non volatile array
#define MAX_TIME_MEASURING_MS 15000	//Maximum time trying to measure HR or SpO2. If after that time we don't have enough samples, discard measure
#define SECOND_FOR_MEASURE_MAX30102_TEMP 5 //Interval in seconds for getting temperature of the sensor when measuring SpO2
float measure_percentage_to_proccess_hr = 90; //Indicates how many values (in percentage) of the total one that should be measured is fine to detect
//peaks. For example, if we expect to have 200 values, if variable = 90, 180 will be enought. Used only in case that not all samples are taken
uint8_t fifo_wr_ptr = 0x00;
uint8_t fifo_ovf_cnt = 0x00;
uint8_t fifo_rd_ptr = 0x00;
uint8_t fifo_last_data = 0x00;
uint64_t hr_time = 11000000L;
uint64_t spo2_time = 10000000L;
uint8_t max30102_intr_detected = 0;
uint8_t int_vector1 = 0;
uint8_t int_vector2 = 0;
bool currently_measuring = false;
bool measure_finished_ok = false;
bool measure_finished_wrong = false;
static kiss_fft_cpx* hr_array = NULL; //This array will stores the RED led measures during HR lecture
float* first_hr_element; //This pointer starts pointing at hr_array and increments its value every time a FIFO interruption is detected
uint16_t hr_array_pos_counter = 0; //This variable counts how many RED LED samples we have
float *spo2_array = NULL; //This array will stores the IR LED measures during SPO2 lecture
float* first_spo2_element; //This pointer starts pointing at spo2_array and increments its value every time a FIFO interruption is detected
uint16_t spo2_array_pos_counter = 0; //This variable counts how many IR LED samples we have
float *filtered_signal = NULL; //This array stores the signal after apply to it a IIR filter
float* first_filtered_element; //This pointer pointing at filtered_signal assigned address in order to not losing this address when moving through the array
uint16_t hr_array_size = 0; //After set. Indicates how many samples we want to receive, in function of the sample rate, averaging and desired measuring time
uint16_t spo2_array_size = 0; //After set. Indicates how many samples we want to receive, in function of the sample rate, averaging and desired measuring time
uint8_t hr_samples_added = 0; //How many HR samples are really added.
uint8_t spo2_samples_added = 0; //How many sp02 samples are really added.
float* hr_array_spo2;
uint8_t number_temperature_samples = 0; //Stores the number of max30102 temperature samples after a read
uint8_t hr_retrying_attemps = 0; //Stores the number of HR measures retries
uint8_t beats_per_minute = 0;
float hr_variability = 0; //Result is in ms
float beats_per_minute_from_spo2 = 0;
struct timeval time_starting_measure;
uint8_t times_retrying = 0;
uint32_t hr_threshold = 0;
TaskHandle_t max30102_int_handle;
static kiss_fft_cpx* spo2_red_input;
static kiss_fft_cpx* spo2_ir_input;
static kiss_fft_cpx* spo2_red_fft;
static kiss_fft_cpx* spo2_ir_fft;
float saturation_percentage = 0;
float saturation_percentage_offset = 0; //To compensate the deviation caused by the temperature in the sensor
static RTC_DATA_ATTR struct timeval time_last_hr_measure;
static RTC_DATA_ATTR struct timeval time_last_po_measure;
kiss_fft_cfg mycfg;

//MAX30102 BT related//
static hr_service hr_srv1 = {false, false, false};
static po_service po_srv1 = {false, false, false, false};
static RTC_DATA_ATTR uint64_t time_since_last_hr_measure;
static RTC_DATA_ATTR uint64_t time_since_last_spo2_measure;
static RTC_DATA_ATTR uint8_t max30102_correctly_initialized;
static RTC_DATA_ATTR uint8_t last_po_measures[PO_STORED_SAMPLES];
static RTC_DATA_ATTR uint16_t last_po_measures_size;
static RTC_DATA_ATTR uint16_t last_po_measures_index;
static RTC_DATA_ATTR uint8_t last_hr_measures[HR_STORED_SAMPLES];
static RTC_DATA_ATTR uint16_t last_hr_measures_size;
static RTC_DATA_ATTR uint16_t last_hr_measures_index;
static RTC_DATA_ATTR float last_hr_variability_measures[HR_STORED_SAMPLES];
static RTC_DATA_ATTR uint16_t last_hr_variability_measures_size;
static RTC_DATA_ATTR uint16_t last_hr_variability_measures_index;

//Defines and variables for MAX30205
#define WARMUP_TIME_MS          300000
#define WARMUP_TEMP             36
#define HT_STORED_SAMPLES     	10	//Size of the array that stores the HT measures
uint64_t temp_time = 12000000L;
static ht_service ht_srv1 = {false, false, false};
static RTC_DATA_ATTR uint64_t time_since_last_ht_measure;
static RTC_DATA_ATTR uint8_t max30205_correctly_initialized;
static RTC_DATA_ATTR float last_ht_measures[HT_STORED_SAMPLES];
static RTC_DATA_ATTR uint16_t last_ht_measures_size;
static RTC_DATA_ATTR uint16_t last_ht_measures_index;
static RTC_DATA_ATTR struct timeval time_last_ht_measure;

//Defines and variables for LSM6DS3
#define LSM6DS3_INT2			35
#define ESP_INTR_FLAG_DEFAULT    0
uint8_t lsm6ds3_intr_detected = 0;
TaskHandle_t lsm6ds3_int_handle;
static RTC_DATA_ATTR uint8_t lsm6ds3_correctly_initialized;
static RTC_DATA_ATTR uint64_t lsm6ds3_accumulated_steps;
static uint8_t times_requested_falls = 0;
//Defines and variables for deep sleep
#define GPIO_MAX30102_INPUT_MASK  (1ULL<<MAX30102_INT_PIN)
#define GPIO_LSM6DS3_INPUT_MASK  	(1ULL<<LSM6DS3_INT2)
static RTC_DATA_ATTR struct timeval sleep_enter_time;
static RTC_DATA_ATTR int running;
bool fall_detected = false;

//Defines and variables for BT
#define GATTS_TAG                     "eSpMART105"
#define DEVICE_NAME                     "PATIENT1"
#define PROFILE_NUM                              1
#define PROFILE_A_APP_ID                         0
#define ESPMART105_APP_ID                     0x55
#define HEART_RATE_SVC_INST_ID                   0
#define HEALTH_THERMOMETER_SVC_INST_ID           1
#define PULSE_OXIMETER_SVC_INST_ID               2
#define BATTERY_SERVICE_SVC_INST_ID              3
#define STEPS_SVC_INST_ID             	 		 4
#define FALL_SVC_INST_ID        	     		 5

#define GATTS_CHAR_VAL_LEN_MAX                0x40

#define SERVICE_UUID_SIZE_BYTES                 96
#define CHAR_DECLARATION_SIZE    (sizeof(uint8_t))
#define BT_TIMEOUT_RECEPTION_TIME_MS         20000

struct timeval timeout_to_no_bt_packet_reception;

uint16_t heart_rate_handle_table[HRS_IDX_NB];
uint16_t health_thermometer_handle_table[HTS_IDX_NB];
uint16_t pulse_oximeter_handle_table[POS_IDX_NB];
uint16_t battery_service_handle_table[BS_IDX_NB];
uint16_t steps_handle_table[SS_IDX_NB];
uint16_t fall_handle_table[FS_IDX_NB];

static int number_of_services = 0; //Number of services created. Starts with 0 and should finish with the total amount of BT services
static int bt_correctly_init = 0;

static uint8_t service_uuid128[SERVICE_UUID_SIZE_BYTES] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x09, 0x18, 0x00, 0x00, //Health Thermometer
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0D, 0x18, 0x00, 0x00, //Heart Rate

    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x22, 0x18, 0x00, 0x00, //Pulse Oximeter Service

    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0F, 0x18, 0x00, 0x00, //Battery Service

	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0F, 0x18, 0x26, 0x00, //Steps Service

	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x11, 0x18, 0x00, 0x00, //Fall Service
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = SERVICE_UUID_SIZE_BYTES,
    .p_service_uuid = service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND, //Pheripheral request connect to any central
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
//static char *esp_key_type_to_str(esp_ble_key_type_t key_type);
static void convert_hex_measure_intervals_to_time_in_useconds(int measure_interval_type);
static uint64_t min(uint64_t a, uint64_t b);
static void save_mi_or_ccc_into_bt_array(uint8_t *value, int mi_type);
static uint32_t update_object_size_from_value_of_client(int measure_object_size, uint8_t *value);
static void save_temp_into_bt_array_from_float (float temperature);
static void save_hr_and_variability_into_bt_array(uint8_t beats, float variancee);
static void save_spo2_into_bt_array(uint8_t saturation_value);
static void save_hr_and_variability_in_RTC_and_increment_object_size(uint8_t beats, float variancee);
static void save_hr_and_variability_into_bt_array(uint8_t beats, float variancee);
static void save_spo2_in_RTC_and_increment_object_size(uint8_t saturation_value);
static void save_spo2_into_bt_array(uint8_t saturation_value);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
//static const uint16_t character_present_format_uuid = ESP_GATT_UUID_CHAR_PRESENT_FORMAT;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE|ESP_GATT_CHAR_PROP_BIT_READ;
//static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
//static const uint8_t char_prop_write_noresponse = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_indicate = ESP_GATT_CHAR_PROP_BIT_INDICATE;
//static const uint8_t char_prop_read_and_indicate = ESP_GATT_CHAR_PROP_BIT_INDICATE|ESP_GATT_CHAR_PROP_BIT_READ;

/// Heart Rate Sensor Service
static const uint16_t heart_rate_svc = ESP_GATT_UUID_HEART_RATE_SVC;

/// Heart Rate Sensor Service - Heart Rate Measurement Characteristic, notify. Descriptor: Client Characteristic Configuration
static const uint16_t hr_meas_uuid = ESP_GATT_HEART_RATE_MEAS;
static uint8_t hr_meas_val[2] = {0x00, 0x00}; // Hearth Rate Measurement(1), Flags(1) = 0x00
static uint8_t hr_meas_ccc[2] = {0x00, 0x00}; 

/// Heart Rate Sensor Service -Body Sensor Location Characteristic, read
static const uint16_t hr_body_sensor_loc_uuid = ESP_GATT_BODY_SENSOR_LOCATION;
static const uint8_t hr_body_sensor_loc_val[1] = {0x02};

/// Heart Rate Sensor Service - Heart Rate Measure Interval Characteristic, write&read
static const uint16_t hr_meas_int_uuid = 0x2A21;
static RTC_DATA_ATTR uint8_t hr_meas_int_val[2] = {0x3C, 0x00}; //First byte = LSB
static RTC_DATA_ATTR uint8_t hr_mi_1;
static RTC_DATA_ATTR uint8_t hr_mi_2;

/// Heart Rate Sensor Service - Heart Rate Object Size Characteristic, write&read
static const uint16_t hr_obj_size_uuid = 0x2AC0;
static uint8_t hr_obj_size_val[8] = {0x00, 0x00, 0x00, 0x00, ((HR_STORED_SAMPLES >> 24) & 0xFF), ((HR_STORED_SAMPLES >> 16) & 0xFF), ((HR_STORED_SAMPLES >> 8) & 0xFF), (HR_STORED_SAMPLES & 0xFF)};

/// Heart Rate Sensor Service - Heart Rate Variability Characteristic, write&read
static const uint16_t hr_variability_uuid = 0x2ADB;
static uint8_t hr_variability_val[4] = {0x00, 0x00, 0x00, 0x00};

/// Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t heart_rate_gatt_db[HRS_IDX_NB] = {
    // Heart Rate Service Declaration
    [HRS_IDX_SVC]                       =  
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(heart_rate_svc), (uint8_t *)&heart_rate_svc}},

    // Heart Rate Measurement Characteristic Declaration
    [HRS_IDX_HR_MEAS_CHAR]            = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_notify}},
      
    // Heart Rate Measurement Characteristic Value
    [HRS_IDX_HR_MEAS_VAL]               =   
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_meas_uuid, ESP_GATT_PERM_READ,
      2*sizeof(uint8_t), sizeof(hr_meas_val), (uint8_t *)&hr_meas_val}},

    // Heart Rate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HRS_IDX_HR_MEAS_CCC]       =    
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(hr_meas_ccc), (uint8_t *)hr_meas_ccc}},

    // Body Sensor Location Characteristic Declaration
    [HRS_IDX_BODY_SENSOR_LOC_CHAR]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // Body Sensor Location Characteristic Value
    [HRS_IDX_BODY_SENSOR_LOC_VAL]   =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_body_sensor_loc_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(hr_body_sensor_loc_val), (uint8_t *)hr_body_sensor_loc_val}},

    // Heart Rate Measure Interval Characteristic Declaration
    [HRS_IDX_HR_CTNL_PT_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Heart Rate Measure Interval Characteristic Value
    [HRS_IDX_HR_CTNL_PT_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_meas_int_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      2*sizeof(uint8_t), sizeof(hr_meas_int_val), (uint8_t *)hr_meas_int_val}},  

	// Heart Rate Variability Characteristic Declaration
	[HRS_IDX_HR_VARIABILITY_CHAR]          =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
	  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

	// Heart Rate Variability Characteristic Value
	[HRS_IDX_HR_VARIABILITY_VAL]             =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_variability_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
	  4*sizeof(uint8_t), sizeof(hr_variability_val), (uint8_t *)hr_variability_val}},

    // Heart Rate Object Size Characteristic Declaration
    [HRS_IDX_HR_OBJ_SIZE_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Heart Rate Object Size Characteristic Value
    [HRS_IDX_HR_OBJ_SIZE_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_obj_size_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      8*sizeof(uint8_t), sizeof(hr_obj_size_val), (uint8_t *)hr_obj_size_val}},
};


/// Health Thermomether Sensor Service
static const uint16_t health_thermometer_svc = ESP_GATT_UUID_HEALTH_THERMOM_SVC;

/// Health Thermomether Sensor Service - Health Thermomether Measurement Characteristic, indicate. Descriptor: Client Characteristic Configuration
static const uint16_t ht_temp_meas_uuid = 0x2A1C;
static uint8_t ht_temp_meas_val[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x04}; //Temperature Type(1) = 0x02, Temperature Measure(4), Flags(1) = 0x04
static uint8_t ht_temp_meas_ccc[2] = {0x00, 0x00}; 

/// Health Thermomether Sensor Service - Temperature Type Characteristic, read
static const uint16_t ht_temp_type_uuid = 0x2A1D;
static const uint8_t ht_temp_type_val[1] = {0x02};

/// Health Thermomether Sensor Service - Heart Rate Measure Interval Characteristic, write&read
static const uint16_t ht_meas_int_uuid = 0x2A21;
static RTC_DATA_ATTR uint8_t ht_meas_int_val[2] = {0x2C, 0x01}; //First byte = LSB
static RTC_DATA_ATTR uint8_t ht_mi_1;
static RTC_DATA_ATTR uint8_t ht_mi_2;

/// Health Thermometer Sensor Service - Health Thermometer Object Size Characteristic, write&read
static const uint16_t ht_obj_size_uuid = 0x2AC0;
static uint8_t ht_obj_size_val[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/// Full HTS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t health_thermometer_gatt_db[HTS_IDX_NB] = {
    // Health Thermometer Service Declaration
    [HTS_IDX_SVC]                       =  
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(health_thermometer_svc), (uint8_t *)&health_thermometer_svc}},

    // Health Thermometer Measurement Characteristic Declaration
    [HTS_IDX_HT_MEAS_CHAR]            = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_indicate}},
      
    // Health Thermometer Measurement Characteristic Value
    [HTS_IDX_MEAS_VAL]               =   
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&ht_temp_meas_uuid, ESP_GATT_PERM_READ,
      6*sizeof(uint8_t), sizeof(ht_temp_meas_val), (uint8_t *)&ht_temp_meas_val}},

    // Health Thermometer Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_HT_MEAS_CCC]       =    
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(ht_temp_meas_ccc), (uint8_t *)ht_temp_meas_ccc}},

    // Temperature Type Characteristic Declaration
    [HTS_IDX_TEMP_TYPE_CHAR]  = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // Temperature Type Characteristic Value
    [HTS_IDX_TEMP_TYPE_VAL]   = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&ht_temp_type_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(ht_temp_type_val), (uint8_t *)ht_temp_type_val}},

    // Health Thermometer Measure Interval Characteristic Declaration
    [HTS_IDX_HT_MI_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Health Thermometer Measure Interval Characteristic Value
    [HTS_IDX_HT_MI_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&ht_meas_int_uuid, ESP_GATT_PERM_WRITE|ESP_GATT_PERM_READ,
      2*sizeof(uint8_t), sizeof(ht_meas_int_val), (uint8_t *)ht_meas_int_val}},

    // Health Thermometer Object Size Characteristic Declaration
    [HTS_IDX_HT_OBJ_SIZE_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Health Thermometer Object Size Characteristic Value ESP_GATT_RSP_BY_APP
    [HTS_IDX_HT_OBJ_SIZE_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&ht_obj_size_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      8*sizeof(uint8_t), sizeof(ht_obj_size_val), (uint8_t *)ht_obj_size_val}},
};


/// Pulse Oximeter Sensor Service
static const uint16_t pulse_oximeter_svc = 0x1822;

/// Pulse Oximeter Sensor Service - PlX Spot Check Measurement Characteristic, indicate. Descriptor: Client Characteristic Configuration
static const uint16_t po_plx_spot_check_meas_uuid = 0x2A5E;
static uint8_t po_plx_spot_check_meas_val[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}; //Device and Sensor Status(3), Measurement Status(2), PR(2), Sp02(2), Flags(1)
static uint8_t po_plx_spot_check_meas_ccc[2] = {0x00, 0x00}; 

/// Pulse Oximeter Sensor Service - PLX Features Characteristic, read
static const uint16_t po_plx_feat_uuid = 0x2A60;
static const uint8_t po_plx_feat_val[7] = {0x00, 0x00, 0x11, 0xC7, 0x01, 0x00, 0x76};

/// Pulse Oximeter Sensor Service - Pulse Oximeter Measure Interval Characteristic, write&read
static const uint16_t po_meas_int_uuid = 0x2A21;
static RTC_DATA_ATTR uint8_t po_meas_int_val[2] = {0x2C, 0x01};
static RTC_DATA_ATTR uint8_t po_mi_1;
static RTC_DATA_ATTR uint8_t po_mi_2;

/// Pulse Oximeter Sensor Service - Pulse Oximeter Object Size Characteristic, write&read
static const uint16_t po_obj_size_uuid = 0x2AC0;
static uint8_t po_obj_size_val[8] = {0x00, 0x00, 0x00, 0x00, ((PO_STORED_SAMPLES >> 24) & 0xFF), ((PO_STORED_SAMPLES >> 16) & 0xFF), ((PO_STORED_SAMPLES >> 8) & 0xFF), (PO_STORED_SAMPLES & 0xFF)};

/// Full POS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t pulse_oximeter_gatt_db[POS_IDX_NB] = {
    // Pulse Oximeter Service Declaration
    [POS_IDX_SVC]                       =  
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(pulse_oximeter_svc), (uint8_t *)&pulse_oximeter_svc}},

    // Pulse Oximeter PlX Spot Check Measurement Characteristic Characteristic Declaration
    [POS_IDX_PLX_SPOT_CHAR]            = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_indicate}},
      
    // PlX Spot Check Measurement Characteristic Value
    [POS_IDX_PLX_SPOT_VAL]               =   
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&po_plx_spot_check_meas_uuid, ESP_GATT_PERM_READ,
      10*sizeof(uint8_t), sizeof(po_plx_spot_check_meas_val), (uint8_t *)&po_plx_spot_check_meas_val}},

    // PlX Spot Check Measurement Characteristic - Client Characteristic Configuration Descriptor
    [POS_IDX_PLX_SPOT_CCC]       =    
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(po_plx_spot_check_meas_ccc), (uint8_t *)po_plx_spot_check_meas_ccc}},

    // PLX Features Characteristic Declaration
    [POS_IDX_PLX_FEAT_CHAR]  = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // PLX Features Characteristic Value
    [POS_IDX_PLX_FEAT_VAL]   = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&po_plx_feat_uuid, ESP_GATT_PERM_READ,
      7*sizeof(uint8_t), sizeof(po_plx_feat_val), (uint8_t *)po_plx_feat_val}},

    // Pulse Oximeter Measure Interval Characteristic Declaration
    [POS_IDX_PO_MI_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Pulse Oximeter Measure Interval Characteristic Value
    [POS_IDX_PO_MI_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&po_meas_int_uuid, ESP_GATT_PERM_WRITE|ESP_GATT_PERM_READ,
      2*sizeof(uint8_t), sizeof(po_meas_int_val), (uint8_t *)po_meas_int_val}}, 

    // Pulse Oximeter Object Size Characteristic Declaration
    [POS_IDX_PO_OBJ_SIZE_CHAR]          = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
                                                    
    // Pulse Oximeter Object Size Characteristic Value
    [POS_IDX_PO_OBJ_SIZE_VAL]             = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&po_obj_size_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      8*sizeof(uint8_t), sizeof(po_obj_size_val), (uint8_t *)po_obj_size_val}},
};


/// Battery Service
static const uint16_t battery_svc = ESP_GATT_UUID_BATTERY_SERVICE_SVC;

/// Battery Service - Battery Level, read&indicate. Descriptors: Client Characteristic Configuration & Characteristic Presentation Format
static const uint16_t bs_bat_level_uuid = 0x2A19;
static uint8_t bs_bat_level_val[1] = {0x00}; //Battery Level

/// Full BS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t battery_service_gatt_db[BS_IDX_NB] = {
    // Battery Service Declaration
    [BS_IDX_SVC]                       =  
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(battery_svc), (uint8_t *)&battery_svc}},

    // Battery Level Characteristic Declaration
    [BS_BAT_LVL_CHAR]            = 
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
      
    // Battery Level Characteristic Value
    [BS_BAT_LVL_VAL]               =   
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&bs_bat_level_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(bs_bat_level_val), (uint8_t *)&bs_bat_level_val}},
};

/// Steps Service
static const uint16_t steps_svc = 0x1826;

/// Steps Service - Steps Number, read&indicate.
static const uint16_t ss_steps_number_uuid = 0x2ACF;
static uint8_t ss_steps_number_val[4] = {0x01, 0x02, 0x03, 0x04}; //Number of steps

/// Full SS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t steps_service_gatt_db[SS_IDX_NB] = {
    // Steps Service Declaration
    [SS_IDX_SVC]                       =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(steps_svc), (uint8_t *)&steps_svc}},

    // Steps Number Characteristic Declaration
    [SS_STEPS_NUMBER_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // Steps Number Characteristic Value
    [SS_STEPS_NUMBER_VAL]               =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&ss_steps_number_uuid, ESP_GATT_PERM_READ,
      4*sizeof(uint8_t), sizeof(ss_steps_number_val), (uint8_t *)&ss_steps_number_val}},
};

/// Fall Service
static const uint16_t fall_svc = 0x1811;

/// Fall Service - Fall detected, read&indicate.
static const uint16_t fs_fall_detected_uuid = 0x2A46;
static uint8_t fs_fall_detected_val[1] = {0x00}; //Fall detection

/// Full FS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t fall_service_gatt_db[FS_IDX_NB] = {
    // Fall Service Declaration
    [FS_IDX_SVC]                       =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(fall_svc), (uint8_t *)&fall_svc}},

    // Fall detected Characteristic Declaration
    [FS_FALL_DETECTED_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // Fall detected Characteristic Value
    [FS_FALL_DETECTED_VAL]               =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&fs_fall_detected_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(fs_fall_detected_val), (uint8_t *)&fs_fall_detected_val}},
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    ESP_LOGI(GATTS_TAG, "GAP_EVT, event %d\n", event);

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    /*
    case ESP_GAP_BLE_SEC_REQ_EVT:
        // send the positive(true) security response to the peer device to accept the security request.
        //If not accept the security request, should sent the security response with negative(false) accept value
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer deivce.
        ESP_LOGE(GATTS_TAG, "The passkey Notify number:%d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        ESP_LOGI(GATTS_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTS_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(GATTS_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(GATTS_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        break;
    }
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
        ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
        break;
    }
    case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT: {
        ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT status = %d", param->clear_bond_dev_cmpl.status);
        break;
    }
    case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT: {
        ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT status = %d, num = %d", param->get_bond_dev_cmpl.status, param->get_bond_dev_cmpl.dev_num);
        esp_ble_bond_dev_t *bond_dev = param->get_bond_dev_cmpl.bond_dev;
        for(int i = 0; i < param->get_bond_dev_cmpl.dev_num; i++) {
            ESP_LOGD(GATTS_TAG, "mask = %x", bond_dev[i].bond_key.key_mask);
            esp_log_buffer_hex(GATTS_TAG, (void *)bond_dev[i].bd_addr, sizeof(esp_bd_addr_t));
        }
        break;
    }
    */
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
    /* If event is register event, store the gatts_if for each profile */
    ESP_LOGI(GATTS_TAG, "EVT %d, gatts if %d\n", event, gatts_if);

    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[PROFILE_A_APP_ID].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id, 
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(GATTS_TAG, "%s %d\n", __func__, __LINE__);
            esp_ble_gap_set_device_name(DEVICE_NAME);
            ESP_LOGI(GATTS_TAG, "%s %d\n", __func__, __LINE__);
            esp_ble_gap_config_adv_data(&adv_data);
            ESP_LOGI(GATTS_TAG, "%s %d\n", __func__, __LINE__);
            esp_ble_gatts_create_attr_tab(heart_rate_gatt_db, gatts_if, HRS_IDX_NB, HEART_RATE_SVC_INST_ID);
            esp_ble_gatts_create_attr_tab(health_thermometer_gatt_db, gatts_if, HTS_IDX_NB, HEALTH_THERMOMETER_SVC_INST_ID);
            esp_ble_gatts_create_attr_tab(pulse_oximeter_gatt_db, gatts_if, POS_IDX_NB, HEALTH_THERMOMETER_SVC_INST_ID);
            esp_ble_gatts_create_attr_tab(battery_service_gatt_db, gatts_if, BS_IDX_NB, BATTERY_SERVICE_SVC_INST_ID);
            esp_ble_gatts_create_attr_tab(steps_service_gatt_db, gatts_if, SS_IDX_NB, STEPS_SVC_INST_ID);
            esp_ble_gatts_create_attr_tab(fall_service_gatt_db, gatts_if, FS_IDX_NB, FALL_SVC_INST_ID);
            break;
        
        case ESP_GATTS_READ_EVT: {
            gettimeofday(&timeout_to_no_bt_packet_reception, NULL);
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
            if(param->read.handle == battery_service_handle_table[BS_BAT_LVL_VAL]) {
				ESP_LOGI(GATTS_TAG, "Battery level sent. Comm was successfully\n");
				last_po_measures_index = 0;
				last_po_measures_size = 0;
			}
            if(param->read.handle == steps_handle_table[SS_STEPS_NUMBER_VAL]) {
            	last_po_measures_index = 0;
            	last_po_measures_size = 0;
            	lsm6ds3_reset_pedometer(true);
            }
            if(param->read.handle == fall_handle_table[FS_FALL_DETECTED_VAL]){
            	times_requested_falls++;
            	if(times_requested_falls == 2){
            		bs_srv1.bs_meas_val = true;
            	}
            }
            break;
        }
        
        case ESP_GATTS_WRITE_EVT: {
            gettimeofday(&timeout_to_no_bt_packet_reception, NULL);
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value %08x\n", param->write.len, *(uint32_t *)param->write.value);
            if(param->write.handle == heart_rate_handle_table[HRS_IDX_HR_CTNL_PT_VAL]) {
                save_mi_or_ccc_into_bt_array(param->write.value, 0);
                ESP_LOGI(GATTS_TAG, "HR MI modified: %02x, %02x\n", hr_meas_int_val[0], hr_meas_int_val[1]);
                hr_srv1.hr_mi = true;
            }
            else if(param->write.handle == health_thermometer_handle_table[HTS_IDX_HT_MI_VAL]){
            	last_ht_measures_index = 0; //If we are modifying the HT Measure Interval is because HT comm was succesfully
            	last_ht_measures_size = 0;
                save_mi_or_ccc_into_bt_array(param->write.value, 1);
                ESP_LOGI(GATTS_TAG, "HT MI modified %02x, %02x\n", ht_meas_int_val[0], ht_meas_int_val[1]);
                ht_srv1.ht_mi = true;
            }
            else if(param->write.handle == pulse_oximeter_handle_table[POS_IDX_PO_MI_VAL]){
            	last_po_measures_index = 0; //If we are modifying the PO Measure Interval is because PO comm was succesfully
            	last_po_measures_size = 0;
                save_mi_or_ccc_into_bt_array(param->write.value, 2);
                ESP_LOGI(GATTS_TAG, "PO MI modified %02x, %02x\n", po_meas_int_val[0], po_meas_int_val[1]);
                po_srv1.po_mi = true;
            }
            else if(param->write.handle == heart_rate_handle_table[HRS_IDX_HR_MEAS_CCC]) {
            	last_hr_measures_index = 0; //If we are modifying the HR Measure Interval is because HR comm was succesfully
            	last_hr_measures_size = 0;
                save_mi_or_ccc_into_bt_array(param->write.value, 3);
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                ESP_LOGI(GATTS_TAG, "HR CCC modified %02x, %02x, received value= %d\n", hr_meas_ccc[0], hr_meas_ccc[1], descr_value);
                if(descr_value == 1){
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[HRS_IDX_HR_MEAS_VAL], 
                    sizeof(hr_meas_val), (uint8_t *)hr_meas_val, false);
                }
            } 
            else if(param->write.handle == health_thermometer_handle_table[HTS_IDX_HT_MEAS_CCC]){
                save_mi_or_ccc_into_bt_array(param->write.value, 4);
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                ESP_LOGI(GATTS_TAG, "HT CCC modified %02x, %02x\n", ht_temp_meas_ccc[0], ht_temp_meas_ccc[1]);
                if(descr_value == 2){
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, health_thermometer_handle_table[HTS_IDX_MEAS_VAL], 
                    sizeof(ht_temp_meas_val), (uint8_t *)ht_temp_meas_val, true);
                }
            }
            else if(param->write.handle == pulse_oximeter_handle_table[POS_IDX_PLX_SPOT_CCC]){
                save_mi_or_ccc_into_bt_array(param->write.value, 5);
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                ESP_LOGI(GATTS_TAG, "PO CCC modified %02x, %02x\n", po_plx_spot_check_meas_ccc[0], po_plx_spot_check_meas_ccc[1]);
                if(descr_value == 2){
					esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, pulse_oximeter_handle_table[POS_IDX_PLX_SPOT_VAL],
					sizeof(po_plx_spot_check_meas_val), (uint8_t *)po_plx_spot_check_meas_val, true);
				}
            }
            else if(param->write.handle == health_thermometer_handle_table[HTS_IDX_HT_OBJ_SIZE_VAL]){
                int index = 0;
                float next_temp = 0;

                index = update_object_size_from_value_of_client(1, param->write.value);
                if(index < HT_STORED_SAMPLES){
                	last_ht_measures_index--;
                	last_ht_measures_size--;
                	if(last_ht_measures_index == 0){
                		last_ht_measures_index = HT_STORED_SAMPLES;
                	}
                    next_temp = last_ht_measures[last_ht_measures_index-1];
                    save_temp_into_bt_array_from_float(next_temp);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, health_thermometer_handle_table[HTS_IDX_MEAS_VAL],
                    		sizeof(ht_temp_meas_val), (uint8_t *)ht_temp_meas_val, true);
                } else {
                    ESP_LOGI(GATTS_TAG, "Index received exceeds HT array size\n");
                }
            }

            else if(param->write.handle == heart_rate_handle_table[HRS_IDX_HR_OBJ_SIZE_VAL]){
				int index = 0;
				uint8_t next_beats = 0;
				float next_variability = 0;

				index = update_object_size_from_value_of_client(0, param->write.value);
				if(index < HR_STORED_SAMPLES){
					last_hr_measures_index--;
					last_hr_variability_measures_index--;
					if(last_hr_measures_index == 0){
						last_hr_measures_index = HR_STORED_SAMPLES;
					}
					if(last_hr_variability_measures_index == 0){
						last_hr_variability_measures_index = HR_STORED_SAMPLES;
					}
					next_beats = last_hr_measures[last_hr_measures_index-1];
					next_variability = last_hr_variability_measures[last_hr_variability_measures_index-1];
					save_hr_and_variability_into_bt_array(next_beats, next_variability);
					esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[HRS_IDX_HR_MEAS_VAL],
							sizeof(hr_meas_val), (uint8_t *)hr_meas_val, false);
				} else {
					ESP_LOGI(GATTS_TAG, "Index received exceeds HR array size\n");
				}
			}

            else if(param->write.handle == pulse_oximeter_handle_table[POS_IDX_PO_OBJ_SIZE_VAL]){
				int index = 0;
				uint8_t next_saturation = 0;

				index = update_object_size_from_value_of_client(2, param->write.value);
				if(index < PO_STORED_SAMPLES){
					last_po_measures_index--;
					if(last_po_measures_index == 0){
						last_po_measures_index = PO_STORED_SAMPLES;
					}
					next_saturation = last_po_measures[last_po_measures_index-1];
					save_spo2_into_bt_array(next_saturation);
					esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, pulse_oximeter_handle_table[POS_IDX_PLX_SPOT_VAL],
							sizeof(po_plx_spot_check_meas_val), (uint8_t *)po_plx_spot_check_meas_val, true);
				} else {
					ESP_LOGI(GATTS_TAG, "Index received exceeds PO array size\n");
				}
			}

            break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
        case ESP_GATTS_MTU_EVT:
        case ESP_GATTS_CONF_EVT:
        case ESP_GATTS_UNREG_EVT:
            break;
        case ESP_GATTS_CREATE_EVT:
            break;
        case ESP_GATTS_ADD_INCL_SRVC_EVT:
            break;
        case ESP_GATTS_ADD_CHAR_EVT: {
            break;
        }
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            break;
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_START_EVT:   
            ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                     param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_STOP_EVT:
            break;
        case ESP_GATTS_CONNECT_EVT:
        	gettimeofday(&timeout_to_no_bt_packet_reception, NULL);
            //start security connect with peer device when receive the connect event sent by the master.
            //esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:, is_conn \n", //was %d
                     param->connect.conn_id,
                     param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                     param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
                     //param->connect.is_connected);
            gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
            
            break;
        case ESP_GATTS_DISCONNECT_EVT:
        	ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            ESP_LOGI(GATTS_TAG, "The number handle =%x\n",param->add_attr_tab.num_handle);
            ESP_LOGI(GATTS_TAG, "The handle =%x\n",*param->add_attr_tab.handles);
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TAG, "Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            } else {
                switch(number_of_services){
                    case 0:
                        memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
                        esp_ble_gatts_start_service(heart_rate_handle_table[HRS_IDX_SVC]);
                        
                        number_of_services++;
                        break;
                    case 1:
                        memcpy(health_thermometer_handle_table, param->add_attr_tab.handles, sizeof(health_thermometer_handle_table));
                        esp_ble_gatts_start_service(health_thermometer_handle_table[HTS_IDX_SVC]);
                        
                        number_of_services++;
                        break;
                    case 2:
                        memcpy(pulse_oximeter_handle_table, param->add_attr_tab.handles, sizeof(pulse_oximeter_handle_table));
                        esp_ble_gatts_start_service(pulse_oximeter_handle_table[POS_IDX_SVC]);
                        number_of_services++;
                        break;
                    case 3:
                        memcpy(battery_service_handle_table, param->add_attr_tab.handles, sizeof(battery_service_handle_table));
                        esp_ble_gatts_start_service(battery_service_handle_table[BS_IDX_SVC]);
                        number_of_services++;
                        break;
                    case 4:
						memcpy(steps_handle_table, param->add_attr_tab.handles, sizeof(steps_handle_table));
						esp_ble_gatts_start_service(steps_handle_table[SS_IDX_SVC]);
						number_of_services++;
						break;
                    case 5:
						memcpy(fall_handle_table, param->add_attr_tab.handles, sizeof(fall_handle_table));
						esp_ble_gatts_start_service(fall_handle_table[FS_IDX_SVC]);
						number_of_services++;
						break;
                    default:
                    	printf("No more services to add\n");
                        break;
                }
            }
            break;
        }
        default:
            break;
        }
}

esp_err_t bluetooth_init(){
    esp_err_t ret;

    if(bt_correctly_init){
    	return ESP_OK;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if(ret){
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "controller_init ok\n");
    //Set BLE TX power Connection Tx power should only be set after connection created.
    //esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N14);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE); //ESP_BT_MODE_BTDM (dual); ESP_BT_MODE_CLASSIC_BT (classic); ESP_BT_MODE_BLE (BLE)
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "controller_enable ok\n");
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "bluedroid_init ok\n");
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "bluedroid_enable ok\n");
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gatts register callback failed\n", __func__);
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "gatss_register_callback ok\n");
    ret = esp_ble_gap_register_callback(gap_event_handler); //Callback before the connection is established (starts the advertising process)
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap register callback failed\n", __func__);
        return ret;
    }
    ESP_LOGI(GATTS_TAG, "gap_register_callback ok\n");
    ret = esp_ble_gatts_app_register(ESPMART105_APP_ID);
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gatts app register failed\n", __func__);
    }
    ESP_LOGI(GATTS_TAG, "gatss_app_register ok\n");

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    /*
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap_set_security_param failed\n", __func__);
    }
    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap_set_security_param failed\n", __func__);
    }
    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap_set_security_param failed\n", __func__);
    }
    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap_set_security_param failed\n", __func__);
    }
    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    if(ret){
        ESP_LOGE(GATTS_TAG, "%s gap_set_security_param failed\n", __func__);
    }
    printf("gap_set_security_param ok\n");
    */
    bt_correctly_init = 1;
    return ret;
}

static void check_for_BS_reception(){
    struct timeval time2;
    uint64_t time_waiting_ms;

    gettimeofday(&timeout_to_no_bt_packet_reception, NULL);
    ESP_LOGI(GATTS_TAG, "Waiting for BS packet\n");
    while (!bs_srv1.bs_meas_val) {
        gettimeofday(&time2, NULL);
        time_waiting_ms = (time2.tv_sec - timeout_to_no_bt_packet_reception.tv_sec) * 1000 + (time2.tv_usec - timeout_to_no_bt_packet_reception.tv_usec) / 1000;
        vTaskDelay(10/portTICK_PERIOD_MS);
        if(time_waiting_ms >= BT_TIMEOUT_RECEPTION_TIME_MS){
            printf("Comm was not successful. Checking now what is missing\n");
            break;
        }
    }

    if(hr_srv1.hr_mi == true){
        ESP_LOGI(GATTS_TAG, "HR MI packet received.\n");
        hr_srv1.hr_mi = false;
    } else {
        printf("HR MI packet NOT received.\n");
    }
    convert_hex_measure_intervals_to_time_in_useconds(1);
    ESP_LOGI(GATTS_TAG, "Value: %d s\n", (int)hr_time/1000000);
    
    if(ht_srv1.ht_mi == true){
        ESP_LOGI(GATTS_TAG, "HT MI packet received.\n");
        ht_srv1.ht_mi = false;
    } else {
        printf("HT MI packet NOT received.\n");
    }
    convert_hex_measure_intervals_to_time_in_useconds(2);
    ESP_LOGI(GATTS_TAG, "Value: %d s\n", (int)temp_time/1000000);

    if(po_srv1.po_mi == true){
        ESP_LOGI(GATTS_TAG, "PO MI packet received.\n");
        po_srv1.po_mi = false;
    } else {
        printf("PO MI packet NOT received.\n");
    }
    convert_hex_measure_intervals_to_time_in_useconds(3);
    ESP_LOGI(GATTS_TAG, "Value: %d s\n", (int)spo2_time/1000000);
}

//ADC FUNCTIONS
esp_err_t adc_init(){
	esp_err_t ret;
	//Configure ADC
	if (unit == ADC_UNIT_1) {
		ret = adc1_config_width(ADC_WIDTH_BIT_12);
		if (ret) {return ret;}
		ret = adc1_config_channel_atten(channel, atten);
		if (ret) {return ret;}
	} else {
		ret = adc2_config_channel_atten((adc2_channel_t)channel, atten);
		if (ret) {return ret;}
	}

	//Characterize ADC
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

	return ret;
}

uint8_t check_battery_level(){ //returns the warning level when it is read at least three times. 0 otherwise
	uint32_t adc_reading = 0;
	//Multisampling
	for (int i = 0; i < NO_OF_SAMPLES; i++) {
		if (unit == ADC_UNIT_1) {
			adc_reading += adc1_get_raw((adc1_channel_t)channel);
		} else {
			int raw;
			adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
			adc_reading += raw;
		}
	}
	adc_reading /= NO_OF_SAMPLES;
	//Convert adc_reading to voltage in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

    uint8_t voltage_percentage = 0;
    if(voltage >=TEN_PERCENT_BATTERY_MV){
    	voltage_percentage = (uint8_t) 100- (ADC_SLOPE_1*(HUNDRED_PERCENT_BATTERY_MV-voltage));
    } else {
    	voltage_percentage = (uint8_t) 100- (ADC_SLOPE_2*(TEN_PERCENT_BATTERY_MV-voltage));
    }

    if(voltage_percentage>=100){ // The voltage divisor has a 0.3 factor, so 4.2V (battery full) is equal to 1.26 V
        bs_bat_level_val[0] = 100;
    }
    else{
        bs_bat_level_val[0] = voltage_percentage;
    }

    return voltage_percentage;
}

//I2C FUNCTIONS
esp_err_t i2c_master_init(){
    esp_err_t ret;
    int i2c_master_port = I2C_MASTER_NUM0;
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    ret = i2c_param_config(i2c_master_port, &conf);
    if (ret) {
        return ret;
    }
    
    ret = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret;
}

//OTHER FUNCTIONS
static void convert_hex_measure_intervals_to_time_in_useconds(int measure_interval_type){ // 0: converts all measure intervals, 1: hr, 2: ht, 3: po
    uint64_t b;
    uint8_t temporal [2];
    switch (measure_interval_type){
        case 0:
            convert_hex_measure_intervals_to_time_in_useconds(1);
            convert_hex_measure_intervals_to_time_in_useconds(2);
            convert_hex_measure_intervals_to_time_in_useconds(3);
            break;
        case 1:
            temporal[0] = hr_meas_int_val[1];
            temporal[1] = hr_meas_int_val[0];
            b = ntohs(*(uint16_t*)temporal);
            hr_time = b *1000000L;
            break;
        case 2:
            temporal[0] = ht_meas_int_val[1];
            temporal[1] = ht_meas_int_val[0];
            b = ntohs(*(uint16_t*)temporal);
            temp_time = b *1000000L;
            break;
        case 3:
            temporal[0] = po_meas_int_val[1];
            temporal[1] = po_meas_int_val[0];
            b = ntohs(*(uint16_t*)temporal);
            spo2_time = b *1000000L;
            break;
        default:
            break;
    }
}

static void get_times_since_last_measures(){
    struct timeval now;
    uint64_t sleep_time_ms_since_last_deep_sleep;
    uint64_t sleep_time_ms_since_last_po;
    uint64_t sleep_time_ms_since_last_hr;
    uint64_t sleep_time_ms_since_last_ht;

    gettimeofday(&now, NULL);
    sleep_time_ms_since_last_deep_sleep =  (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

    sleep_time_ms_since_last_hr =  (now.tv_sec - time_last_hr_measure.tv_sec) * 1000 + (now.tv_usec - time_last_hr_measure.tv_usec) / 1000;
    sleep_time_ms_since_last_ht =  (now.tv_sec - time_last_ht_measure.tv_sec) * 1000 + (now.tv_usec - time_last_ht_measure.tv_usec) / 1000;
    sleep_time_ms_since_last_po =  (now.tv_sec - time_last_po_measure.tv_sec) * 1000 + (now.tv_usec - time_last_po_measure.tv_usec) / 1000;
    time_since_last_hr_measure += sleep_time_ms_since_last_hr;
    time_since_last_ht_measure += sleep_time_ms_since_last_ht;
    time_since_last_spo2_measure += sleep_time_ms_since_last_po;

    ESP_LOGI(GATTS_TAG, "Time spent in deep sleep: %llu ms\n", sleep_time_ms_since_last_deep_sleep);
    ESP_LOGI(GATTS_TAG, "Time since last HR measure: %llu ms\n", time_since_last_hr_measure);
    ESP_LOGI(GATTS_TAG, "Time since last HT measure: %llu ms\n", time_since_last_ht_measure);
    ESP_LOGI(GATTS_TAG, "Time since last PO measure: %llu ms\n", time_since_last_spo2_measure);
}

static uint64_t get_time_to_sleep(){

    convert_hex_measure_intervals_to_time_in_useconds(0);

    uint64_t time_to_new_hr_measure = 0;
    uint64_t time_to_new_ht_measure = 0;
    uint64_t time_to_new_po_measure = 0;

    if(time_since_last_hr_measure*1000 < hr_time){
        time_to_new_hr_measure = hr_time - time_since_last_hr_measure*1000;
    }
    if(time_since_last_ht_measure*1000 < temp_time){
        time_to_new_ht_measure = temp_time - time_since_last_ht_measure*1000;
    }
    if(time_since_last_spo2_measure*1000 < spo2_time){
        time_to_new_po_measure = spo2_time - time_since_last_spo2_measure*1000;
    }

    uint64_t time_to_sleep = min(time_to_new_hr_measure, min(time_to_new_ht_measure, time_to_new_po_measure));

    ESP_LOGI(GATTS_TAG, "Time since last HR measure: %llu ms\n", time_since_last_hr_measure);
    ESP_LOGI(GATTS_TAG, "Time since last HT measure: %llu ms\n", time_since_last_ht_measure);
    ESP_LOGI(GATTS_TAG, "Time since last PO measure: %llu ms\n", time_since_last_spo2_measure);



    ESP_LOGI(GATTS_TAG, "hr time: %llu ms.\n", hr_time);
    ESP_LOGI(GATTS_TAG, "ht_time: %llu ms.\n", temp_time);
    ESP_LOGI(GATTS_TAG, "spo2 time: %llu ms.\n", spo2_time);



    ESP_LOGI(GATTS_TAG, "Time to new hr measure: %llu ms.\n", time_to_new_hr_measure/1000);
    ESP_LOGI(GATTS_TAG, "Time to new ht measure: %llu ms.\n", time_to_new_ht_measure/1000);
    ESP_LOGI(GATTS_TAG, "Time to new po measure: %llu ms.\n", time_to_new_po_measure/1000);
    ESP_LOGI(GATTS_TAG, "Time until next wakeup: %llu ms.\n", time_to_sleep/1000);
    
    return time_to_sleep;
}

static void save_spo2_into_bt_array(uint8_t saturation_value){

	po_plx_spot_check_meas_val[8]= saturation_value;

	if(bt_correctly_init == 1 ){
		esp_ble_gatts_set_attr_value(pulse_oximeter_handle_table[POS_IDX_PLX_SPOT_VAL], sizeof(po_plx_spot_check_meas_val), (const uint8_t*) &po_plx_spot_check_meas_val);
	}
}

static void save_hr_and_variability_into_bt_array(uint8_t beats, float variancee){
	char temporal[4];

	hr_meas_val[0]= beats;

	memcpy(temporal, (unsigned char*) (&variancee), 4);

	hr_variability_val[0]= temporal[3];
	hr_variability_val[1]= temporal[2];
	hr_variability_val[2]= temporal[1];
	hr_variability_val[3]= temporal[0];

	if(bt_correctly_init == 1 ){
		esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_MEAS_VAL], sizeof(hr_meas_val), (const uint8_t*) &hr_meas_val);
		esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_VARIABILITY_VAL], sizeof(hr_variability_val), (const uint8_t*) &hr_variability_val);
	}
}

static void save_temp_into_bt_array(uint8_t data_h, uint8_t data_l){
    float temperat = convert_temp(data_h, data_l);
    save_temp_into_bt_array_from_float(temperat);
}

static void save_temp_into_bt_array_from_float(float temperature){
    char temporal[4];

    memcpy(temporal, (unsigned char*) (&temperature), 4);
    ht_temp_meas_val[1]= temporal[3];
    ht_temp_meas_val[2]= temporal[2];
    ht_temp_meas_val[3]= temporal[1];
    ht_temp_meas_val[4]= temporal[0];

    if(bt_correctly_init == 1 ){
        esp_ble_gatts_set_attr_value(health_thermometer_handle_table[HTS_IDX_MEAS_VAL], sizeof(ht_temp_meas_val), (const uint8_t*) &ht_temp_meas_val);
    }
}

static uint32_t update_object_size_from_value_of_client(int measure_object_size, uint8_t *value){
    uint32_t ret = 0;

    ret |= value[4] << 24;
    ret |= value[5] << 16;
    ret |= value[6] << 8;
    ret |= value[7];

    uint8_t* p =  value;
    uint8_t v4 = *p;
    p++;
    uint8_t v3 = *p;
    p++;
    uint8_t v2 = *p;
    p++;
    uint8_t v1 = *p;

    p++;
    uint8_t v8 = *p;
    p++;
    uint8_t v7 = *p;
    p++;
    uint8_t v6 = *p;
    p++;
    uint8_t v5 = *p;

    switch(measure_object_size){
        case 0:
            hr_obj_size_val[0] = v1;
            hr_obj_size_val[1] = v2;
            hr_obj_size_val[2] = v3;
            hr_obj_size_val[3] = v4;

            hr_obj_size_val[4] = v5;
			hr_obj_size_val[5] = v6;
			hr_obj_size_val[6] = v7;
			hr_obj_size_val[7] = v8;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_OBJ_SIZE_VAL], sizeof(hr_obj_size_val), (const uint8_t*) &hr_obj_size_val);
            }
            break;
        case 1:
            ht_obj_size_val[0] = v1;
            ht_obj_size_val[1] = v2;
            ht_obj_size_val[2] = v3;
            ht_obj_size_val[3] = v4;

            ht_obj_size_val[4] = v5;
            ht_obj_size_val[5] = v6;
			ht_obj_size_val[6] = v7;
			ht_obj_size_val[7] = v8;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(health_thermometer_handle_table[HTS_IDX_HT_OBJ_SIZE_VAL], sizeof(ht_obj_size_val), (const uint8_t*) &ht_obj_size_val);
            }
            break;
        case 2:
            po_obj_size_val[0] = v1;
            po_obj_size_val[1] = v2;
            po_obj_size_val[2] = v3;
            po_obj_size_val[3] = v4;

            po_obj_size_val[4] = v5;
			po_obj_size_val[5] = v6;
			po_obj_size_val[6] = v7;
			po_obj_size_val[7] = v8;

            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(pulse_oximeter_handle_table[POS_IDX_PO_OBJ_SIZE_VAL], sizeof(po_obj_size_val), (const uint8_t*) &po_obj_size_val);
            }
            break;
        default:
            break;
    }

    ESP_LOGI(GATTS_TAG, "New object size reception = %d .\n", (ret));
    return ret;
}

static void save_mi_or_ccc_into_bt_array(uint8_t *value, int mi_or_ccc_type){ // 0: HR MI, 1: HT MI, 2: PO MI, 3: HR CCC, 4: HT CCC, 5: PO CCC
    uint8_t* p =  value;
    uint8_t v2 = *p;
    p++;
    uint8_t v1 = *p;

    switch(mi_or_ccc_type){
        case 0:
            hr_meas_int_val[1] = v1;
            hr_meas_int_val[0] = v2;
            hr_mi_2 = v1;
            hr_mi_1 = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_CTNL_PT_VAL], sizeof(hr_meas_int_val), (const uint8_t*) &hr_meas_int_val);
            }
            break;
        case 1:
            ht_meas_int_val[1] = v1;
            ht_meas_int_val[0] = v2;
            ht_mi_2 = v1;
            ht_mi_1 = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(health_thermometer_handle_table[HTS_IDX_HT_MI_VAL], sizeof(ht_meas_int_val), (const uint8_t*) &ht_meas_int_val);
            }
            break;
        case 2:
            po_meas_int_val[1] = v1;
            po_meas_int_val[0] = v2;
            po_mi_2 = v1;
            po_mi_1 = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(pulse_oximeter_handle_table[POS_IDX_PO_MI_VAL], sizeof(po_meas_int_val), (const uint8_t*) &po_meas_int_val);
            }
            break;
        case 3:
            hr_meas_ccc[1] = v1;
            hr_meas_ccc[0] = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_MEAS_CCC], sizeof(hr_meas_ccc), (const uint8_t*) &hr_meas_ccc);
            }
            break;
        case 4:
            ht_temp_meas_ccc[1] = v1;
            ht_temp_meas_ccc[0] = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(health_thermometer_handle_table[HTS_IDX_HT_MEAS_CCC], sizeof(ht_temp_meas_ccc), (const uint8_t*)&ht_temp_meas_ccc);
            }
            break;
        case 5:
            po_plx_spot_check_meas_ccc[1] = v1;
            po_plx_spot_check_meas_ccc[0] = v2;
            if(bt_correctly_init == 1){
                esp_ble_gatts_set_attr_value(pulse_oximeter_handle_table[POS_IDX_PLX_SPOT_CCC], sizeof(po_plx_spot_check_meas_ccc), (const uint8_t*)&po_plx_spot_check_meas_ccc);
            }
            break;
        default:
            break;
    }   
}

static void init_objects_size(){

	ht_obj_size_val[2] = (uint8_t)((last_ht_measures_index & 0xFF00) >> 8);
	ht_obj_size_val[3] = (uint8_t)(last_ht_measures_index & 0x00FF);
	ht_obj_size_val[6] = (uint8_t)((last_ht_measures_size & 0xFF00) >> 8);
	ht_obj_size_val[7] = (uint8_t)(last_ht_measures_size & 0x00FF);

	hr_obj_size_val[2] = (uint8_t)((last_hr_measures_index & 0xFF00) >> 8);
	hr_obj_size_val[3] = (uint8_t)(last_hr_measures_index & 0x00FF);
	hr_obj_size_val[6] = (uint8_t)((last_hr_measures_size & 0xFF00) >> 8);
	hr_obj_size_val[7] = (uint8_t)(last_hr_measures_size & 0x00FF);

	po_obj_size_val[2] = (uint8_t)((last_po_measures_index & 0xFF00) >> 8);
	po_obj_size_val[3] = (uint8_t)(last_po_measures_index & 0x00FF);
	po_obj_size_val[6] = (uint8_t)((last_po_measures_size & 0xFF00) >> 8);
	po_obj_size_val[7] = (uint8_t)(last_po_measures_size & 0x00FF);

}

static uint64_t min(uint64_t a, uint64_t b){
    uint64_t val;
    val = (a<b) ? a:b;
    return val;
}

static uint16_t get_bigger_closer_power_of_two(uint16_t value){
	if(value>=64 && value < 128){
		return 128;
	} else if(value>=128 && value < 256){
		return 256;
	} else if(value>=256 && value < 512){
		return 512;
	} else if(value>=512 && value < 1024){
		return 1024;
	} else if(value>=1024 && value < 2048){
		return 2048;
	} else if(value>=2048 && value < 4096){
		return 4096;
	} else if(value>=4096 && value < 8196){
		return 8196;
	}else if(value>=8196 && value < 16384){
		return 16384;
	}else if(value>=16384 && value < 32768){
		return 32768;
	}else if(value>=32768){
		return 32768;
	}
	return 0;
}

static void set_1_8_and_5_voltage_lines(bool activate_or_deactive){
	if(activate_or_deactive){
		gpio_set_level(GPIO_19, 1);
		gpio_set_level(GPIO_14, 1);
	}else{
		gpio_set_level(GPIO_19, 0);
		gpio_set_level(GPIO_14, 0);
	}
	vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void set_1_8_voltage_line(bool activate_or_deactive){
	if(activate_or_deactive){
		gpio_set_level(GPIO_19, 1);
	}else{
		gpio_set_level(GPIO_19, 0);
	}
	vTaskDelay(10 / portTICK_PERIOD_MS);
}

max30102_err_t get_HR_from_FFT(){
	uint8_t lower_index = 0;
	uint8_t higher_index = 0;
	float current_value = 0;
	float sample_freq = get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0);
	float red_peak_value = 0;
	float red_peak_freq = 0;
	int i = 0;

	for(i = 0; i<HR_DELETED_SAMPLES; i++){
		hr_array++;
	}
	kiss_fft(mycfg, hr_array, spo2_red_fft);
	for(i=0; i<hr_array_size; i++){
		current_value = sample_freq*i/hr_array_size;
		if(current_value>=LOWER_FREQ_FFT){
			lower_index = i;
			break;
		}
	}
	for(i=0; i<hr_array_size; i++){
		current_value = sample_freq*i/hr_array_size;
		if(current_value>=UPPER_FREQ_FFT){
			higher_index = i;
			break;
		}
	}

	red_peak_value = sqrt(pow(spo2_red_fft[lower_index].r,2) + pow(spo2_red_fft[lower_index].i,2));
	for(i=lower_index; i<higher_index; i++){
		current_value = sqrt(pow(spo2_red_fft[i].r,2) + pow(spo2_red_fft[i].i,2));
		if(current_value>red_peak_value){
			red_peak_freq = sample_freq*i/hr_array_size;
		}
	}

	beats_per_minute_from_spo2 = (red_peak_freq)*60;
	for(i = 0; i<HR_DELETED_SAMPLES; i++){
		spo2_red_input--;
		hr_array--;
	}
	return MAX30102_OK;
}

max30102_err_t max30102_getSaturation(){
	uint8_t lower_index = 0;
	uint8_t higher_index = 0;
	float current_value = 0;
	float current_value2 = 0;
	float sample_freq = get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0);
	float red_peak_value = 0;
	float ir_peak_value = 0;
	float red_peak_freq = 0;
	float ir_peak_freq = 0;
	float ratio = 0;
	float red_dc = 0;
	float ir_dc = 0;
	int i = 0;

	for(i = 0; i<SPO2_DELETED_SAMPLES; i++){
		spo2_red_input++;
		spo2_ir_input++;
	}
	kiss_fft(mycfg, spo2_red_input, spo2_red_fft);
	kiss_fft(mycfg, spo2_ir_input, spo2_ir_fft);
	for(i=0; i<spo2_array_size; i++){
		current_value = sample_freq*i/spo2_array_size;
		if(current_value>=LOWER_FREQ_FFT){
			lower_index = i;
			break;
		}
	}
	for(i=0; i<spo2_array_size; i++){
		current_value = sample_freq*i/spo2_array_size;
		if(current_value>=UPPER_FREQ_FFT){
			higher_index = i;
			break;
		}
	}
	red_dc = sqrt(pow(spo2_red_fft[0].r,2) + pow(spo2_red_fft[0].i,2));
	ir_dc = sqrt(pow(spo2_ir_fft[0].r,2) + pow(spo2_ir_fft[0].i,2));;
	red_peak_value = sqrt(pow(spo2_red_fft[lower_index].r,2) + pow(spo2_red_fft[lower_index].i,2));
	ir_peak_value = sqrt(pow(spo2_ir_fft[lower_index].r,2) + pow(spo2_ir_fft[lower_index].i,2));
	for(i=lower_index; i<higher_index; i++){
		current_value = sqrt(pow(spo2_red_fft[i].r,2) + pow(spo2_red_fft[i].i,2));
		current_value2 = sqrt(pow(spo2_ir_fft[i].r,2) + pow(spo2_ir_fft[i].i,2));
		if(current_value>red_peak_value){
			red_peak_value = current_value;
			red_peak_freq = sample_freq*i/spo2_array_size;
		}
		if(current_value2>ir_peak_value){
			ir_peak_value = current_value2;
			ir_peak_freq = sample_freq*i/spo2_array_size;
		}
	}

	ratio = (red_peak_value/red_dc)/(ir_peak_value/ir_dc);
	saturation_percentage = 110-(25*ratio)-saturation_percentage_offset;
	if(saturation_percentage>100){ saturation_percentage = 100;};

	beats_per_minute_from_spo2 = (ir_peak_freq+red_peak_freq)/2*60;
	for(i = 0; i<SPO2_DELETED_SAMPLES; i++){
		spo2_red_input--;
		spo2_ir_input--;
	}
	time_since_last_spo2_measure = 0;
	return MAX30102_OK;
}

static void max30102_int_task(void* arg){
	uint8_t temp_1 = 0;
	esp_err_t ret;
	uint8_t gpio36_level;
	uint8_t temp_int = 0;
	uint8_t temp_dec = 0;
	float max30102_temperature = 0;
	uint8_t max30102_operation_mode;

    while(1){
    	if(fall_detected){
    		max30102_power_safe_on();
    		currently_measuring = false;
    		measure_finished_ok = false;
    		measure_finished_wrong = true;
    		vTaskDelete(NULL);
    	}
        gpio36_level = gpio_get_level(36);
        if((gpio36_level == 0) && max30102_intr_detected == 0){
            ESP_LOGI(GATTS_TAG, "INT MISS\n");
            max30102_intr_detected++;
        }

        //When detecting interruptions, the priority is: FIFO almost full; Proximity Threshold Triggered,
        //Internal Temperature Conversion Ready, ALC overflow, PPG_RDY (this one is not used neither implemented)
        if(max30102_intr_detected != 0){
        	if(gpio36_level == 0){
        		max30102_read_intst1(&int_vector1);
        		max30102_read_intst2(&int_vector2);
        	}

        	if(int_vector1>>7 == 1){ //FIFO almost full
        		max30102_operation_mode = max30102_get_mode();
        		currently_measuring = true;

        		if(max30102_operation_mode==2){
        			ret = max30102_multiple_fifo_read(hr_array, NULL, &hr_array_pos_counter, NULL, &hr_samples_added, NULL, (hr_array_size+HR_DELETED_SAMPLES), false);
        			if(ret == MAX30102_HR_OUT_OF_RANGE){
        				if(times_retrying<TIMES_FOR_RETRY){
        					max30102_red_reset_current();
        					hr_array_pos_counter = 0;
        					gettimeofday(&time_starting_measure, NULL);
        					ESP_LOGI(GATTS_TAG, "Excesive samples out of range. Trying to re-set current...\n");
        					ret = max30102_set_current_when_starting_measure();
    						if(ret){
    							max30102_power_safe_on();
    							currently_measuring = false;
    							measure_finished_ok = false;
    							measure_finished_wrong = true;
    							vTaskDelete(NULL);
    						} else {
    							gettimeofday(&time_starting_measure, NULL);
    							times_retrying++;
    						}
        				} else{
        					max30102_red_reset_current();
        					hr_array_pos_counter = 0;
        					max30102_power_safe_on();
							currently_measuring = false;
							measure_finished_ok = false;
							measure_finished_wrong = true;
							vTaskDelete(NULL);
        				}
					}
        			if( (hr_array_pos_counter>=hr_array_size+HR_DELETED_SAMPLES) ){
						currently_measuring = false;
						measure_finished_ok = true;
						measure_finished_wrong = false;
						max30102_power_safe_on();
						vTaskDelete(NULL);
					}
        		} else if (max30102_operation_mode==3){
        			ret = max30102_multiple_fifo_read(spo2_red_input, spo2_ir_input, &hr_array_pos_counter, &spo2_array_pos_counter, &hr_samples_added, &spo2_samples_added, (spo2_array_size+SPO2_DELETED_SAMPLES), false);
        			if(ret == MAX30102_HR_OUT_OF_RANGE || ret == MAX30102_SPO2_OUT_OF_RANGE){
        				if(times_retrying<TIMES_FOR_RETRY){
							max30102_red_reset_current();
							max30102_ir_reset_current();
							hr_array_pos_counter = 0;
							spo2_array_pos_counter = 0;
							gettimeofday(&time_starting_measure, NULL);
							ESP_LOGI(GATTS_TAG, "Excesive samples out of range. Trying to re-set current...\n");
							max30102_change_mode(2);
							ret = max30102_set_current_when_starting_measure();
							if(ret){
								max30102_power_safe_on();
								currently_measuring = false;
								measure_finished_ok = false;
								measure_finished_wrong = true;
								vTaskDelete(NULL);
							} else {
								max30102_change_mode(3);
								ret = max30102_IR_set_current_when_starting_measure();
								if(ret){
									max30102_power_safe_on();
									currently_measuring = false;
									measure_finished_ok = false;
									measure_finished_wrong = true;
									vTaskDelete(NULL);
								} else {
									times_retrying++;
									gettimeofday(&time_starting_measure, NULL);
								}
							}
						} else{
							max30102_red_reset_current();
							max30102_ir_reset_current();
							hr_array_pos_counter = 0;
							spo2_array_pos_counter = 0;
							max30102_power_safe_on();
							currently_measuring = false;
							measure_finished_ok = false;
							measure_finished_wrong = true;
							vTaskDelete(NULL);
						}
					}

        			if( (hr_array_pos_counter>=spo2_array_size+SPO2_DELETED_SAMPLES) && (spo2_array_pos_counter>=spo2_array_size+SPO2_DELETED_SAMPLES) ){
						max30102_measure_temp();
					}
        		} else {
        			printf("Unknown mode: %d \n", max30102_operation_mode);
        		}
			}

			temp_1 = int_vector1<<2;
			temp_1 = temp_1>>7;
			if(temp_1 == 1){ //Proximity Threshold Triggered
				ESP_LOGI(GATTS_TAG, "PROX OBJECT!!\n");
			}

			if(int_vector2>>1 == 1){ //Internal Temperature Conversion Ready
				max30102_read_die_temp_integer(&temp_int);
				max30102_read_die_temp_fraction(&temp_dec);
				temp_int = (int8_t) temp_int;
				max30102_temperature = temp_int + (temp_dec*0.0625);
				if(max30102_temperature>=38 && max30102_temperature<40){
					saturation_percentage_offset = 1;
				} else if(max30102_temperature>=40){
					saturation_percentage_offset = 2;
				}
				if( (hr_array_pos_counter>=spo2_array_size+SPO2_DELETED_SAMPLES) && (spo2_array_pos_counter>=spo2_array_size+SPO2_DELETED_SAMPLES) ){
					currently_measuring = false;
					measure_finished_ok = true;
					measure_finished_wrong = false;
					max30102_power_safe_on();
					vTaskDelete(NULL);
				}
			}

			temp_1 = int_vector1<<2;
			temp_1 = temp_1>>7;
			if(temp_1==1){ //ALC overflow
				ESP_LOGI(GATTS_TAG, "ALC OVFLW!!\n");
			}
			if(int_vector1<<7 == 128){
				ESP_LOGI(GATTS_TAG, "PWR RDY!\n");
			}

			max30102_intr_detected--;
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

static void IRAM_ATTR MAX30102_isr_handler(void* arg){
	max30102_intr_detected++;
}

static esp_err_t max30102_enable_interruptions(){
    esp_err_t ret;
    gpio_config_t io_conf;
    
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_MAX30102_INPUT_MASK;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    
    ret = gpio_config(&io_conf);
    if(ret){ return ret;}

    ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    if(ret){ return ret;}
    
    ret = gpio_isr_handler_add(MAX30102_INT_PIN, MAX30102_isr_handler, (void*) MAX30102_INT_PIN);
    if(ret){ return ret;}

    return ret;
}

static max30102_err_t max30102_start_measuring (uint8_t mode, uint8_t pw, uint8_t sample_rate, uint8_t averaging){
	max30102_err_t ret = 0;
	esp_err_t ret2;

	set_1_8_and_5_voltage_lines(true);
	max30102_change_mode(mode);
	max30102_change_spo2_led_pulse_width(pw);
	max30102_change_spo2_sample_rate(sample_rate);
	max30102_change_sample_averaging(averaging);
	max30102_enable_interruptions();

	if(mode == 2){
		hr_array_size = get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0)*SECONDS_MEASURE_HR;
		hr_array_size = get_bigger_closer_power_of_two(hr_array_size);
		hr_array = (kiss_fft_cpx *)calloc(hr_array_size+HR_DELETED_SAMPLES, sizeof(kiss_fft_cpx));
		filtered_signal = (float *)calloc(hr_array_size, sizeof(float));
		mycfg = kiss_fft_alloc(hr_array_size, 0, NULL, NULL);
		spo2_red_fft = (kiss_fft_cpx *)calloc(hr_array_size, sizeof(kiss_fft_cpx));
		float* hr_array_floats = (float *)calloc(hr_array_size, sizeof(float));

		if((hr_array == NULL) || (filtered_signal == NULL) || (hr_array_floats == NULL) || (mycfg == NULL) || (spo2_red_fft == NULL)){
			printf("ERR allocating mem for HR array\n");
			if(hr_array){free(hr_array);}
			if(filtered_signal){free(filtered_signal);}
			if(hr_array_floats){free(hr_array_floats);}
			if(spo2_red_fft){free(spo2_red_fft);}
			if(mycfg){free(mycfg);}
		} else {
			struct timeval current_measure_time;
			uint64_t hr_measuring_time_ms = 0;
			bool previous_currently_measuring_state = false;
			int samplesProcessed = 0;
			beats_per_minute = 0;
			hr_variability = 0; //Result is in ms

			max30102_change_mode(2);
			ret = max30102_set_current_when_starting_measure();
			if(!ret){
				xTaskCreate(max30102_int_task, "max30102_task", 4096, NULL, 10, &max30102_int_handle);
				times_retrying = 0;
				gettimeofday(&time_starting_measure, NULL);
				while(hr_measuring_time_ms<MAX_TIME_MEASURING_MS){
					gettimeofday(&current_measure_time, NULL);
					hr_measuring_time_ms = (current_measure_time.tv_sec - time_starting_measure.tv_sec) * 1000 + (current_measure_time.tv_usec - time_starting_measure.tv_usec) / 1000;
					if( (!previous_currently_measuring_state) && measure_finished_ok){
						break;
					}
					if( (!previous_currently_measuring_state) && measure_finished_wrong){
						break;
					}
					previous_currently_measuring_state = currently_measuring;
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}

				max30102_power_safe_on();
				set_1_8_and_5_voltage_lines(false);

				if(hr_array_pos_counter>= hr_array_size){
					first_hr_element = hr_array_floats;
					for(int i = 0; i<hr_array_size; i++){
						*(hr_array_floats++) = hr_array[i].r;
					}
					hr_array_floats -= hr_array_size;

					first_filtered_element = filtered_signal;
					iirfilter *filteriir = iirfilter_create(); // Create an instance of the filter
					iirfilter_reset(filteriir);
					samplesProcessed = iirfilter_filterInChunks(filteriir, first_hr_element, first_filtered_element, hr_array_size);  //Learning of the filter
					samplesProcessed = iirfilter_filterInChunks(filteriir, first_hr_element, first_filtered_element, hr_array_size);  // Filter the input test signal
					iirfilter_destroy(filteriir);
					beats_per_minute = max30102_calculateBeats(first_filtered_element, samplesProcessed, &hr_variability);

					if(beats_per_minute<BPM_CORRECT_TH){
						get_HR_from_FFT();
						beats_per_minute = (uint8_t) round(beats_per_minute_from_spo2);
						if(beats_per_minute>BPM_CORRECT_TH_UP){
							beats_per_minute = 0;
						}
						hr_variability = 0;
					}
					save_hr_and_variability_into_bt_array(beats_per_minute, hr_variability);
					save_hr_and_variability_in_RTC_and_increment_object_size(beats_per_minute, hr_variability);
					time_since_last_hr_measure = 0;
					ret = MAX30102_OK;
				} else {
					ret = MAX30102_HR_NOT_PERFORMED;
				}
				currently_measuring = false;
				previous_currently_measuring_state = false;
				measure_finished_ok = false;
				measure_finished_wrong = false;
				time_since_last_hr_measure = 0;
			} else {
				printf("Unable to set current. HR measure skipped...\n");
				max30102_power_safe_on();
				set_1_8_and_5_voltage_lines(false);
				ret = MAX30102_HR_NOT_PERFORMED;
			}
			free(hr_array);
			free(filtered_signal);
			free(mycfg);
			free(spo2_red_fft);
			free(hr_array_floats);
		}

	} else if(mode == 3){
		spo2_array_size = get_MAX30102_sample_rate(0)/get_MAX30102_sample_averaging(0)*SECONDS_MEASURE_SPO2;
		spo2_array_size = get_bigger_closer_power_of_two(spo2_array_size);
		spo2_red_input = (kiss_fft_cpx *)calloc(spo2_array_size+SPO2_DELETED_SAMPLES, sizeof(kiss_fft_cpx));
		spo2_ir_input = (kiss_fft_cpx *)calloc(spo2_array_size+SPO2_DELETED_SAMPLES, sizeof(kiss_fft_cpx));
		mycfg = kiss_fft_alloc(spo2_array_size, 0, NULL, NULL);
		spo2_red_fft = (kiss_fft_cpx *)calloc(spo2_array_size, sizeof(kiss_fft_cpx));
		spo2_ir_fft = (kiss_fft_cpx *)calloc(spo2_array_size, sizeof(kiss_fft_cpx));
		if((spo2_red_input == NULL) || (spo2_ir_input == NULL) || (spo2_red_fft == NULL) || (spo2_ir_fft == NULL) || (mycfg == NULL) ){
			printf("ERR allocating mem for SPO2 arrays\n");
			if(spo2_red_input){ free(spo2_red_input);}
			if(spo2_ir_input){ free(spo2_ir_input);}
			if(spo2_red_fft){ free(spo2_red_fft);}
			if(spo2_ir_fft){ free(spo2_ir_fft);}
			if(mycfg){ free(mycfg);}
			ret = MAX30102_ERROR_ALLOC_MEM;
		} else {
			struct timeval current_measure_time;
			uint64_t sp02_measuring_time_ms = 0;
			bool previous_currently_measuring_state = false;

			max30102_change_mode(2);
			ret = max30102_set_current_when_starting_measure();
			if(ret){
				printf("Unable to set current. SPO2 measure skipped...\n");
				max30102_power_safe_on();
				set_1_8_and_5_voltage_lines(false);
				ret =  MAX30102_SPO2_NOT_PERFORMED;
			} else {
				max30102_change_mode(3);
				ret2 = max30102_IR_set_current_when_starting_measure();

				if(!ret && !ret2){
					xTaskCreate(max30102_int_task, "max30102_task", 4096, NULL, 10, &max30102_int_handle);
					times_retrying = 0;
					gettimeofday(&time_starting_measure, NULL);

					while(sp02_measuring_time_ms<MAX_TIME_MEASURING_MS){
						gettimeofday(&current_measure_time, NULL);
						sp02_measuring_time_ms = (current_measure_time.tv_sec - time_starting_measure.tv_sec) * 1000 + (current_measure_time.tv_usec - time_starting_measure.tv_usec) / 1000;
						if( (!previous_currently_measuring_state) && measure_finished_ok){
							break;
						}
						if( (!previous_currently_measuring_state) && measure_finished_wrong){
							break;
						}
						previous_currently_measuring_state = currently_measuring;
						vTaskDelay(10 / portTICK_PERIOD_MS);
					}

					max30102_power_safe_on();
					free(spo2_ir_input);
					free(spo2_red_fft);
					free(spo2_ir_fft);
					free(mycfg);
					if((hr_array_pos_counter>= (spo2_array_size+SPO2_DELETED_SAMPLES)) && (spo2_array_pos_counter>= (spo2_array_size+SPO2_DELETED_SAMPLES))){
						ret = max30102_getSaturation();
						if(!ret){
							save_spo2_in_RTC_and_increment_object_size(round(saturation_percentage));
							save_spo2_into_bt_array(round(saturation_percentage));
						}

						if(time_since_last_hr_measure >= hr_time){
							ESP_LOGI(GATTS_TAG, "SpO2 was requested. Measuring also HR\n");
							hr_array_spo2 = (float *)calloc(spo2_array_size, sizeof(float));
							filtered_signal = (float *)calloc(spo2_array_size, sizeof(float));
							if(hr_array_spo2 == NULL || filtered_signal == NULL){
								printf("ERR allocating mem for filtered array\n");
								if(hr_array_spo2){free(hr_array_spo2);}
							} else {
								first_hr_element = hr_array_spo2;
								for(int i = 0; i<spo2_array_size; i++){
									*(hr_array_spo2++) = spo2_red_input[i].r;
								}
								hr_array_spo2 -= spo2_array_size;

								beats_per_minute = 0;
								hr_variability = 0;
								int samplesProcessed = 0;
								first_filtered_element = filtered_signal;

								iirfilter *filteriir = iirfilter_create(); // Create an instance of the filter
								iirfilter_reset(filteriir);
								samplesProcessed = iirfilter_filterInChunks(filteriir, first_hr_element, first_filtered_element, spo2_array_size);    //Learning of the filter
								samplesProcessed = iirfilter_filterInChunks(filteriir, first_hr_element, first_filtered_element, spo2_array_size);    // Filter the input test signal
								iirfilter_destroy(filteriir);
								beats_per_minute = max30102_calculateBeats(first_filtered_element, samplesProcessed, &hr_variability);

								if(abs(beats_per_minute_from_spo2-beats_per_minute)<=MAX_DIFF_HR_TIME_FREQ){
									beats_per_minute= round((beats_per_minute+beats_per_minute_from_spo2)/2);
								}
								if(beats_per_minute<BPM_CORRECT_TH){
									if((beats_per_minute_from_spo2<=BPM_CORRECT_TH_UP) && (beats_per_minute_from_spo2>=BPM_CORRECT_TH)){
										beats_per_minute = beats_per_minute_from_spo2;
									}
									beats_per_minute = 0;
									hr_variability = 0;
									ret = MAX30102_SPO2_OK_HR_FAIL;
								}
								save_hr_and_variability_into_bt_array(beats_per_minute, hr_variability);
								save_hr_and_variability_in_RTC_and_increment_object_size(beats_per_minute, hr_variability);
								time_since_last_hr_measure = 0;
							}
							free(hr_array_spo2);
							free(filtered_signal);
						}
					} else {
						ret = MAX30102_SPO2_NOT_PERFORMED;
					}
				} else {
					printf("Unable to set current. SPO2 measure skipped...\n");
					ret = MAX30102_SPO2_NOT_PERFORMED;
				}
			}
		}
		time_since_last_spo2_measure = 0;
		max30102_power_safe_on();
		if(spo2_red_input){
			free(spo2_red_input);
		}
	} else {
		ret =  MAX30102_FAIL;
	}
	set_1_8_and_5_voltage_lines(false);
	currently_measuring = false;
	measure_finished_ok = false;
	measure_finished_wrong = false;
	hr_array_pos_counter = 0;
	spo2_array_pos_counter = 0;
	hr_array_size = 0;
	spo2_array_size = 0;
	return ret;
}

static void save_falls_into_bt_array(uint8_t kind_of_fall){

	fs_fall_detected_val[0]= kind_of_fall;

	if(bt_correctly_init == 1 ){
		esp_ble_gatts_set_attr_value(fall_handle_table[FS_FALL_DETECTED_VAL], sizeof(fs_fall_detected_val), (const uint8_t*) &fs_fall_detected_val);
	}
}

static void save_steps_into_bt_array(uint32_t steps){
	char temporal[4];

	memcpy(temporal, (unsigned char*) (&steps), 4);

	ss_steps_number_val[0]= temporal[3];
	ss_steps_number_val[1]= temporal[2];
	ss_steps_number_val[2]= temporal[1];
	ss_steps_number_val[3]= temporal[0];

	if(bt_correctly_init == 1 ){
		esp_ble_gatts_set_attr_value(steps_handle_table[SS_STEPS_NUMBER_VAL], sizeof(ss_steps_number_val), (const uint8_t*) &ss_steps_number_val);
	}
}

static void lsm6ds3_int_task(void* arg){
	esp_err_t ret;
	uint8_t gpio35_level;
	uint8_t wakeup_reg = 0;
	uint8_t tap_reg = 0;
	uint8_t func_reg = 0;
	uint8_t d6d_reg = 0;
	uint16_t steps;

    while(1){
        gpio35_level = gpio_get_level(35);
        if((gpio35_level == 0) && lsm6ds3_intr_detected == 0){
            ESP_LOGI(GATTS_TAG, "INT MISS\n");
        }

        if(lsm6ds3_intr_detected != 0){
        	if((ret = lsm6ds3_get_wake_up_src(&wakeup_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //free fall =  FF_IA (MSB = 7)
				if((wakeup_reg>>5)){
					ESP_LOGI(GATTS_TAG,"FALL DETECTED\n");
					ret = lsm6ds3_run_falls_algorithm();
					ESP_LOGI(GATTS_TAG, "Fall order = %d\n", ret);
					if(ret== LSM6DS3_FALSE_ALARM){
						break;
					}
					save_falls_into_bt_array(ret);
					fall_detected = true;
				} else {
					printf("WAKEUP %d\n", wakeup_reg);
				}

				if((ret = lsm6ds3_get_tap_src(&tap_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //double-tap = bit 5
				if((tap_reg & 16)){
					ESP_LOGI(GATTS_TAG,"DOUBLE TAP\n");
					save_falls_into_bt_array(LSM6DS3_FALL_NOTIFIED_BY_PATIENT);
					fall_detected = true;
				} else {
					printf("TAP %d\n", tap_reg);
				}

				if((ret = lsm6ds3_get_func_src(&func_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //Step overflow = bit 3
				if((func_reg & 8)){
					ESP_LOGI(GATTS_TAG,"STEPS_OVERFLOW\n");
					lsm6ds3_get_FIFO_step_counter(&steps);
					lsm6ds3_accumulated_steps = lsm6ds3_accumulated_steps+steps;
					save_steps_into_bt_array(lsm6ds3_accumulated_steps);
				} else {
					printf("FUNC %d\n", func_reg);
				}
				if((ret = lsm6ds3_get_d6d_src(&d6d_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //Read only in case that register causes the interruption
			lsm6ds3_intr_detected--;
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

static void IRAM_ATTR lsm6ds3_isr_handler(void* arg){
	lsm6ds3_intr_detected++;
}

static esp_err_t lsm6ds3_enable_interruptions(){
    esp_err_t ret;
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_LSM6DS3_INPUT_MASK;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    ret = gpio_config(&io_conf);
    if(ret){ return ret;}

    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if(ret){ return ret;}

    ret = gpio_isr_handler_add(LSM6DS3_INT2, lsm6ds3_isr_handler, (void*) LSM6DS3_INT2);
    if(ret){ return ret;}

    return ret;
}

static void init_last_measures_arrays(){
    int i = 0;

    for(i=0; i<HT_STORED_SAMPLES; i++){
        last_ht_measures[i] = 0;
    }
    for(i=0; i<HR_STORED_SAMPLES; i++){
        last_hr_measures[i] = 0;
    }
    for(i=0; i<PO_STORED_SAMPLES; i++){
        last_po_measures[i] = 0;
    }
}

static void save_temp_in_RTC_and_increment_object_size(uint8_t sensor_data_h, uint8_t sensor_data_l){
    last_ht_measures[last_ht_measures_index] = convert_temp(sensor_data_h, sensor_data_l);
    last_ht_measures_index++;
    last_ht_measures_size++;

    ht_obj_size_val[3] = (last_ht_measures_index & 0x000000ff);
    ht_obj_size_val[2] = (last_ht_measures_index & 0x0000ff00) >> 8;
    ht_obj_size_val[1] = (last_ht_measures_index & 0x00ff0000) >> 16;
    ht_obj_size_val[0] = (last_ht_measures_index & 0xff000000) >> 24;

    ht_obj_size_val[7] = (last_ht_measures_size & 0x000000ff);
    ht_obj_size_val[6] = (last_ht_measures_size & 0x0000ff00) >> 8;
    ht_obj_size_val[5] = (last_ht_measures_size & 0x00ff0000) >> 16;
    ht_obj_size_val[4] = (last_ht_measures_size & 0xff000000) >> 24;

    if(last_ht_measures_index>=HT_STORED_SAMPLES){
        last_ht_measures_index = 0;
    }

    if(last_ht_measures_size>=HT_STORED_SAMPLES){
		last_ht_measures_size = HT_STORED_SAMPLES;
	}

    if(bt_correctly_init == 1){
        esp_ble_gatts_set_attr_value(health_thermometer_handle_table[HTS_IDX_HT_OBJ_SIZE_CHAR], sizeof(ht_obj_size_val), (const uint8_t*)&ht_obj_size_val);
    }
}

static void save_hr_and_variability_in_RTC_and_increment_object_size(uint8_t beats, float variancee){
    last_hr_measures[last_hr_measures_index] = beats;
    last_hr_measures_index++;
    last_hr_measures_size++;

    if(last_hr_measures_index>=HR_STORED_SAMPLES){
        last_hr_measures_index = 0;
    }

    if(last_hr_measures_size>=HR_STORED_SAMPLES){
    	last_hr_measures_size = HR_STORED_SAMPLES;
    }

    last_hr_variability_measures[last_hr_variability_measures_index] = variancee;
    last_hr_variability_measures_index++;
    last_hr_variability_measures_size++;

    if(last_hr_variability_measures_index>=HR_STORED_SAMPLES){
    	last_hr_variability_measures_index = 0;
    }

    if(last_hr_variability_measures_size>=HR_STORED_SAMPLES){
		last_hr_variability_measures_size = HR_STORED_SAMPLES;
	}

    hr_obj_size_val[3] = (last_hr_measures_index & 0x000000ff);
    hr_obj_size_val[2] = (last_hr_measures_index & 0x0000ff00) >> 8;
    hr_obj_size_val[1] = (last_hr_measures_index & 0x00ff0000) >> 16;
    hr_obj_size_val[0] = (last_hr_measures_index & 0xff000000) >> 24;

    hr_obj_size_val[7] = (last_hr_measures_size & 0x000000ff);
	hr_obj_size_val[6] = (last_hr_measures_size & 0x0000ff00) >> 8;
	hr_obj_size_val[5] = (last_hr_measures_size & 0x00ff0000) >> 16;
	hr_obj_size_val[4] = (last_hr_measures_size & 0xff000000) >> 24;

    if(bt_correctly_init == 1){
        esp_ble_gatts_set_attr_value(heart_rate_handle_table[HRS_IDX_HR_OBJ_SIZE_CHAR], sizeof(hr_obj_size_val), (const uint8_t*)&hr_obj_size_val);
    }
}

static void save_spo2_in_RTC_and_increment_object_size(uint8_t saturation_value){
    last_po_measures[last_po_measures_index] = saturation_value;
    last_po_measures_index++;
    last_po_measures_size++;

    if(last_po_measures_index>=PO_STORED_SAMPLES){
        last_po_measures_index = 0;
    }

    if(last_po_measures_size>=PO_STORED_SAMPLES){
		last_po_measures_size = PO_STORED_SAMPLES;
    }

    po_obj_size_val[3] = (last_po_measures_index & 0x000000ff);
	po_obj_size_val[2] = (last_po_measures_index & 0x0000ff00) >> 8;
	po_obj_size_val[1] = (last_po_measures_index & 0x00ff0000) >> 16;
    po_obj_size_val[0] = (last_po_measures_index & 0xff000000) >> 24;

    po_obj_size_val[7] = (last_po_measures_size & 0x000000ff);
	po_obj_size_val[6] = (last_po_measures_size & 0x0000ff00) >> 8;
	po_obj_size_val[5] = (last_po_measures_size & 0x00ff0000) >> 16;
	po_obj_size_val[4] = (last_po_measures_size & 0xff000000) >> 24;

    if(bt_correctly_init == 1){
        esp_ble_gatts_set_attr_value(heart_rate_handle_table[POS_IDX_PO_OBJ_SIZE_CHAR], sizeof(po_obj_size_val), (const uint8_t*)&po_obj_size_val);
    }
}

static void configure_deep_sleep_and_sleep(uint64_t time_to_sleep){
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_timer_wakeup(time_to_sleep);
    esp_deep_sleep_start();
}

static void config_gpios(){
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
}

void app_main(){

    esp_err_t ret;
    uint8_t config_value = 0x01; //Refer to MAX30205 datasheet to understand each value of this byte
    uint8_t config_received;
    uint64_t time_to_sleep;
    nvs_handle my_handle;

    int new_data = 0;
    uint64_t battery_level;
    uint8_t sensor_data_h, sensor_data_l;
    uint8_t wakeup_reg = 0;
	uint8_t tap_reg = 0;
	uint8_t func_reg = 0;
	uint8_t d6d_reg = 0;
	uint16_t steps;

    /* Init NVS, I2C, ADC and MAX30102 interruptions*/
    if((ret = nvs_flash_init())) {printf("Error (%d) initializing NVS.\n", ret);}
    if((ret = nvs_open("storage", NVS_READWRITE, &my_handle))) {printf("Error (%d) opening NVS.\n", ret);}
    if((ret = i2c_master_init())) {printf("Failed initializing the i2c port. Error %d \n", ret);}
    if((ret = adc_init())) {printf("Failed initializing ADC. Error %d \n", ret);}
    print_mux = xSemaphoreCreateMutex();
    init_objects_size();
    config_gpios();

    if((ret = adc_init())) {printf("Error (%d) initializing ADC.\n", ret);}
    /* If battery level is under threshold, don't start, enter in deep_sleep immediately */
    battery_level = check_battery_level();
    bs_bat_level_val[0] = battery_level;
	if(battery_level<=ADC_SHUTDOWN_LEVEL){
	   ESP_LOGI(GATTS_TAG, "Battery too low (%d%%). Going to sleep now. Waking up again in %llu us\n", bs_bat_level_val[0], check_battery_time_when_battery_is_low_us);
	   esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
	   esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
	   esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	   esp_sleep_enable_timer_wakeup(check_battery_time_when_battery_is_low_us);
	   gettimeofday(&sleep_enter_time, NULL);
	   esp_deep_sleep_start();
	}

    switch (esp_sleep_get_wakeup_cause()) {

		case ESP_SLEEP_WAKEUP_EXT1:{
			if((ret = lsm6ds3_get_wake_up_src(&wakeup_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //free fall =  FF_IA (MSB = 7)
			if((wakeup_reg>>5)){
				ESP_LOGI(GATTS_TAG,"FALL DETECTED\n");
				ret = lsm6ds3_run_falls_algorithm();
				ESP_LOGI(GATTS_TAG, "Fall order = %d\n", ret);
				if(ret== LSM6DS3_FALSE_ALARM){
					break;
				}
				save_falls_into_bt_array(ret);
				ret = bluetooth_init();
				if(ret){
					printf("Failed initializing Bluetooth. Error %d \n", ret);
				} else {
					ESP_LOGI(GATTS_TAG, "Bluetooth init OK.\n");
					check_for_BS_reception();
				}
			} else {
				printf("WAKEUP %d\n", wakeup_reg);
			}

			if((ret = lsm6ds3_get_tap_src(&tap_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //double-tap = bit 5
			printf("TAP_REG = %d\n", tap_reg);
			if((tap_reg & 16)){
				ESP_LOGI(GATTS_TAG,"DOUBLE TAP\n");
				save_falls_into_bt_array(LSM6DS3_FALL_NOTIFIED_BY_PATIENT);
				ret = bluetooth_init();
				if(ret){
					printf("Failed initializing Bluetooth. Error %d \n", ret);
				} else {
					ESP_LOGI(GATTS_TAG, "Bluetooth init OK.\n");
					check_for_BS_reception();
				}

			} else {
				printf("TAP %d\n", tap_reg);
			}

			if((ret = lsm6ds3_get_func_src(&func_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //Step overflow = bit 3
			if((func_reg & 8)){
				ESP_LOGI(GATTS_TAG,"STEPS_OVERFLOW\n");
				lsm6ds3_get_FIFO_step_counter(&steps);
				lsm6ds3_accumulated_steps = lsm6ds3_accumulated_steps+steps;
				save_steps_into_bt_array(lsm6ds3_accumulated_steps);
			} else {
				printf("FUNC %d\n", func_reg);
			}

			if((ret = lsm6ds3_get_d6d_src(&d6d_reg))){ESP_LOGI(GATTS_TAG, "ERR R INT V: %d\n", ret);} //Read only in case that register causes the interruption
			break;
		}

		default:{
			break;
		}

	}

    if(running == 0){ //If it is the first time the program executes, then...
    	time_since_last_hr_measure = hr_time;
    	time_since_last_ht_measure = temp_time;
    	time_since_last_spo2_measure = spo2_time;

        gettimeofday(&sleep_enter_time, NULL);

        /* Initialize the MAX30205, MAX30102 and LSM6DS3. Save NVS variables in order to store them for the first time */
        set_1_8_voltage_line(true);
        if ((ret = max30205_init(config_value, &config_received))) {printf("Failed initializing MAX30205. Error %d \n", ret);}
        else {max30205_correctly_initialized = 1;}
        if ((ret = max30102_init())) {printf("Failed initializing MAX30102. Error %d \n", ret);}
        else{max30102_correctly_initialized = 1; ESP_LOGI(GATTS_TAG, "MAX30102 correctly initialized\n");}
        if ((ret = lsm6ds3_init())) {printf("Failed initializing LSM. Error %d \n", ret);}
        else {lsm6ds3_correctly_initialized = 1;}
        init_last_measures_arrays();

        /* Performs the first temperature lecture and store it in the BT temp array. If everything goes fine, reset temperature timer */

        if(lsm6ds3_correctly_initialized == 1){
			lsm6ds3_enable_interruptions();
			xTaskCreate(lsm6ds3_int_task, "lsm6ds3_task", 4096, NULL, 10, &lsm6ds3_int_handle);
			lsm6ds3_get_FIFO_step_counter(&steps);
			lsm6ds3_reset_pedometer(true);
			lsm6ds3_accumulated_steps = lsm6ds3_accumulated_steps+steps;
			save_steps_into_bt_array(lsm6ds3_accumulated_steps);
		}

        if(max30205_correctly_initialized == 1){
            if((ret = max30205_one_shot_and_read(&sensor_data_h, &sensor_data_l))) {
                printf("Error measuring temperature. Error: %d\n", ret);
            } else {
				save_temp_in_RTC_and_increment_object_size(sensor_data_h, sensor_data_l);
				save_temp_into_bt_array(sensor_data_h, sensor_data_l);
				new_data = 1;
            }
        } else{
            if ((ret = max30205_init(config_value, &config_received))) {
            	printf("Failed initializing MAX30205. Error %d \n", ret);
            }
            else {
                max30205_correctly_initialized = 1;
                if((ret = max30205_one_shot_and_read(&sensor_data_h, &sensor_data_l))) {
                    printf("Error measuring temperature. Error: %d\n", ret);
                } else {
					save_temp_in_RTC_and_increment_object_size(sensor_data_h, sensor_data_l);
					save_temp_into_bt_array(sensor_data_h, sensor_data_l);
					new_data = 1;
                }
            }
        }
    	gettimeofday(&time_last_ht_measure, NULL);
    	time_since_last_ht_measure = 0;

        /* Performs the first HR and SPO2 lecture and store it in the BT HR or SPO2 array. If everything goes fine, reset timers (made internally by max30102_start_measuring) */

        if(max30102_correctly_initialized == 1){
        	ret = max30102_start_measuring(3, 2, 4, 1);
        	if(!ret){new_data=1;}
        } else {
            if ((ret = max30102_init())) {
            	printf("Failed initializing MAX30102. Error %d \n", ret);
            } else {
                max30102_correctly_initialized = 1;
                ret = max30102_start_measuring(3, 2, 4, 1);
                if(!ret){new_data=1;}
            }
        }

    	gettimeofday(&time_last_hr_measure, NULL);
    	time_since_last_hr_measure = 0;
    	gettimeofday(&time_last_po_measure, NULL);
    	time_since_last_spo2_measure = 0;

    	if(new_data){
            ret = bluetooth_init();
            if(ret){
                printf("Failed initializing Bluetooth. Error %d \n", ret);
            } else {
                ESP_LOGI(GATTS_TAG, "Bluetooth init OK.\n");
                check_for_BS_reception(); //BS packet is the last BT transactions performed. Receive it means that all previous values were sent/received.
            }
    	}

        running = 1;
        get_times_since_last_measures();
        time_to_sleep = get_time_to_sleep();
        gettimeofday(&sleep_enter_time, NULL);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        esp_sleep_enable_ext1_wakeup(GPIO_LSM6DS3_INPUT_MASK, ESP_EXT1_WAKEUP_ALL_LOW);
        configure_deep_sleep_and_sleep(time_to_sleep);

    } else { //if running != 0

    	if(!lsm6ds3_correctly_initialized){
    		if ((ret = lsm6ds3_init())) {printf("Failed initializing LSM6DS3. Error %d \n", ret);}
    		else {
    			lsm6ds3_correctly_initialized = 1;
    			lsm6ds3_enable_interruptions();
    			xTaskCreate(lsm6ds3_int_task, "lsm6ds3_task", 4096, NULL, 10, &lsm6ds3_int_handle);
    		}
		} else {
			lsm6ds3_enable_interruptions();
			xTaskCreate(lsm6ds3_int_task, "lsm6ds3_task", 4096, NULL, 10, &lsm6ds3_int_handle);
		}

    	set_1_8_voltage_line(true);
		if ((ret = max30102_init())) {printf("Failed initializing MAX30102. Error %d \n", ret);}
		else{max30102_correctly_initialized = 1; ESP_LOGI(GATTS_TAG, "MAX30102 correctly initialized\n");}

    	if(!max30102_correctly_initialized){
    		if ((ret = max30102_init())) {printf("Failed initializing MAX30102. Error %d \n", ret);}
    		else{max30102_correctly_initialized = 1;}
    	}

    	if(last_ht_measures_index){
    		save_temp_into_bt_array_from_float(last_ht_measures[last_ht_measures_index]);
    	}

    	if(last_hr_measures_index){
    		save_hr_and_variability_into_bt_array(last_hr_measures[last_hr_measures_index], last_hr_variability_measures[last_hr_variability_measures_index]);
    	}

    	if(last_po_measures_index){
    		save_spo2_into_bt_array(last_po_measures[last_po_measures_index]);
    	}

    }

    get_times_since_last_measures();

    convert_hex_measure_intervals_to_time_in_useconds(0);

    if(time_since_last_spo2_measure>=(spo2_time/1000) && time_since_last_hr_measure>=(hr_time/1000)){
        ESP_LOGI(GATTS_TAG, "TIME TO MEASURE HR & SPO2.\n");
        if(max30102_correctly_initialized == 1){
        	ret = max30102_start_measuring(3, 2, 4, 1);
			if(ret == MAX30102_OK){
				new_data = 1;
			}
		}
    	gettimeofday(&time_last_hr_measure, NULL);
    	time_since_last_hr_measure = 0;
    	gettimeofday(&time_last_po_measure, NULL);
        time_since_last_spo2_measure = 0;
    }

    if(time_since_last_spo2_measure>=(spo2_time/1000) && time_since_last_hr_measure<(hr_time/1000)){
        ESP_LOGI(GATTS_TAG, "TIME TO MEASURE SPO2.\n");
        if(max30102_correctly_initialized == 1){
        	ret = max30102_start_measuring(2, 2, 4, 1);
			if(ret == MAX30102_OK){
				new_data = 1;
			}
		}
    	gettimeofday(&time_last_po_measure, NULL);
        time_since_last_spo2_measure = 0;
    }

    if(time_since_last_hr_measure>=(hr_time/1000) && time_since_last_spo2_measure<(spo2_time/1000)){
        ESP_LOGI(GATTS_TAG, "TIME TO MEASURE HR.\n");
        if(max30102_correctly_initialized == 1){
        	ret = max30102_start_measuring(2, 2, 4, 1);
        	if(ret == MAX30102_OK){
        		new_data = 1;
        	}
        }
    	gettimeofday(&time_last_hr_measure, NULL);
    	time_since_last_hr_measure = 0;
    }

    if(time_since_last_ht_measure>=(temp_time/1000)){
        ESP_LOGI(GATTS_TAG, "TIME TO MEASURE TEMP.\n");
        if(max30205_correctly_initialized){
        	if((ret = max30205_one_shot_and_read(&sensor_data_h, &sensor_data_l))) {
				printf("Error measuring temperature. Error: %d\n", ret);
			} else {
				save_temp_in_RTC_and_increment_object_size(sensor_data_h, sensor_data_l);
				save_temp_into_bt_array(sensor_data_h, sensor_data_l);
				new_data = 1;
			}
        }
    	gettimeofday(&time_last_ht_measure, NULL);
    	time_since_last_ht_measure = 0;
    }

    if(new_data){
        if (running != 0){
        	lsm6ds3_get_FIFO_step_counter(&steps);
			lsm6ds3_reset_pedometer(true);
			lsm6ds3_accumulated_steps = lsm6ds3_accumulated_steps+steps;
			save_steps_into_bt_array(lsm6ds3_accumulated_steps);
            ret = bluetooth_init();
            if(ret){
                printf("Failed initializing Bluetooth. Error %d \n", ret);
            } else {
                ESP_LOGI(GATTS_TAG, "Bluetooth init OK.\n");
                check_for_BS_reception();
                new_data = 0;
            }
        }
    }

    if (running == 0){
        running++;
        if(running >= 255){running = 1;}
    }

    get_times_since_last_measures();
    time_to_sleep = get_time_to_sleep();

    gettimeofday(&sleep_enter_time, NULL);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    esp_sleep_enable_ext1_wakeup(GPIO_LSM6DS3_INPUT_MASK, ESP_EXT1_WAKEUP_ALL_LOW);
    configure_deep_sleep_and_sleep(time_to_sleep);
}
