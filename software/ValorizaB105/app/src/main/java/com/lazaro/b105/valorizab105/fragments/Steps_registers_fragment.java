package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;
import com.lazaro.b105.valorizab105.CustomMeasuresList;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;
import com.lazaro.b105.valorizab105.SplashActivity;

import java.util.Arrays;
import java.util.Date;
import java.util.UUID;

public class Steps_registers_fragment extends Fragment {
    Patient patient = MainActivity.patients_DB.getPatientByESP32_ID(Content_main_fragment.last_patient_ID);
    CustomMeasuresList adapter;

    ListView list;
    String values[];
    String values_sorted[];
    String dates[];
    Date dates_converted[];
    Date dates_sorted[];
    public String fake_date = "00:00:00    01/01/1980";

    String empty_register;
    View steps_reg_view;

    public interface StepsRegistersInteractionListener {}

    private StepsRegistersInteractionListener mListener;

    public Steps_registers_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        steps_reg_view = inflater.inflate(R.layout.fragment_steps_registers, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.steps_registers);

        empty_register = getResources().getString(R.string.empty_register);

        if(patient !=null) {
            int number_of_values = Patient.array_length_2;
            dates = new String[number_of_values];
            dates_converted = new Date[number_of_values];
            dates_sorted = new Date[number_of_values];
            values = new String[number_of_values];
            values_sorted = new String[number_of_values];

            for (int i = 0; i < number_of_values; i++) {
                dates[i] = patient.getSteps_date(i);
                Log.i("DATES", dates[i]);
                if(dates[i] ==  null || dates[i].matches(" ") || dates[i].matches("") || dates[i].matches("  ")){
                    dates[i] = fake_date;
                }
                values[i] = patient.getSteps(i);
                if(values[i].matches(" ") || values[i].matches("")){
                    values[i] = getResources().getString(R.string.empty_register);
                }
                Log.i("DATES2", dates[i]);
            }
            for (int i = 0; i < number_of_values; i++) {
                try {
                    dates_converted[i] = MainActivity.dateFormat.parse(dates[i]);
                } catch (Exception e) {

                }
            }

            ArrayIndexComparator comparator = new ArrayIndexComparator(dates_converted);
            Integer[] indexes = comparator.createIndexArray();
            try {
                Arrays.sort(indexes, comparator);
            }catch (Exception e){

            }

            for (int i = 0; i < number_of_values; i++) {
                values_sorted[i] = values[indexes[i]];
                dates_sorted[i] = dates_converted[indexes[i]];
            }
            for (int i = 0; i < number_of_values; i++) {
                try {
                    dates[i] = MainActivity.dateFormat.format(dates_sorted[i]);
                    if(dates[i].matches(fake_date)){
                        dates[i] = " ";
                    }
                }catch (Exception e){

                }
            }

            adapter = new CustomMeasuresList(getActivity(), values_sorted, dates, 4);
        }

        list = (ListView) steps_reg_view.findViewById(R.id.steps_registers_listview);
        list.setAdapter(adapter);

        return steps_reg_view;
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof StepsRegistersInteractionListener) {
            mListener = (StepsRegistersInteractionListener) context;
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
