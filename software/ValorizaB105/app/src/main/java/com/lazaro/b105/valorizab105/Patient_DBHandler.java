package com.lazaro.b105.valorizab105;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteCursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;

public class Patient_DBHandler extends SQLiteOpenHelper{

    private static final int DATABASE_VERSION = 1;
    private static final String DATABASE_NAME = "patientsData";
    private static final String TABLE_PATIENTS = "Patients";

    public static String strSeparator = ",";

    // Patients Table Columns names
    private static final String KEY_ID = "id";
    private static final String KEY_NAME = "name";
    private static final String KEY_AGE = "age";
    private static final String KEY_CENTER = "center";
    private static final String KEY_ESP32_ID = "esp32_id";
    private static final String KEY_TEMPERATURE = "temperature";
    private static final String KEY_SATURATION = "saturation";
    private static final String KEY_STEPS = "steps";
    private static final String KEY_HEART_RATE = "heart_rate";
    private static final String KEY_GENDER = "gender";
    private static final String KEY_BLOOD_TYPE = "blood_type";
    private static final String KEY_CLINICAL_HISTORY = "clinical_history";
    private static final String KEY_ARRAY_INDEX_TEMP = "array_index_temp";
    private static final String KEY_ARRAY_INDEX_SAT = "array_index_sat";
    private static final String KEY_ARRAY_INDEX_STEPS = "array_index_steps";
    private static final String KEY_ARRAY_INDEX_HR = "array_index_hr";
    private static final String KEY_TEMP_DATE = "temp_date";
    private static final String KEY_SAT_DATE = "sat_date";
    private static final String KEY_STEPS_DATE = "steps_date";
    private static final String KEY_HR_DATE = "hr_date";
    private static final String KEY_PHOTO_ID = "photo_id";
    private static final String KEY_LAST_CONNECTION_DATE = "last_connection_date";
    private static final String KEY_ESP32_BATTERY = "esp32_battery";
    private static final String KEY_MUSCLES_CONDITION = "muscles_condition";
    private static final String KEY_BONES_CONDITION = "bones_condition";
    private static final String KEY_MOBILITY_DEGREE = "mobility_degree";
    private static final String KEY_USUALLY_FAINTS = "usually_faints";
    private static final String KEY_PREVIOUS_FALLS = "previous_falls";

