package com.lazaro.b105.valorizab105.fragments;

import android.app.AlertDialog;
import android.app.TimePickerDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.TimePicker;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.R;
import com.lazaro.b105.valorizab105.Settings;
import com.lazaro.b105.valorizab105.Settings_DBHandler;

import java.util.ArrayList;
import java.util.Calendar;

public class Settings_fragment extends Fragment {

    Settings_DBHandler settings_db = MainActivity.settings_DB;
    ArrayList<Settings> settings = settings_db.getAllSettings();
    private Settings setting;
    private TextView last_p;
    private TextView temp_t;
    private TextView temp_t_text;
    private TextView sat_t;
    private TextView sat_t_text;
    private TextView hr_t;
    private TextView hr_t_text;
    private TextView max_hr_t;
    private TextView max_hr_t_text;
    private TextView min_hr_t;
    private TextView min_hr_t_text;
    private TextView max_variability_t;
    private TextView max_variability_t_text;
    private TextView min_sat_t;
    private TextView min_sat_t_text;
    private TextView min_temp_t;
    private TextView min_temp_t_text;
    private TextView max_temp_t;
    private TextView max_temp_t_text;

    private TextView app_mode;

    private int initialMinutes;

    public interface SettingsInteractionListener {}

    private SettingsInteractionListener mListener;

    public Settings_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View settings_view = inflater.inflate(R.layout.fragment_settings, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.nav_settings);
        MainActivity.navigationView.getMenu().getItem(4).setChecked(true);

        last_p = (TextView) settings_view.findViewById(R.id.last_p_text);

        temp_t = (TextView) settings_view.findViewById(R.id.temp_time);
        temp_t_text = (TextView) settings_view.findViewById(R.id.temp_time_text);
        sat_t = (TextView) settings_view.findViewById(R.id.sat_t);
        sat_t_text = (TextView) settings_view.findViewById(R.id.sat_t_text);
        hr_t = (TextView) settings_view.findViewById(R.id.hr_t);
        hr_t_text = (TextView) settings_view.findViewById(R.id.hr_t_text);
        max_hr_t = (TextView) settings_view.findViewById(R.id.max_hr_t);
        max_hr_t_text = (TextView) settings_view.findViewById(R.id.max_hr_t_text);
        min_hr_t = (TextView) settings_view.findViewById(R.id.min_hr_t);
        min_hr_t_text = (TextView) settings_view.findViewById(R.id.min_hr_t_text);
        max_variability_t = (TextView) settings_view.findViewById(R.id.max_variability_t);
        max_variability_t_text = (TextView) settings_view.findViewById(R.id.max_variability_t_text);
        min_sat_t = (TextView) settings_view.findViewById(R.id.min_sat_t);
        min_sat_t_text = (TextView) settings_view.findViewById(R.id.min_sat_t_text);
        max_temp_t = (TextView) settings_view.findViewById(R.id.max_temp_t);
        max_temp_t_text = (TextView) settings_view.findViewById(R.id.max_temp_t_text);
        min_temp_t = (TextView) settings_view.findViewById(R.id.min_temp_t);
        min_temp_t_text = (TextView) settings_view.findViewById(R.id.min_temp_t_text);

        if(settings != null) {
            setting  = settings.get(0);
            try {
                String p_name = MainActivity.patients_DB.getPatientbyId(Integer.parseInt(setting.getSetting_value())).getName();
                last_p.setText(p_name);
            } catch (Exception e){
                last_p.setText(" ");
            }
            setting = settings.get(1);
            int seconds_int = Integer.parseInt(setting.getSetting_value());
            int hours_int = seconds_int/3600;
            int mins_int = seconds_int%3600;
            mins_int = mins_int/60;
            temp_t_text.setText(String.valueOf(hours_int) + " " +getContext().getResources().getString(R.string.hours_and)+ " " +String.valueOf(mins_int)+ " " +getContext().getResources().getString(R.string.minutes));

            setting = settings.get(2);
            seconds_int = Integer.parseInt(setting.getSetting_value());
            hours_int = seconds_int/3600;
            mins_int = seconds_int%3600;
            mins_int = mins_int/60;
            sat_t_text.setText(String.valueOf(hours_int) + " " +getContext().getResources().getString(R.string.hours_and)+ " " +String.valueOf(mins_int)+ " " +getContext().getResources().getString(R.string.minutes));

            setting = settings.get(3);
            seconds_int = Integer.parseInt(setting.getSetting_value());
            hours_int = seconds_int/3600;
            mins_int = seconds_int%3600;
            mins_int = mins_int/60;
            hr_t_text.setText(String.valueOf(hours_int) + " " +getContext().getResources().getString(R.string.hours_and)+ " " +String.valueOf(mins_int)+ " " +getContext().getResources().getString(R.string.minutes));

            setting = settings.get(5);
            seconds_int = Integer.parseInt(setting.getSetting_value());
            max_hr_t_text.setText(String.valueOf(seconds_int) + " " +getContext().getResources().getString(R.string.ppm));

            setting = settings.get(6);
            try {
                seconds_int = Integer.parseInt(setting.getSetting_value());
            } catch (Exception e){

            }
            min_hr_t_text.setText(String.valueOf(seconds_int) + " " +getContext().getResources().getString(R.string.ppm));

            setting = settings.get(7);
            double th_value = 0;
            try {
                th_value = Double.parseDouble(setting.getSetting_value());
            } catch (Exception e){

            }
            max_variability_t_text.setText(String.valueOf(th_value) + " " +getContext().getResources().getString(R.string.ms));

            setting = settings.get(8);
            seconds_int = Integer.parseInt(setting.getSetting_value());
            min_sat_t_text.setText(String.valueOf(seconds_int) + " " +getContext().getResources().getString(R.string.percetage));

            setting = settings.get(9);
            th_value = Double.parseDouble(setting.getSetting_value());
            max_temp_t_text.setText(String.valueOf(th_value) + " " +getContext().getResources().getString(R.string.degrees));

            setting = settings.get(10);
            th_value = Double.parseDouble(setting.getSetting_value());
            min_temp_t_text.setText(String.valueOf(th_value) + " " +getContext().getResources().getString(R.string.degrees));
        }

