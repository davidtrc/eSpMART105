package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.Locale;

public class More_info_fragment extends Fragment {

    private Patient patient_to_show = MainActivity.patients_DB.getPatientByESP32_ID(Content_main_fragment.last_patient_ID);

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
    private TextView patient_age = null;
    private TextView patient_center = null;

    private CardView temp_card;
    private CardView hr_card;
    private CardView sat_card;
    private CardView steps_card;
    private View more_info_view;


    public interface MoreInfoInteractionListener {}

    private MoreInfoInteractionListener mListener;

    public More_info_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        more_info_view = inflater.inflate(R.layout.fragment_more_info, container, false);

        if(patient_to_show != null){
            gender = (TextView) more_info_view.findViewById(R.id.mf_see_gender);
            blood_type = (TextView) more_info_view.findViewById(R.id.mf_see_blood_type);
            pebble_id =  (TextView) more_info_view.findViewById(R.id.mf_see_pebble_id);
            patient_name = (TextView) more_info_view.findViewById(R.id.mf_see_patient_name);
            patient_temp = (TextView) more_info_view.findViewById(R.id.mf_see_temp_value);
            patient_sat = (TextView) more_info_view.findViewById(R.id.mf_see_sat_value);
            patient_hr = (TextView) more_info_view.findViewById(R.id.mf_see_hr_value);
            patient_steps = (TextView) more_info_view.findViewById(R.id.mf_see_steps_value);
            patient_photo = (ImageView) more_info_view.findViewById(R.id.mf_see_patient_photo);
            temp_card = (CardView) more_info_view.findViewById(R.id.mf_see_temp_card);
            hr_card = (CardView) more_info_view.findViewById(R.id.mf_see_hr_card);
            sat_card = (CardView) more_info_view.findViewById(R.id.mf_see_saturation_card);
            steps_card = (CardView) more_info_view.findViewById(R.id.mf_see_steps_card);
            clinical_history = (TextView) more_info_view.findViewById(R.id.mf_see_clinical_history);
            patient_age = (TextView) more_info_view.findViewById(R.id.mf_see_age);
            patient_center = (TextView) more_info_view.findViewById(R.id.mf_see_center);
        }
        try {
            setPatientToShow(patient_to_show);
        }catch (Exception e){

        }

        return more_info_view;
    }

    public void setPatientToShow(Patient patient){
        String patient_temp_str = null;
        String patient_sat_str = null;
        String patient_hr_str = null;
        String patient_steps_str = null;

        try {
            float patient_tempp = Float.parseFloat(patient.getTemperature(patient.getArray_index_temp()-1));
            patient_temp_str = String.format("%.2f %s", patient_tempp, "ºC");
            String hr_pat = patient.getHr(patient.getArray_index_hr()-1);
            String[] p1 = hr_pat.split("/");
            float hr_from_patient = Float.parseFloat(p1[0]);
            float var_from_patient = Float.parseFloat(p1[1]);
            patient_hr_str = String.format("%.0f%s%.2f %s", hr_from_patient, "/", var_from_patient, "ppm/ms");
            //patient_temp_str = patient.getTemperature(patient.getArray_index_temp()-1);
            patient_sat_str = patient.getSaturation(patient.getArray_index_sat()-1);
            //patient_hr_str = patient.getHr(patient.getArray_index_hr()-1);
            patient_steps_str = patient.getSteps(patient.getArray_index_steps()-1);
        } catch (Exception e){

        }

        if(gender ==  null || blood_type == null || pebble_id == null || patient_name == null || patient_temp == null ||
                patient_sat == null|| patient_hr == null || patient_steps == null || patient_photo == null || clinical_history == null
                || patient_age == null || patient_center ==null){
            gender = (TextView) more_info_view.findViewById(R.id.mf_see_gender);
            blood_type = (TextView) more_info_view.findViewById(R.id.mf_see_blood_type);
            pebble_id =  (TextView) more_info_view.findViewById(R.id.mf_see_pebble_id);
            patient_name = (TextView) more_info_view.findViewById(R.id.mf_see_patient_name);
            patient_temp = (TextView) more_info_view.findViewById(R.id.mf_see_temp_value);
            patient_sat = (TextView) more_info_view.findViewById(R.id.mf_see_sat_value);
            patient_hr = (TextView) more_info_view.findViewById(R.id.mf_see_hr_value);
            patient_steps = (TextView) more_info_view.findViewById(R.id.mf_see_steps_value);
            patient_photo = (ImageView) more_info_view.findViewById(R.id.mf_see_patient_photo);
            temp_card = (CardView) more_info_view.findViewById(R.id.mf_see_temp_card);
            hr_card = (CardView) more_info_view.findViewById(R.id.mf_see_hr_card);
            sat_card = (CardView) more_info_view.findViewById(R.id.mf_see_saturation_card);
            steps_card = (CardView) more_info_view.findViewById(R.id.mf_see_steps_card);
            clinical_history = (TextView) more_info_view.findViewById(R.id.mf_see_clinical_history);
            patient_age = (TextView) more_info_view.findViewById(R.id.mf_see_age);
            patient_center = (TextView) more_info_view.findViewById(R.id.mf_see_center);
        }
        patient_name.setText(patient.getName());
        gender.setText(getResources().getString(R.string.gender)+ " " +patient.getGender());
        patient_age.setText(getResources().getString(R.string.age_two_dots)+ " " +patient.getAge());
        blood_type.setText(getResources().getString(R.string.blood_type)+ " " +patient.getBlood_type());
        patient_center.setText(getResources().getString(R.string.center)+ " " +patient.getCenter());
        pebble_id.setText(getResources().getString(R.string.pebble_ID)+ " " +patient.getEsp32_ID());

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
            try{
                String temp = String.format(Locale.getDefault(), "%.2f %s", patient_temp_str, "ºC");
                Log.d("EE", temp);
                Log.d("EE", "EE");
            }catch (Exception e){

            }

            patient_temp.setText(patient_temp_str);
        }
        if(patient_sat_str == null || patient_sat_str.matches("null")){
            patient_sat.setText(R.string.no_record);
            patient_sat.setTextSize(17);
        } else {
            patient_sat.setText(patient_sat_str + " %");
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
            patient_steps.setText(patient_steps_str );
        }

    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof MoreInfoInteractionListener) {
            mListener = (MoreInfoInteractionListener) context;
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
