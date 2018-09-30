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

import com.lazaro.b105.valorizab105.CustomListAdapter;
import com.lazaro.b105.valorizab105.CustomSettingsList;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;

import java.util.ArrayList;

public class Show_results_db_fragment extends Fragment {

    ArrayList patients = null;
    Patient patient = null;
    CustomListAdapter adapter;

    private int search_method;
    private String search_text;

    private ListView list;
    private String names[];
    private String photodir[];
    private String centers[];
    private String ESP32IDS[];

    private View results_db;

    public interface ResultsDBInteractionListener {}

    private ResultsDBInteractionListener mListener;

    public Show_results_db_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        results_db = inflater.inflate(R.layout.fragment_show_results_db, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.db_results);

        try {
            search_method = getArguments().getInt("search_method");
            search_text = getArguments().getString("search_text");
        }catch (Exception e){
            Log.d("search", "was null");
        }

        switch (search_method){
            case 1:
                patients = MainActivity.patients_DB.getPatientbyName(search_text);
                break;
            case 2:
                patients = MainActivity.patients_DB.getPatientbyCenter(search_text);
                break;
            default:
                patients = null;
                patient =  null;
                break;
        }

        if(patients != null) {
            int number_of_patients = patients.size();

            names = new String[number_of_patients];
            photodir = new String[number_of_patients];
            centers = new String[number_of_patients];
            ESP32IDS = new String[number_of_patients];
            Patient temp_patient;
            for (int i = 0; i < number_of_patients; i++) {
                temp_patient = (Patient) patients.get(i);
                names[i] = temp_patient.getName();
                centers[i] = temp_patient.getCenter();
                photodir[i] = temp_patient.getPhoto_id();
                ESP32IDS[i] = temp_patient.getEsp32_ID();
            }

            adapter = new CustomListAdapter(getActivity(), names, photodir, centers, search_method, search_text, ESP32IDS);
            list = (ListView) results_db.findViewById(R.id.db_results_list);
            list.setAdapter(adapter);
        }
        if (patient != null){
            adapter = new CustomListAdapter(getActivity(), new String[]{patient.getName()},
                    new String[]{patient.getPhoto_id()}, new String[]{patient.getCenter()}, search_method, search_text, new String[]{patient.getEsp32_ID()});
            list = (ListView) results_db.findViewById(R.id.db_results_list);
            list.setAdapter(adapter);
        }

        if(patient == null && patients == null){
            CustomSettingsList adapter2;
            adapter2 = new CustomSettingsList(getActivity(),new String[]{getContext().getResources().getString(R.string.no_patients)},
                    new String[]{" "} );
            list = (ListView) results_db.findViewById(R.id.db_results_list);
            list.setAdapter(adapter2);
        }

        return results_db;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof ResultsDBInteractionListener) {
            mListener = (ResultsDBInteractionListener) context;
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
