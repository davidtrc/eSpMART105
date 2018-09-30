package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.lazaro.b105.valorizab105.CustomMeasuresList;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;

import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

public class Temp_registers_fragment extends Fragment {
    Patient patient = MainActivity.patients_DB.getPatientByESP32_ID(Content_main_fragment.last_patient_ID);
    CustomMeasuresList adapter;

    ListView list;
    String values[];
    String values_sorted[];
    String dates[];
    Date dates_converted[];
    Date dates_sorted[];
    String empty_register;
    View temp_reg_view;
    public String fake_date = "00:00:00    01/01/1980";

    public interface TempRegistersInteractionListener { }

    private TempRegistersInteractionListener mListener;

    public Temp_registers_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        temp_reg_view = inflater.inflate(R.layout.fragment_temp_register, container, false);
        empty_register = getResources().getString(R.string.empty_register);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.temp_registers);

        if(patient !=null) {
            int number_of_values = Patient.array_length_1;
            dates = new String[number_of_values];
            dates_converted = new Date[number_of_values];
            dates_sorted = new Date[number_of_values];
            values = new String[number_of_values];
            values_sorted = new String[number_of_values];

            for (int i = 0; i < number_of_values; i++) {
                dates[i] = patient.getTemp_date(i);
                if(dates[i] ==  null || dates[i].matches(" ")){
                    dates[i] = fake_date;
                }
                try {
                    Log.i("EX", "1");
                    float temp_temp = Float.parseFloat(patient.getTemperature(i));
                    Log.i("EX", "2");
                    String temp = String.format(Locale.getDefault(), "%.2f", temp_temp);
                    Log.i("EX", "3");
                    values[i] = temp;
                    Log.i("EX", "4");
                } catch(Exception e){
                    Log.i("EX", e.toString() + String.valueOf(i));
                    values[i] = patient.getTemperature(i);
                }
                if(values[i].matches(" ") || values[i].matches("")){
                    values[i] = getResources().getString(R.string.empty_register);
                }
            }
            for (int i = 0; i < number_of_values; i++) {
                try {
                    dates_converted[i] = MainActivity.dateFormat.parse(dates[i]);
                } catch (Exception e) { }
            }

            ArrayIndexComparator comparator = new ArrayIndexComparator(dates_converted);
            Integer[] indexes = comparator.createIndexArray();
            try {
                Arrays.sort(indexes, comparator);
            }catch (Exception e){}

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
                }catch (Exception e){}
            }

            adapter = new CustomMeasuresList(getActivity(), values_sorted, dates, 1);
        }

        list = (ListView) temp_reg_view.findViewById(R.id.temp_registers_listview);
        list.setAdapter(adapter);

        return temp_reg_view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof TempRegistersInteractionListener) {
            mListener = (TempRegistersInteractionListener) context;
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
