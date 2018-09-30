package com.lazaro.b105.valorizab105.fragments;

import android.support.v4.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;

public class Falls_form_fragment extends Fragment {

    Spinner muscles_condition;
    Spinner bones_condition;
    Spinner mobility_degree;
    Spinner falls_frequency;
    EditText previous_falls;
    Button update_form_button;

    private static final float weights[] = {(float)0, (float)0.1, (float)0.2, (float)0.8, (float)1};


    private FallsFormFragmentInteractionListener mListener;

    public interface FallsFormFragmentInteractionListener {}

    public Falls_form_fragment() {}

    View formView;
    private Patient patient;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        formView = inflater.inflate(R.layout.fragment_falls_form, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.falls_form_label);

        int patient_id = getArguments().getInt("ID");
        patient = MainActivity.patients_DB.getPatientbyId(patient_id);

        muscles_condition = (Spinner) formView.findViewById(R.id.muscles_condition_spinner);
        bones_condition = (Spinner) formView.findViewById(R.id.bones_condition_spinner);
        mobility_degree = (Spinner) formView.findViewById(R.id.mobility_degree_spinner);
        falls_frequency = (Spinner) formView.findViewById(R.id.usually_faint_spinner);
        previous_falls = (EditText) formView.findViewById(R.id.number_falls_edittext);
        update_form_button = (Button) formView.findViewById(R.id.button_update_falls_form);

        ArrayAdapter<CharSequence> degree_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.degree_array, R.layout.custom_spinner_item);
        degree_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        ArrayAdapter<CharSequence> yesno_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.yes_no_array, R.layout.custom_spinner_item);
        degree_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        muscles_condition.setAdapter(degree_adapter);
        bones_condition.setAdapter(degree_adapter);
        mobility_degree.setAdapter(degree_adapter);
        falls_frequency.setAdapter(yesno_adapter);

        float spinnerval = patient.getMuscle_condition();
        setSPinnerValue(spinnerval, muscles_condition);
        spinnerval = patient.getBones_condition();
        setSPinnerValue(spinnerval, bones_condition);
        spinnerval = patient.getMobility_degree();
        setSPinnerValue(spinnerval,mobility_degree);

        int fallsval = patient.getUsually_faints();

        if(fallsval == 1){
            falls_frequency.setSelection(1);
        }else {
            falls_frequency.setSelection(0);
        }

        fallsval = patient.getPrevious_falls();
        try {
            previous_falls.setText(String.valueOf(fallsval));
        }catch (Exception e){
        }

        update_form_button.setOnClickListener((View view) -> updatePatientFallsData());

        return formView;
    }

    public void updatePatientFallsData(){
        if(patient == null){
            return;
        }

        patient.setMuscle_condition(getSpinnerValue(muscles_condition));
        patient.setBones_condition(getSpinnerValue(bones_condition));
        patient.setMobility_degree(getSpinnerValue(mobility_degree));

        String fallsval = falls_frequency.getSelectedItem().toString();
        if(fallsval.equals(getResources().getStringArray(R.array.yes_no_array)[0])){
            patient.setUsually_faints(0);
        } else{
            patient.setUsually_faints(1);
        }
        try {
            patient.setPrevious_falls(Integer.parseInt(previous_falls.getText().toString()));
        } catch (Exception e){
        }

        MainActivity.patients_DB.updatePatient(patient);

        getFragmentManager().popBackStack();
    }

    public void setSPinnerValue(float value, Spinner spinner){
        int size = spinner.getAdapter().getCount();
        for(int i = 0; i< size; i++){
            if(value == weights[i]){
                spinner.setSelection(i);
            }
        }
    }

    public float getSpinnerValue(Spinner spinner){
        String spinnerText = spinner.getSelectedItem().toString();
        int spinnerSize = spinner.getAdapter().getCount();

        for(int i = 0; i< spinnerSize; i++){
            if(spinnerText.equals(getResources().getStringArray(R.array.degree_array)[i])) {
                return weights[i];
            }
        }
        return 0;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof FallsFormFragmentInteractionListener) {
            mListener = (FallsFormFragmentInteractionListener) context;
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
