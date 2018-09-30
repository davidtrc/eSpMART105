package com.lazaro.b105.valorizab105;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteCursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;

/**
 * Created by David on 16/04/2017.
 */

public class Settings_DBHandler extends SQLiteOpenHelper {

    private static final int DATABASE_VERSION = 1;
    private static final String DATABASE_NAME = "SettingsMenu";
    private static final String TABLE_SETTINGS = "Settings";

    // Shops Table Columns names
    private static final String KEY_ID = "id";
    private static final String KEY_SETTING_NAME = "setting_name";
    private static final String KEY_SETTING_VALUE = "setting_value";

    public Settings_DBHandler(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_CONTACTS_TABLE = "CREATE TABLE " + TABLE_SETTINGS + "("
                + KEY_ID + " INTEGER PRIMARY KEY AUTOINCREMENT," + KEY_SETTING_NAME + " TEXT NOT NULL," + KEY_SETTING_VALUE + " TEXT NOT NULL " +")";
        db.execSQL(CREATE_CONTACTS_TABLE);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_SETTINGS);
        // Creating tables again
        onCreate(db);
    }

    // Adding new setting
    public void addSetting(String setting, String value) {

        Settings temp = null;

        temp = getSettingsbyName(setting);
        if (temp == null){
            SQLiteDatabase db = this.getWritableDatabase();
            ContentValues values = new ContentValues();
            values.put(KEY_SETTING_NAME, setting);
            values.put(KEY_SETTING_VALUE, value);

            // Inserting Row
            db.insert(TABLE_SETTINGS, null, values);
            db.close(); // Closing database connection
        }

    }

    //Getting all settings
    public ArrayList<Settings> getAllSettings(){

        ArrayList settings = new ArrayList<Settings>();

        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_SETTINGS, null);

        Settings setting;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                do{
                    setting = new Settings(cursor.getInt(0), cursor.getString(1), cursor.getString(2));
                    settings.add(setting);
                } while (cursor.moveToNext());

            }
        } else{
            db.close();
            return null;
        }

        cursor.close();
        db.close();
        return settings;

    }

    // Getting setting by ID
    public Settings getSettingsbyId(int id) {
        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_SETTINGS + " WHERE " + KEY_ID + " =?", new String[]{String.valueOf(id)});

        Settings settings =  null;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                settings = new Settings(cursor.getInt(0), cursor.getString(1), cursor.getString(2));
            }
        } else {
            return settings;
        }

        cursor.close();
        db.close();
        return settings;
    }

    // Getting settings by name
    public Settings getSettingsbyName(String name) {
        SQLiteDatabase db = this.getReadableDatabase();
        SQLiteCursor cursor = (SQLiteCursor) db.rawQuery("SELECT * FROM " + TABLE_SETTINGS + " WHERE " + KEY_SETTING_NAME + " =?", new String[]{name});

        Settings settings =  null;
        if (cursor != null && cursor.getCount() > 0){
            if (cursor.moveToFirst()){
                settings = new Settings(cursor.getInt(0), cursor.getString(1), cursor.getString(2));
            }
        } else {
            return settings;
        }

        cursor.close();
        db.close();
        return settings;
    }

    // Updating a setting
    public int updateSettings(Settings settings) {
        SQLiteDatabase db = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(KEY_SETTING_NAME, settings.getSettingName());
        values.put(KEY_SETTING_VALUE, settings.getSetting_value());

        // updating row
        int result =  db.update(TABLE_SETTINGS, values, KEY_ID + " = ?",
                new String[]{String.valueOf(settings.getSettingsId())});
        db.close();
        return result;
    }

    // Deleting a setting
    public void deleteSettings(Settings settings) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TABLE_SETTINGS, KEY_ID + " = ?",
                new String[] { String.valueOf(settings.getSettingsId()) });
        db.close();
    }
}
