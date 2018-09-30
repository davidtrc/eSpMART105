// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#define HRPS_HT_MEAS_MAX_LEN            (100)

#define HRPS_MANDATORY_MASK             (0x0F)
#define HRPS_BODY_SENSOR_LOC_MASK       (0x30)
#define HRPS_HR_CTNL_PT_MASK            (0xC0)

///Attributes State Machine

enum{
    HRS_IDX_SVC,

    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_CCC,

    HRS_IDX_BODY_SENSOR_LOC_CHAR,
    HRS_IDX_BODY_SENSOR_LOC_VAL,

    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,

	HRS_IDX_HR_VARIABILITY_CHAR,
	HRS_IDX_HR_VARIABILITY_VAL,

    HRS_IDX_HR_OBJ_SIZE_CHAR,
    HRS_IDX_HR_OBJ_SIZE_VAL,

    HRS_IDX_NB,
};

enum {
	HTS_IDX_SVC,

    HTS_IDX_HT_MEAS_CHAR,
    HTS_IDX_MEAS_VAL,
    HTS_IDX_HT_MEAS_CCC,

    HTS_IDX_TEMP_TYPE_CHAR,
    HTS_IDX_TEMP_TYPE_VAL,

    HTS_IDX_HT_MI_CHAR,
    HTS_IDX_HT_MI_VAL,

    HTS_IDX_HT_OBJ_SIZE_CHAR,
    HTS_IDX_HT_OBJ_SIZE_VAL,

    HTS_IDX_NB,
};

enum {
    POS_IDX_SVC,

    POS_IDX_PLX_SPOT_CHAR,
    POS_IDX_PLX_SPOT_VAL,
    POS_IDX_PLX_SPOT_CCC,

    POS_IDX_PLX_FEAT_CHAR,
    POS_IDX_PLX_FEAT_VAL,

    POS_IDX_PO_MI_CHAR,
    POS_IDX_PO_MI_VAL,

    POS_IDX_PO_OBJ_SIZE_CHAR,
    POS_IDX_PO_OBJ_SIZE_VAL,

    POS_IDX_NB,
};

enum {
    BS_IDX_SVC,

    BS_BAT_LVL_CHAR,
    BS_BAT_LVL_VAL,

    BS_IDX_NB,
};

enum {
    SS_IDX_SVC,

    SS_STEPS_NUMBER_CHAR,
	SS_STEPS_NUMBER_VAL,

    SS_IDX_NB,
};

enum {
    FS_IDX_SVC,

    FS_FALL_DETECTED_CHAR,
	FS_FALL_DETECTED_VAL,

    FS_IDX_NB,
};

typedef struct HR_services {

    bool hr_meas_val; 
    bool hr_meas_ccc;

    bool hr_mi;

} hr_service;

typedef struct HT_services {

    bool ht_meas_val; 
    bool ht_meas_ccc;

    bool ht_mi;

} ht_service; 

typedef struct PO_services {

    bool po_meas_val; 
    bool po_meas_ccc;

    bool po_features;

    bool po_mi;

} po_service; 

typedef struct BS_services {

    bool bs_meas_val;

} bs_service; 
