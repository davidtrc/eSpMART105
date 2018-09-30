package com.lazaro.b105.valorizab105;

public class Patient {

    public static final int array_length_1 = 20;
    public static final int array_length_2 = 10;
    private int id;
    private String name;
    private String esp32_ID;
    private String[] temperature;
    private String[] saturation;
    private String[] steps;
    private String[] heart_rate;
    private String gender;
    private String blood_type;
    private String clinical_history;
    private int array_index_temp;
    private int array_index_sat;
    private int array_index_steps;
    private int array_index_hr;
    private String[] temp_date;
    private String[] sat_date;
    private String[] steps_date;
    private String[] hr_date;
    private String photo_id;
    private String age;
    private String center;
    private String last_connection_date;
    private String esp32_battery;
    private float muscle_condition;
    private float bones_condition;
    private float mobility_degree;
    private int usually_faints;
    private int previous_falls;


    public static String strSeparator = Patient_DBHandler.strSeparator;

    public Patient() {
        this.temperature = new String[array_length_1];
        this.saturation = new String[array_length_1];
        this.heart_rate = new String[array_length_1];
        this.steps = new String[array_length_2];

        this.array_index_steps = 0;
        this.array_index_sat = 0;
        this.array_index_temp = 0;
        this.array_index_hr = 0;

        this.temp_date = new String[array_length_1];
        this.sat_date = new String[array_length_1];
        this.hr_date = new String[array_length_1];
        this.steps_date = new String[array_length_2];

        for(int i = 0; i<array_length_1; i++){
            this.temperature[i] = "";
            this.temp_date[i] = "";
            this.saturation[i] = "";
            this.sat_date[i] = "";
            this.heart_rate[i]= "";
            this.hr_date[i] = "";

        }
        for(int i = 0; i<array_length_2; i++){
            this.steps[i] = "";
            this.steps_date[i] = "";
        }

        this.esp32_battery = "-";

        this.muscle_condition = 0;
        this.bones_condition = 0;
        this.mobility_degree = 0;
        this.usually_faints = 0;
        this.previous_falls = 0;
    }

    public Patient(int id, String name, String age,
                   String[] temperature, int temp_index, String[] temp_date,
                   String[] saturation, int sat_index, String[] sat_date,
                   String[] steps, int steps_index, String[] steps_date,
                   String[] heart_rate, int hr_index, String[] hr_date,
                   String gender, String blood_type, String clinical_history, String photo_id,
                   String center, String esp32_ID, String last_connection_date, String esp32_battery,
                   float muscle_condition, float bones_condition, float mobility_degree, int usually_faints, int previous_falls){
        this.id = id;
        this.name = name;
        this.age = age;
        this.center = center;
        this.esp32_ID = esp32_ID;

        this.temperature = temperature;
        this.temp_date = temp_date;
        this.array_index_temp = temp_index;

        this.saturation = saturation;
        this.sat_date = sat_date;
        this.array_index_sat = sat_index;

        this.heart_rate = heart_rate;
        this.hr_date = hr_date;
        this.array_index_hr = hr_index;

        this.steps = steps;
        this.steps_date = steps_date;
        this.array_index_steps = steps_index;

        this.gender = gender;
        this.blood_type = blood_type;
        this.clinical_history = clinical_history;
        this.photo_id = photo_id;

        this.last_connection_date = last_connection_date;
        this.esp32_battery = esp32_battery;

        this.muscle_condition = muscle_condition;
        this.bones_condition = bones_condition;
        this.mobility_degree = mobility_degree;
        this.usually_faints = usually_faints;
        this.previous_falls = previous_falls;
    }

    public void setName (String name){
        this.name = name;
    }


    public void setESP32_ID (String esp32_ID){
        this.esp32_ID = esp32_ID;
    }

    public void setTemperature (String temperature){
        if(!(this.array_index_temp < array_length_1)){
            this.array_index_temp = 0;
        }
        this.temperature[array_index_temp] = temperature;
    }

    public void setTemperatureDate (String temperatureDate){
        if(!(this.array_index_temp < array_length_1)){
            this.array_index_temp = 0;
        }
        this.temp_date[array_index_temp] = temperatureDate;
        this.array_index_temp++;
    }

    public void setSaturation (String saturation){
        if(!(this.array_index_sat < array_length_1)) {
            this.array_index_sat = 0;
        }
        this.saturation[array_index_sat] = saturation;
    }

    public void setSaturationDate (String saturationDate){
        if(!(this.array_index_sat < array_length_1)) {
            this.array_index_sat = 0;
        }
        this.sat_date[array_index_sat] = saturationDate;
        this.array_index_sat++;
    }

    public void setHeart_rate (String heart_rate){
        if(!(this.array_index_hr < array_length_1)) {
            this.array_index_hr = 0;
        }
        this.heart_rate[array_index_hr] = heart_rate;
    }

    public void setHeart_rateDate (String heart_rateDate){
        if(!(this.array_index_hr < array_length_1)) {
            this.array_index_hr = 0;
        }
        this.hr_date[array_index_hr] = heart_rateDate;
        this.array_index_hr++;
    }

    public void setSteps (String steps){
        if(!(this.array_index_steps < array_length_2)) {
            this.array_index_steps = 0;
        }
        this.steps[array_index_steps] = steps;
    }

