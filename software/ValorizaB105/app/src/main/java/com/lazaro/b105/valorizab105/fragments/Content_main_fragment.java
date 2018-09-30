package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.CardView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;
import com.lazaro.b105.valorizab105.ImageConverter;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;
import com.lazaro.b105.valorizab105.Settings;
import com.lazaro.b105.valorizab105.SplashActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.UUID;

public class Content_main_fragment extends Fragment {


    public static String last_patient_ID;
    private Patient patient_to_show;

    public static TextView connected_to_patient = null;
    private TextView patient_name = null;
    public static TextView patient_temp = null;
    public static TextView patient_sat = null;
    public static TextView patient_hr = null;
    public static TextView patient_steps = null;
    public static TextView esp32_battery = null;
    //public static int temp_color = 0;
    //public static int sat_color = 0;
    //public static int hr_color = 0;
    //public static int steps_color = 0;
    private ImageView patient_photo;
    private Button moreinfobutton;
    private Button find_near_patients;
    private CardView temp_card;
    private CardView hr_card;
    private CardView sat_card;
    private CardView steps_card;
    public static boolean hr_alarm;
    public static boolean hr_var_alarm;
    public static boolean ht_alarm;
    public static boolean po_alarm;
    public static boolean esp32_battery_alarm;
    public static final int ESP32_TH = 10;


    private View main_view;

    public interface OnFragmentInteractionListener {}

    OnFragmentInteractionListener mListener;

    public Content_main_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        main_view = inflater.inflate(R.layout.fragment_content_main, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.main_label);
        MainActivity.navigationView.getMenu().getItem(0).setChecked(true);

        connected_to_patient = (TextView) main_view.findViewById(R.id.main_connected_to_patient_value);
        patient_name = (TextView) main_view.findViewById(R.id.main_patient_name);
        patient_temp = (TextView) main_view.findViewById(R.id.temp_value);
        patient_sat = (TextView) main_view.findViewById(R.id.sat_value);
        patient_hr = (TextView) main_view.findViewById(R.id.hr_value);
        patient_steps = (TextView) main_view.findViewById(R.id.steps_value);
        patient_photo = (ImageView) main_view.findViewById(R.id.main_patient_photo);
        moreinfobutton = (Button) main_view.findViewById(R.id.moreinfobutton);
        find_near_patients = (Button) main_view.findViewById(R.id.main_find_patients);
        temp_card = (CardView) main_view.findViewById(R.id.temp_card);
        hr_card = (CardView) main_view.findViewById(R.id.hr_card);
        sat_card = (CardView) main_view.findViewById(R.id.saturation_card);
        steps_card = (CardView) main_view.findViewById(R.id.steps_card);
        esp32_battery = (TextView) main_view.findViewById(R.id.esp32_battery_text);

