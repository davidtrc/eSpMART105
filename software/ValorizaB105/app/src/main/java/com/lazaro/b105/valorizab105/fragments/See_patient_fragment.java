package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.CardView;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.lazaro.b105.valorizab105.CustomListAdapter;
import com.lazaro.b105.valorizab105.ImageConverter;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;
import com.lazaro.b105.valorizab105.Settings;
import com.lazaro.b105.valorizab105.Settings_DBHandler;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class See_patient_fragment extends Fragment {

    private Patient patient_to_show = CustomListAdapter.patient;

    private ImageView patient_photo;
    private TextView gender = null;
    private TextView blood_type = null;
    private TextView pebble_id =  null;
    private TextView patient_name = null;
    private TextView patient_temp = null;
    private TextView patient_sat = null;
    private TextView patient_hr = null;
    private TextView patient_steps = null;
    private TextView clinical_history = null;
    private TextView age_text = null;
    private CardView temp_card;
    private CardView hr_card;
    private CardView sat_card;
    private CardView steps_card;
    private Button connect_to_patient = null;

    private View see_patient_view;

    public interface SeePatientInteractionListener {}

    private SeePatientInteractionListener mListener;

    public See_patient_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        see_patient_view = inflater.inflate(R.layout.fragment_see_patient, container, false);
        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.see_patient);

        if(patient_to_show != null){
            gender = (TextView) see_patient_view.findViewById(R.id.see_gender);
            blood_type = (TextView) see_patient_view.findViewById(R.id.see_blood_type);
            pebble_id =  (TextView) see_patient_view.findViewById(R.id.see_pebble_id);
            patient_name = (TextView) see_patient_view.findViewById(R.id.see_patient_name);
            patient_temp = (TextView) see_patient_view.findViewById(R.id.see_temp_value);
            patient_sat = (TextView) see_patient_view.findViewById(R.id.see_sat_value);
            patient_hr = (TextView) see_patient_view.findViewById(R.id.see_hr_value);
            patient_steps = (TextView) see_patient_view.findViewById(R.id.see_steps_value);
            patient_photo = (ImageView) see_patient_view.findViewById(R.id.see_patient_photo);
            temp_card = (CardView) see_patient_view.findViewById(R.id.see_temp_card);
            hr_card = (CardView) see_patient_view.findViewById(R.id.see_hr_card);
            sat_card = (CardView) see_patient_view.findViewById(R.id.see_saturation_card);
            steps_card = (CardView) see_patient_view.findViewById(R.id.see_steps_card);
            clinical_history = (TextView) see_patient_view.findViewById(R.id.see_clinical_history);
            connect_to_patient = (Button) see_patient_view.findViewById(R.id.see_connect_to_patient_button);
            age_text = (TextView) see_patient_view.findViewById(R.id.age_text_field);
        }
        setPatientToShow(patient_to_show);

        connect_to_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(patient_to_show != null) {
                    Content_main_fragment.last_patient_ID = String.valueOf(patient_to_show.getId());
                    Settings_DBHandler set_db = MainActivity.settings_DB;
                    Settings set;
                    String st_name = MainActivity.setting1;
                    set = set_db.getSettingsbyName(st_name);
                    set.setSettingValue(Content_main_fragment.last_patient_ID);
                    set_db.updateSettings(set);
                }

                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = Content_main_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = ((FragmentActivity)getContext()).getSupportFragmentManager();
                if (fragmentManager.getBackStackEntryCount() > 0) {
                    FragmentManager.BackStackEntry first = fragmentManager.getBackStackEntryAt(0);
                    fragmentManager.popBackStack(first.getId(), FragmentManager.POP_BACK_STACK_INCLUSIVE);
                }
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).commit();
            }
        });
        return see_patient_view;
    }

    public void setPatientToShow(Patient patient){
        String patient_temp_str = null;
        String patient_sat_str = null;
        String patient_hr_str = null;
        String patient_steps_str = null;

        try {
            patient_temp_str = patient.getTemperature(patient.getArray_index_temp()-1) + " ºC";
            patient_sat_str = patient.getSaturation(patient.getArray_index_sat()-1) + " %";
            patient_hr_str = patient.getHr(patient.getArray_index_hr()-1) + " ppm/ms";
            patient_steps_str = patient.getSteps(patient.getArray_index_steps()-1);
        } catch (Exception e){

        }

        if(gender ==  null || blood_type == null || pebble_id == null || patient_name == null || patient_temp == null ||
                patient_sat == null|| patient_hr == null || patient_steps == null || patient_photo == null || clinical_history == null || age_text == null){
            Fragment fragment = null;
            Class fragmentClass = null;
            fragmentClass = See_patient_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }

            FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("See_patient_fragment").commit();

            gender = (TextView) see_patient_view.findViewById(R.id.see_gender);
            blood_type = (TextView) see_patient_view.findViewById(R.id.see_blood_type);
            pebble_id =  (TextView) see_patient_view.findViewById(R.id.see_pebble_id);
            patient_name = (TextView) see_patient_view.findViewById(R.id.main_patient_name);
            patient_temp = (TextView) see_patient_view.findViewById(R.id.temp_value);
            patient_sat = (TextView) see_patient_view.findViewById(R.id.sat_value);
            patient_hr = (TextView) see_patient_view.findViewById(R.id.hr_value);
            patient_steps = (TextView) see_patient_view.findViewById(R.id.steps_value);
            patient_photo = (ImageView) see_patient_view.findViewById(R.id.main_patient_photo);
            clinical_history = (TextView) see_patient_view.findViewById(R.id.see_clinical_history);
            age_text = (TextView) see_patient_view.findViewById(R.id.age_text_field);
        }

        gender.setText(getResources().getString(R.string.gender) + " " + patient_to_show.getGender());
        blood_type.setText(getResources().getString(R.string.blood_type) + " " + patient_to_show.getBlood_type());
        pebble_id.setText(getResources().getString(R.string.center) + " " + patient_to_show.getCenter());
        age_text.setText(getResources().getString(R.string.age_two_dots) + " " + patient_to_show.getAge());
        patient_name.setText(patient.getName());
        String history = null;
        if(patient_to_show.getClinical_history() != null){
            history = patient_to_show.getClinical_history();
        }
        if(history == null || history.matches("") || history.matches(" ") || history.matches("null") ){
            clinical_history.setText(getResources().getString(R.string.no_history));
        } else {
            clinical_history.setText(history);
        }
        clinical_history.setMovementMethod(new ScrollingMovementMethod());
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
        } else {
            patient_temp.setText(patient_temp_str);
        }
        if(patient_sat_str == null || patient_sat_str.matches("null")){
            patient_sat.setText(R.string.no_record);
            patient_sat.setTextSize(17);
        } else {
            patient_sat.setText(patient_sat_str);
        }
        if(patient_hr_str == null || patient_hr_str.matches("null")){
            patient_hr.setText(R.string.no_record);
            patient_hr.setTextSize(17);
        } else {
            patient_hr.setText(patient_hr_str);
        }
        if(patient_steps_str == null || patient_hr_str.matches("null")){
            patient_steps.setText(R.string.no_record);
            patient_steps.setTextSize(17);
        } else {
            patient_steps.setText(patient_steps_str);
        }

    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof SeePatientInteractionListener) {
            mListener = (SeePatientInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement SeePatientInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

}
