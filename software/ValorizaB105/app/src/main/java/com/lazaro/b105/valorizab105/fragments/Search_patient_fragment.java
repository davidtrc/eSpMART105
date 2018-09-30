package com.lazaro.b105.valorizab105.fragments;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.Patient_DBHandler;
import com.lazaro.b105.valorizab105.R;

import java.util.ArrayList;

public class Search_patient_fragment extends Fragment {
    private EditText db_query;
    private Button search_by_name;
    private Button search_by_pebbleid;
    private View search_view;
    public static ArrayList patients = null;
    public static Patient patient;
    public static int search_method;
    public static String search_text;

    private SearchPatientInteractionListener mListener;

    public interface SearchPatientInteractionListener {}

    public Search_patient_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        search_view = inflater.inflate(R.layout.fragment_search_patient, container, false);

        db_query = (EditText) search_view.findViewById(R.id.db_query_field);
        search_by_name = (Button) search_view.findViewById(R.id.search_name_button);
        search_by_pebbleid = (Button) search_view.findViewById(R.id.search_pebbleid_button);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.search_patient);
        MainActivity.navigationView.getMenu().getItem(2).setChecked(true);

        search_by_name.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int duration = Toast.LENGTH_SHORT;
                CharSequence text = getContext().getResources().getString(R.string.name_not_empty2);
                Toast toast;

                if(db_query.getText().toString().matches("")){
                    toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                    return;
                }
                hideKeyboardFrom(getActivity().getApplicationContext(), search_view);
                patients = null;
                Patient_DBHandler patients_db = MainActivity.patients_DB;
                patients = patients_db.getPatientbyName(db_query.getText().toString());
                if(patients == null){
                    text = getContext().getResources().getString(R.string.patient_doesnt_exist);
                    toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else {
                    search_method = 1;
                    search_text = db_query.getText().toString();
                    Fragment fragment = null;
                    Class fragmentClass = Show_results_db_fragment.class;
                    try {
                        fragment = (Fragment) fragmentClass.newInstance();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    Bundle bundle = new Bundle();
                    bundle.putInt("search_method", search_method);
                    bundle.putString("search_text", search_text);
                    fragment.setArguments(bundle);
                    FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                    fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Show_results_db_fragment").commit();

                }
            }
        });

        search_by_pebbleid.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int duration = Toast.LENGTH_SHORT;
                CharSequence text = getContext().getResources().getString(R.string.center_not_empty);
                Toast toast;

                if(db_query.getText().toString().matches("")){
                    toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                    return;
                }
                hideKeyboardFrom(getActivity().getApplicationContext(), search_view);
                patient = null;
                Patient_DBHandler patients_db = MainActivity.patients_DB;
                patients = patients_db.getPatientbyCenter(db_query.getText().toString());
                if(patients == null){
                    text = getContext().getResources().getString(R.string.patient_doesnt_exist);
                    toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else {
                    search_method = 2;
                    search_text = db_query.getText().toString();
                    Fragment fragment = null;
                    Class fragmentClass = null;
                    fragmentClass = Show_results_db_fragment.class;
                    try {
                        fragment = (Fragment) fragmentClass.newInstance();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    Bundle bundle = new Bundle();
                    bundle.putInt("search_method", search_method);
                    bundle.putString("search_text", search_text);
                    fragment.setArguments(bundle);
                    FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                    fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Show_results_db_fragment").commit();

                    ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.db_results);

                }
            }
        });

        return search_view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof SearchPatientInteractionListener) {
            mListener = (SearchPatientInteractionListener) context;
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

    public static void hideKeyboardFrom(Context context, View view) {
        InputMethodManager imm = (InputMethodManager) context.getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

}
