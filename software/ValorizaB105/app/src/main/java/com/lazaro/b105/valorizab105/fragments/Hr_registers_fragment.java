package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
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
import java.util.Collections;
import java.util.Date;
import java.util.UUID;
import java.util.stream.IntStream;

public class Hr_registers_fragment extends Fragment {

    Patient patient = MainActivity.patients_DB.getPatientByESP32_ID(Content_main_fragment.last_patient_ID);

    CustomMeasuresList adapter;
    ListView list;
    String values[];
    String values_sorted[];
    String dates[];
    Date dates_converted[];
    Date dates_sorted[];
    View hr_regs_view;

    String empty_register;
    public String fake_date = "00:00:00    01/01/1980";

    public interface HrRegistersInteractionListener {}

    private HrRegistersInteractionListener mListener;

    public Hr_registers_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        hr_regs_view = inflater.inflate(R.layout.fragment_hr_registers, container, false);
        empty_register = getResources().getString(R.string.empty_register);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.hr_registers);

        if(patient !=null) {
            int number_of_values = Patient.array_length_1;
            dates = new String[number_of_values];
            dates_converted = new Date[number_of_values];
            dates_sorted = new Date[number_of_values];
            values = new String[number_of_values];
            values_sorted = new String[number_of_values];

            for (int i = 0; i < number_of_values; i++) {
                dates[i] = patient.getHr_date(i);
                values[i] = patient.getHr(i);
                if(values[i].matches(" ") || values[i].matches("")){
                    values[i] = getResources().getString(R.string.empty_register);
                } else {
                    values[i] = values[i];
                }
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
                }catch (Exception e){

                }
            }

            adapter = new CustomMeasuresList(getActivity(), values_sorted, dates, 3);
        }

        list = (ListView) hr_regs_view.findViewById(R.id.hr_registers_listview);
        list.setAdapter(adapter);

        return hr_regs_view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof HrRegistersInteractionListener) {
            mListener = (HrRegistersInteractionListener) context;
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
