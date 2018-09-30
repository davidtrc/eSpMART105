package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.R;

public class About_valoriza_fragment extends Fragment {

    TextView valoriza_link;

    AboutValorizaInteractionListener mListener;

    public interface AboutValorizaInteractionListener {}

    public About_valoriza_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View rootView = inflater.inflate(R.layout.fragment_about_valoriza, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.aboutvaloriza_label);

        valoriza_link =  (TextView) rootView.findViewById(R.id.valoriza_link);
        valoriza_link.setClickable(true);
        valoriza_link.setMovementMethod(LinkMovementMethod.getInstance());
        String text = "<a href='http://www.valorizasm.com/'>"
                + getActivity().getApplicationContext().getResources().getString(R.string.click_valoriza_link)+ "</a>";
        valoriza_link.setText(Html.fromHtml(text));

        return rootView;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof AboutValorizaInteractionListener) {
            mListener = (AboutValorizaInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement AboutValorizaInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

}