    public Patient_DBHandler(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_CONTACTS_TABLE = "CREATE TABLE " + TABLE_PATIENTS + "("
                + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT," + KEY_NAME + " TEXT NOT NULL," + KEY_AGE + " TEXT NOT NULL,"
                + KEY_TEMPERATURE + " TEXT," + KEY_ARRAY_INDEX_TEMP + " INTEGER," + KEY_TEMP_DATE + " TEXT,"
                + KEY_SATURATION + " TEXT," + KEY_ARRAY_INDEX_SAT + " INTEGER," + KEY_SAT_DATE + " TEXT,"
                + KEY_STEPS + " TEXT," + KEY_ARRAY_INDEX_STEPS + " INTEGER," + KEY_STEPS_DATE + " TEXT,"
                + KEY_HEART_RATE + " TEXT," + KEY_ARRAY_INDEX_HR + " INTEGER," + KEY_HR_DATE + " TEXT,"
                + KEY_GENDER + " TEXT NOT NULL," + KEY_BLOOD_TYPE + " TEXT NOT NULL," + KEY_CLINICAL_HISTORY + " TEXT," + KEY_PHOTO_ID + " TEXT NOT NULL,"
                + KEY_CENTER + " TEXT NOT NULL," + KEY_ESP32_ID + " TEXT NOT NULL,"  + KEY_LAST_CONNECTION_DATE + " TEXT, " + KEY_ESP32_BATTERY + " TEXT,"
                + KEY_MUSCLES_CONDITION + " REAL NOT NULL," + KEY_BONES_CONDITION + " REAL NOT NULL," + KEY_MOBILITY_DEGREE + " REAL NOT NULL,"
                + KEY_USUALLY_FAINTS + " INTEGER NOT NULL," + KEY_PREVIOUS_FALLS + " INTEGER NOT NULL" + ")";
        db.execSQL(CREATE_CONTACTS_TABLE);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_PATIENTS);
        // Creating tables again
        onCreate(db);
    }

    // Adding new patient
    public long addPatient(Patient patient, String ID) {
        Patient patient_test;
        try {
            patient_test = getPatientbyId(Integer.parseInt(ID));
        } catch (Exception e){
            patient_test = null;
        }
        if(patient_test == null) {
            try {
                SQLiteDatabase db = this.getWritableDatabase();
                ContentValues values = new ContentValues();
                values.put(KEY_NAME, patient.getName());
                values.put(KEY_AGE, patient.getAge());

                values.put(KEY_TEMPERATURE, createStringofCommas(Patient.array_length_1));
                values.put(KEY_ARRAY_INDEX_TEMP, 0);
                values.put(KEY_TEMP_DATE, createStringofCommas(Patient.array_length_1));

                values.put(KEY_SATURATION, createStringofCommas(Patient.array_length_1));
                values.put(KEY_ARRAY_INDEX_SAT, 0);
                values.put(KEY_SAT_DATE, createStringofCommas(Patient.array_length_1));

                values.put(KEY_STEPS, createStringofCommas(Patient.array_length_2));
                values.put(KEY_ARRAY_INDEX_STEPS, 0);
                values.put(KEY_STEPS_DATE, createStringofCommas(Patient.array_length_2));

                values.put(KEY_HEART_RATE, createStringofCommas(Patient.array_length_1));
                values.put(KEY_ARRAY_INDEX_HR, 0);
                values.put(KEY_HR_DATE, createStringofCommas(Patient.array_length_1));

                values.put(KEY_GENDER, patient.getGender());
                values.put(KEY_BLOOD_TYPE, patient.getBlood_type());
                values.put(KEY_CLINICAL_HISTORY, patient.getClinical_history());
                values.put(KEY_PHOTO_ID, patient.getPhoto_id());
                values.put(KEY_CENTER, patient.getCenter());
                values.put(KEY_ESP32_ID, patient.getEsp32_ID());
                values.put(KEY_LAST_CONNECTION_DATE, patient.getLast_connection_date());
                values.put(KEY_ESP32_BATTERY, patient.getESP32_battery());

                values.put(KEY_MUSCLES_CONDITION, patient.getMuscle_condition());
                values.put(KEY_BONES_CONDITION, patient.getBones_condition());
                values.put(KEY_MOBILITY_DEGREE, patient.getMobility_degree());
                values.put(KEY_USUALLY_FAINTS, patient.getUsually_faints());
                values.put(KEY_PREVIOUS_FALLS, patient.getPrevious_falls());

                // Inserting Row
                long ret = db.insert(TABLE_PATIENTS, null, values);
                db.close(); // Closing database connection
                return ret;
            }catch(Exception e){
                return -1;
            }
        }
        return 0;
    }

    // Getting patient by ID
    public Patient getPatientbyId(int id) {
        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_PATIENTS + " WHERE " + KEY_ID + " =?", new String[]{String.valueOf(id)});

        Patient patient =  null;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                patient = new Patient(cursor.getInt(0), cursor.getString(1), cursor.getString(2),
                        convertStringToArray(cursor.getString(3)), cursor.getInt(4), convertStringToArray(cursor.getString(5)),
                        convertStringToArray(cursor.getString(6)), cursor.getInt(7), convertStringToArray(cursor.getString(8)),
                        convertStringToArray(cursor.getString(9)), cursor.getInt(10), convertStringToArray(cursor.getString(11)),
                        convertStringToArray(cursor.getString(12)), cursor.getInt(13), convertStringToArray(cursor.getString(14)),
                        cursor.getString(15), cursor.getString(16), cursor.getString(17), cursor.getString(18),
                        cursor.getString(19), cursor.getString(20), cursor.getString(21), cursor.getString(22),
                        cursor.getFloat(23), cursor.getFloat(24), cursor.getFloat(25), cursor.getInt(26), cursor.getInt(27));
            }
        } else {
            return patient;
        }

        cursor.close();
        db.close();
        return patient;
    }

    // Getting patient by name
    public ArrayList<Patient> getPatientbyName(String name) {
        ArrayList patients = new ArrayList<Patient>();

        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_PATIENTS + " WHERE " + KEY_NAME + " LIKE ?", new String[]{"%" + name + "%"});

        Patient patient;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                do{
                    patient = new Patient(cursor.getInt(0), cursor.getString(1), cursor.getString(2),
                            convertStringToArray(cursor.getString(3)), cursor.getInt(4), convertStringToArray(cursor.getString(5)),
                            convertStringToArray(cursor.getString(6)), cursor.getInt(7), convertStringToArray(cursor.getString(8)),
                            convertStringToArray(cursor.getString(9)), cursor.getInt(10), convertStringToArray(cursor.getString(11)),
                            convertStringToArray(cursor.getString(12)), cursor.getInt(13), convertStringToArray(cursor.getString(14)),
                            cursor.getString(15), cursor.getString(16), cursor.getString(17), cursor.getString(18),
                            cursor.getString(19), cursor.getString(20), cursor.getString(21), cursor.getString(22),
                            cursor.getFloat(23), cursor.getFloat(24), cursor.getFloat(25), cursor.getInt(26), cursor.getInt(27));
                    patients.add(patient);
                } while (cursor.moveToNext());
            }
        } else{
            return null;
        }

        cursor.close();
        db.close();
        return patients;
    }

    // Getting patient by name
    public ArrayList<Patient> getPatientbyCenter(String center) {
        ArrayList patients = new ArrayList<Patient>();

        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_PATIENTS + " WHERE " + KEY_CENTER + " LIKE ?", new String[]{"%" + center + "%"});

        Patient patient;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                do{
                    patient = new Patient(cursor.getInt(0), cursor.getString(1), cursor.getString(2),
                            convertStringToArray(cursor.getString(3)), cursor.getInt(4), convertStringToArray(cursor.getString(5)),
                            convertStringToArray(cursor.getString(6)), cursor.getInt(7), convertStringToArray(cursor.getString(8)),
                            convertStringToArray(cursor.getString(9)), cursor.getInt(10), convertStringToArray(cursor.getString(11)),
                            convertStringToArray(cursor.getString(12)), cursor.getInt(13), convertStringToArray(cursor.getString(14)),
                            cursor.getString(15), cursor.getString(16), cursor.getString(17), cursor.getString(18),
                            cursor.getString(19), cursor.getString(20), cursor.getString(21), cursor.getString(22),
                            cursor.getFloat(23), cursor.getFloat(24), cursor.getFloat(25), cursor.getInt(26), cursor.getInt(27));
                    patients.add(patient);
                } while (cursor.moveToNext());
            }
        } else{
            return null;
        }

        cursor.close();
        db.close();
        return patients;
    }

    // Getting patient by pebble id
    public Patient getPatientByESP32_ID(String esp32_id) {

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery("SELECT * FROM " + TABLE_PATIENTS + " WHERE " + KEY_ESP32_ID + " =?", new String[]{esp32_id});

        Patient patient =  null;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                do{
                    patient = new Patient(cursor.getInt(0), cursor.getString(1), cursor.getString(2),
                            convertStringToArray(cursor.getString(3)), cursor.getInt(4), convertStringToArray(cursor.getString(5)),
                            convertStringToArray(cursor.getString(6)), cursor.getInt(7), convertStringToArray(cursor.getString(8)),
                            convertStringToArray(cursor.getString(9)), cursor.getInt(10), convertStringToArray(cursor.getString(11)),
                            convertStringToArray(cursor.getString(12)), cursor.getInt(13), convertStringToArray(cursor.getString(14)),
                            cursor.getString(15), cursor.getString(16), cursor.getString(17), cursor.getString(18),
                            cursor.getString(19), cursor.getString(20), cursor.getString(21), cursor.getString(22),
                            cursor.getFloat(23), cursor.getFloat(24), cursor.getFloat(25), cursor.getInt(26), cursor.getInt(27));
                } while (cursor.moveToNext());

            }
        } else{
            return patient;
        }
        cursor.close();
        db.close();
        return patient;
    }

    // Updating a patient
    public int updatePatient(Patient patient) {
        SQLiteDatabase db = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(KEY_NAME, patient.getName());
        values.put(KEY_AGE, patient.getAge());
        values.put(KEY_CENTER, patient.getCenter());
        values.put(KEY_ESP32_ID, patient.getEsp32_ID());
        values.put(KEY_TEMPERATURE, convertArrayToString(patient.getTemperature()));
        values.put(KEY_SATURATION, convertArrayToString(patient.getSaturation()));
        values.put(KEY_STEPS, convertArrayToString(patient.getSteps()));
        values.put(KEY_HEART_RATE, convertArrayToString(patient.getHr()));
        values.put(KEY_GENDER, patient.getGender());
        values.put(KEY_BLOOD_TYPE, patient.getBlood_type());
        values.put(KEY_CLINICAL_HISTORY, patient.getClinical_history());
        values.put(KEY_ARRAY_INDEX_TEMP, patient.getArray_index_temp());
        values.put(KEY_ARRAY_INDEX_SAT, patient.getArray_index_sat());
        values.put(KEY_ARRAY_INDEX_STEPS, patient.getArray_index_steps());
        values.put(KEY_ARRAY_INDEX_HR, patient.getArray_index_hr());
        values.put(KEY_TEMP_DATE, convertArrayToString(patient.getTemp_date()));
        values.put(KEY_SAT_DATE, convertArrayToString(patient.getSat_date()));
        values.put(KEY_STEPS_DATE, convertArrayToString(patient.getSteps_date()));
        values.put(KEY_HR_DATE, convertArrayToString(patient.getHr_date()));
        values.put(KEY_PHOTO_ID, patient.getPhoto_id());
        values.put(KEY_LAST_CONNECTION_DATE, patient.getLast_connection_date());
        values.put(KEY_ESP32_BATTERY, patient.getESP32_battery());

        values.put(KEY_MUSCLES_CONDITION, patient.getMuscle_condition());
        values.put(KEY_BONES_CONDITION, patient.getBones_condition());
        values.put(KEY_MOBILITY_DEGREE, patient.getMobility_degree());
        values.put(KEY_USUALLY_FAINTS, patient.getUsually_faints());
        values.put(KEY_PREVIOUS_FALLS, patient.getPrevious_falls());

        // updating row
        int result = db.update(TABLE_PATIENTS, values, KEY_ID + " = ?", new String[]{String.valueOf(patient.getId())});
        db.close();
        return result;
    }

    // Deleting a patient
    public void deletePatient(Patient patient) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TABLE_PATIENTS, KEY_ID + " = ?",
                new String[] { String.valueOf(patient.getId()) });
        db.close();
    }

    public static String convertArrayToString(String[] array){
        if(array == null){
            return null;
        }
        String str = "";
        for (int i = 0; i<array.length; i++) {
            str = str+array[i];
            // Do not append comma at the end of last element
            if(i<array.length-1){
                str = str+strSeparator;
            }
        }
        return str;
    }

    public static String createStringofCommas(int positions){
        String str = "";
        for (int i = 0; i<positions; i++) {
            if (i<(positions - 1)){
                str = str + strSeparator + " ";
            }
        }
        return str;
    }

    public static String[] convertStringToArray(String str){
        if(str == null){
            return null;
        }
        String[] arr = str.split(strSeparator);
        return arr;
    }
}
