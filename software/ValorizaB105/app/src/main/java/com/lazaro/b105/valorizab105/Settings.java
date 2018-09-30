package com.lazaro.b105.valorizab105;

/**
 * Created by David on 16/04/2017.
 */

public class Settings {

    private int id;
    private String setting_name;
    private String setting_value;

    public Settings(){}

    public Settings(int id, String setting_name, String setting_value){
        this.id = id;
        this.setting_name = setting_name;
        this.setting_value = setting_value;
    }

    public void setSettingName(String name){
        this.setting_name = name;
    }

    public void setSettingValue(String value){
        this.setting_value = value;
    }

    public int getSettingsId(){
        return id;
    }

    public String getSettingName(){
        return setting_name;
    }

    public String getSetting_value(){
        return setting_value;
    }
}