        temp_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {changeTime(1);}
        });
        temp_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeTime(1);
            }
        });

        sat_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeTime(2);
            }
        });
        sat_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeTime(2);
            }
        });

        hr_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeTime(3);
            }
        });
        hr_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeTime(3);
            }
        });

        max_hr_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(6, false);
            }
        });
        max_hr_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(6, false);
            }
        });

        min_hr_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(7, false);
            }
        });
        min_hr_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(7, false);
            }
        });

        max_variability_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(8, true);
            }
        });
        max_variability_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(8, true);
            }
        });

        min_sat_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(9, false);
            }
        });
        min_sat_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(9, false);
            }
        });

        min_temp_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(10, true);
            }
        });
        min_temp_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(10, true);
            }
        });

        max_temp_t_text.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(11, true);
            }
        });
        max_temp_t.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                changeThresholds(11, true);
            }
        });
        return settings_view;

    }

    private void changeThresholds(int parameter, boolean decimal){

        AlertDialog.Builder alert = new AlertDialog.Builder(getActivity());
        alert.setTitle(getString(R.string.set_new_value));
        final EditText input = new EditText(getActivity());
        if (decimal){
            input.setInputType(InputType.TYPE_NUMBER_FLAG_DECIMAL);
        }else{
            input.setInputType(InputType.TYPE_CLASS_NUMBER);
        }

        input.setRawInputType(Configuration.KEYBOARD_12KEY);
        alert.setView(input);
        alert.setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
            Settings set1;
            public void onClick(DialogInterface dialog, int whichButton) {
                String text = input.getText().toString();
                set1 = settings.get(parameter);
                set1.setSettingValue(text);
                switch (parameter){
                    case 6:
                        max_hr_t_text.setText(text+ " " +getResources().getString(R.string.ppm));
                        break;
                    case 7:
                        min_hr_t_text.setText(text+ " " +getResources().getString(R.string.ppm));
                        break;
                    case 8:
                        max_variability_t_text.setText(text+ " " +getResources().getString(R.string.ms));
                        break;
                    case 9:
                        min_sat_t_text.setText(text+ " " +getResources().getString(R.string.percetage));
                        break;
                    case 10:
                        max_temp_t_text.setText(text+ " " +getResources().getString(R.string.degrees));
                        break;
                    case 11:
                        min_temp_t_text.setText(text+ " " +getResources().getString(R.string.degrees));
                        break;
                    default:
                        break;
                }
                if(set1 != null) {
                    settings_db.updateSettings(set1);
                }
            }
        });
        alert.setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                //Put actions for CANCEL button here, or leave in blank
            }
        });
        alert.show();

    }

    private void changeTime(int parameter){
        Calendar mcurrentTime = Calendar.getInstance();
        int hour = mcurrentTime.get(Calendar.HOUR_OF_DAY);
        int minute = mcurrentTime.get(Calendar.MINUTE);
        TimePickerDialog mTimePicker;
        mTimePicker = new TimePickerDialog(getContext(), new TimePickerDialog.OnTimeSetListener() {
            @Override
            public void onTimeSet(TimePicker timePicker, int selectedHour, int selectedMinute) {
                switch (parameter){
                    case 1:
                        temp_t_text.setText(selectedHour+ " " +getResources().getString(R.string.hours_and)+ " " +selectedMinute+ " " +getResources().getString(R.string.minutes));
                        break;
                    case 2:
                        sat_t_text.setText(selectedHour+ " " +getResources().getString(R.string.hours_and)+ " " +selectedMinute+ " " +getResources().getString(R.string.minutes));
                        break;
                    case 3:
                        hr_t_text.setText(selectedHour+ " " +getResources().getString(R.string.hours_and)+ " " +selectedMinute+ " " +getResources().getString(R.string.minutes));
                        break;
                    default:
                        break;
                }

                int seconds_selected = selectedHour*3600 + selectedMinute*60;
                Settings temp_s = settings.get(parameter);
                temp_s.setSettingValue(String.valueOf(seconds_selected));
                settings_db.updateSettings(temp_s);
            }
        }, hour, minute, true);
        mTimePicker.setTitle(getResources().getString(R.string.measure_each));
        mTimePicker.show();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof SettingsInteractionListener) {
            mListener = (SettingsInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

}