        moreinfobutton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = More_info_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("More_info_fragment").commit();
            }
        });

        temp_card.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Temp_registers_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Temp_registers_fragment").commit();
            }
        });

        sat_card.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Sat_registers_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Sat_registers_fragment").commit();
            }
        });

        hr_card.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Hr_registers_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Hr_registers_fragment").commit();
            }
        });

        steps_card.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Steps_registers_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Steps_registers_fragment").commit();
            }
        });

        find_near_patients.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Connect_ESP32_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Connect_ESP32_fragment").commit();
            }
        });

        Patient patient_on_load = null;
        last_patient_ID = MainActivity.settings_DB.getSettingsbyName(MainActivity.setting1).getSetting_value();
        try{
            patient_on_load = MainActivity.patients_DB.getPatientByESP32_ID(last_patient_ID);
        }catch (Exception e){}
        if(patient_on_load == null){
            for(int i=0; i<100; i++){
                patient_on_load = MainActivity.patients_DB.getPatientbyId(i);
                if(patient_on_load != null){
                    last_patient_ID = patient_on_load.getEsp32_ID();
                    MainActivity.settings_DB.getSettingsbyName(MainActivity.setting1).setSettingValue(last_patient_ID);
                    break;
                }
            }
        }

        if(patient_on_load != null) {
            Settings set = MainActivity.settings_DB.getSettingsbyName(MainActivity.setting1);
            set.setSettingValue(patient_on_load.getEsp32_ID());
            MainActivity.settings_DB.updateSettings(set);
            setPatientToShow(patient_on_load);
        }
        return main_view;
    }

    public void setPatientToShow(Patient patient){
        String patient_temp_str = null;
        String patient_sat_str = null;
        String patient_hr_str = null;
        String patient_steps_str = null;

        try {
            float temp_from_patient = Float.parseFloat(patient.getTemperature(patient.getArray_index_temp()-1));
            if(temp_from_patient>=MainActivity.ht_up_th || temp_from_patient<=MainActivity.ht_bt_th){
                patient_temp.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
            } else {
                patient_temp.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.green_connected));
            }
            patient_temp_str = patient.getTemperature(patient.getArray_index_temp()-1) + " ºC";
            try{
                float patient_tempp = Float.parseFloat(patient.getTemperature(patient.getArray_index_temp()-1));
                patient_temp_str = String.format("%.2f %s", patient_tempp, "ºC");
            }catch (Exception e){
            }

        } catch (Exception e){

        }
        try {
            float sat_from_patient = Float.parseFloat(patient.getSaturation(patient.getArray_index_sat()-1));
            if(sat_from_patient<=MainActivity.po_bt_th){
                patient_sat.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
            } else {
                patient_sat.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.green_connected));
            }
            patient_sat_str = patient.getSaturation(patient.getArray_index_sat()-1) + " %";
        } catch (Exception e){

        }
        try {
            String hr_pat = patient.getHr(patient.getArray_index_hr()-1);
            String[] p1 = hr_pat.split("/");
            float hr_from_patient = Float.parseFloat(p1[0]);
            float var_from_patient = Float.parseFloat(p1[1]);

            if(hr_from_patient<=MainActivity.hr_bt_th || hr_from_patient>=MainActivity.hr_up_th){
                patient_hr.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
            } else {
                patient_hr.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.green_connected));
            }
            if(var_from_patient>=MainActivity.hr_var_up_th){
                patient_hr.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
            }

            patient_hr_str = patient.getHr(patient.getArray_index_hr()-1) + " ppm/ms";
            try{
                patient_hr_str = String.format("%.0f%s%.2f %s", hr_from_patient, "/", var_from_patient, "ppm/ms");
            }catch (Exception e){
                Log.i("ERROR", e.toString());
            }
        } catch (Exception e){

        }
        try {
            float step_from_patient = Float.parseFloat(patient.getSteps(patient.getArray_index_steps()-1));
            patient_steps_str = patient.getSteps(patient.getArray_index_steps()-1) + " " + getContext().getResources().getString(R.string.steps2);
        } catch (Exception e){

        }
        try {
            if(patient.getLast_connection_date().matches("NEVER")){
                connected_to_patient.setText(getContext().getResources().getString(R.string.never));
                connected_to_patient.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
            } else {
                connected_to_patient.setText(patient.getLast_connection_date());
                connected_to_patient.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.green_connected));
            }
        } catch (Exception e){

        }
        try{
            int bat_percentage = Integer.parseInt(patient.getESP32_battery());
             esp32_battery.setText(getContext().getResources().getString(R.string.esp32_battery) + " " +patient.getESP32_battery()+ "%");
             if(bat_percentage <=10){
                 esp32_battery.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
             } else{
                 esp32_battery.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.green_connected));
             }

        }catch(Exception e){

        }

        patient_to_show = patient;
        if(patient_name == null || patient_temp == null || patient_sat == null|| patient_hr == null || patient_steps == null || patient_photo == null ){
            Log.d("Some elements", "Are NULL");
            Fragment fragment = null;
            Class fragmentClass = null;
            fragmentClass = Content_main_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }

            FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Content_main_fragment").commit();

            patient_name = (TextView) main_view.findViewById(R.id.main_patient_name);
            patient_temp = (TextView) main_view.findViewById(R.id.temp_value);
            patient_sat = (TextView) main_view.findViewById(R.id.sat_value);
            patient_hr = (TextView) main_view.findViewById(R.id.hr_value);
            patient_steps = (TextView) main_view.findViewById(R.id.steps_value);
            patient_photo = (ImageView) main_view.findViewById(R.id.main_patient_photo);
        }

        patient_name.setText(patient.getName());
        String mCurrentPhotoPath = patient.getPhoto_id();
        File photo = new File(mCurrentPhotoPath);
        Bitmap b = null;
        Bitmap bcircular = null;
        try {
            b = BitmapFactory.decodeStream(new FileInputStream(photo));
            bcircular = ImageConverter.getRoundedCornerBitmap(b, 300);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        patient_photo.setImageBitmap(bcircular);

        if(patient_temp_str == null || patient_temp_str.matches("null") ){
            patient_temp.setText(R.string.no_record);
            patient_temp.setTextSize(17);
            patient_temp.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
        } else {
            patient_temp.setText(patient_temp_str);
        }

        if(patient_sat_str == null || patient_sat_str.matches("null")){
            patient_sat.setText(R.string.no_record);
            patient_sat.setTextSize(17);
            patient_sat.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
        } else {
            patient_sat.setText(patient_sat_str);
        }

        if(patient_hr_str == null || patient_hr_str.matches("null")){
            patient_hr.setText(R.string.no_record);
            patient_hr.setTextSize(17);
            patient_hr.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
        } else {
            patient_hr.setText(patient_hr_str);
        }

        if(patient_steps_str == null ){//|| patient_hr_str.matches("null")){
            patient_steps.setText(R.string.no_record);
            patient_steps.setTextSize(17);
            patient_steps.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), R.color.red_unconnected));
        } else {
            patient_steps.setText(patient_steps_str);
        }

    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

}
