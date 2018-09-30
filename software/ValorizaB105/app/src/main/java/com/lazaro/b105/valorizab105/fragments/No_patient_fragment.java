package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.lazaro.b105.valorizab105.R;

public class No_patient_fragment extends Fragment {
    public interface NoPatientInteractionListener {}

    private NoPatientInteractionListener mListener;
    private Button add_patient;

    public No_patient_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View no_patient_view = inflater.inflate(R.layout.fragment_no_patient, container, false);
        add_patient = (Button) no_patient_view.findViewById(R.id.no_patient_add_patient_button);

        add_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Fragment fragment = null;
                Class fragmentClass;
                fragmentClass = Add_patient_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }
                FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                //getActivity().getSupportFragmentManager().popBackStack(); innecesario?? davidtrc
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Add_patient_fragment").commit();
            }
        });
        return no_patient_view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof NoPatientInteractionListener) {
            mListener = (NoPatientInteractionListener) context;
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