    public void setStepsDate (String stepsDate) {
        if(!(this.array_index_steps < array_length_2)) {
            this.array_index_steps = 0;
        }
        this.steps_date[array_index_steps] = stepsDate;
        this.array_index_steps++;
    }

    public void setGender (String gender){
        this.gender = gender;
    }

    public void setBlood_type (String blood_type){
        this.blood_type = blood_type;
    }

    public void setClinical_history (String clinical_history){this.clinical_history = clinical_history;}

    public void setPhoto_id(String photo_id){
        this.photo_id = photo_id;
    }

    public void setAge(String age){
        this.age = age;
    }

    public void setCenter(String center){
        this.center = center;
    }

    public void setEsp32_ID (String esp32_ID){
        this.esp32_ID = esp32_ID;
    }

    public void setLast_connection_date(String last_connection_date){this.last_connection_date = last_connection_date;}

    public void setESP32_battery (String esp32_battery){ this.esp32_battery = esp32_battery; }

    public int getId(){
        return this.id;
    }

    public String getName(){
        return name;
    }

    public String getAge() {return age;}

    public String getCenter() {return center;}

    public String getEsp32_ID(){
        return esp32_ID;
    }

    public String getTemperature(int index){
         return this.temperature[index];
    }

    public String[] getTemperature(){
        return this.temperature;
    }

    public String getSaturation(int index){
        return saturation[index];
    }

    public String[] getSaturation(){
        return saturation;
    }

    public String getSteps(int index){
        return steps[index];
    }

    public String[] getSteps(){
        return steps;
    }

    public String getHr(int index){
        return heart_rate[index];
    }

    public String[] getHr(){
        return heart_rate;
    }

    public String getGender(){
        return this.gender;
    }

    public String getBlood_type(){
        return blood_type;
    }

    public String getClinical_history(){
        return clinical_history;
    }

    public int getArray_index_temp(){
        return array_index_temp;
    }

    public int getArray_index_sat(){
        return array_index_sat;
    }

    public int getArray_index_steps(){
        return array_index_steps;
    }

    public int getArray_index_hr(){
        return array_index_hr;
    }

    public String getTemp_date(int index){
        return temp_date[index];
    }

    public String[] getTemp_date(){
        return temp_date;
    }

    public String getSat_date(int index){
        return sat_date[index];
    }

    public String[] getSat_date(){
        return sat_date;
    }

    public String getSteps_date(int index){
        return steps_date[index];
    }

    public String[] getSteps_date(){
        return steps_date;
    }

    public String getHr_date(int index){
        return hr_date[index];
    }

    public String[] getHr_date(){
        return hr_date;
    }

    public String getPhoto_id(){
        return photo_id;
    }

    public String getLast_connection_date(){ return last_connection_date;}

    public  String getESP32_battery(){ return esp32_battery;}

    public float getMuscle_condition(){ return muscle_condition;}

    public void setMuscle_condition(float muscle_condition){this.muscle_condition = muscle_condition; }

    public float getBones_condition(){ return bones_condition;}

    public void setBones_condition(float bones_condition){ this.bones_condition = bones_condition;}

    public float getMobility_degree(){ return mobility_degree;}

    public void setMobility_degree(float mobility_degree){ this.mobility_degree = mobility_degree;}

    public int getUsually_faints(){ return usually_faints;}

    public void setUsually_faints(int usually_faints){ this.usually_faints = usually_faints;}

    public int getPrevious_falls(){ return previous_falls;}

    public void setPrevious_falls(int previous_falls){ this.previous_falls = previous_falls;}

    public String toString() {
        String t = "";
        String hr = "";
        String st = "";
        String sat = "";
        String ch;
        String ph_id;

        String td = "";
        String hrd = "";
        String std =  "";
        String satd = "";

        for( int i = 0; i<temperature.length; i++){
            t = t + " " + this.temperature[i];
            td = td + "" + this.temp_date[i];
            sat = sat + "" + this.saturation[i];
            satd = satd + this.sat_date[i];
            hr = hr + "" + this.heart_rate[i];
            hrd = hrd + "" + this.hr_date[i];
        }

        for (int i = 0; i<steps.length; i++) {
            st = st + "" + this.steps[i];
            std = std + "" + this.steps_date[i];
        }

        if(this.clinical_history == null){
            ch = "null";
        } else {
            ch = this.clinical_history.toString();
        }

        if(this.photo_id == null){
            ph_id = "null";
        } else {
            ph_id = this.photo_id;
        }

        return "ID: " + this.id + " Name: " + this.name  + " Temperature: " + t + " Saturation: " + sat +
                " Steps: " + st + " Heart rate: " + hr + " Gender: " + this.gender + " Blood Type: " + this.blood_type + " Clinical History: " +
                ch + " Photo ID: " + ph_id + " Temperature Date: " + td + " Saturation Date: " + satd + " Steps Date: " +
                std + "Heart Rate Date: " + hrd + "Temp, sat, steps, hr index: " + array_index_temp + array_index_sat + array_index_steps + array_index_hr
                +"Bones condition: " + bones_condition + " Muscles condition: " + muscle_condition + " Mobility degree: " + mobility_degree
                + "Faints: " +usually_faints + " Previous falls: " + previous_falls;
    }

}

