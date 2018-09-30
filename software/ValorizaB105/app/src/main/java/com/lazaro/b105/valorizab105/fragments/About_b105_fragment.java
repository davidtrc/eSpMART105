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

public class About_b105_fragment extends Fragment {

    TextView b105_link;

    private AboutB105InteractionListener mListener;

    public interface AboutB105InteractionListener {}

    public About_b105_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View b105View = inflater.inflate(R.layout.fragment_about_b105, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.aboutb105_label);

        b105_link =  (TextView) b105View.findViewById(R.id.b105_link);
        b105_link.setClickable(true);
        b105_link.setMovementMethod(LinkMovementMethod.getInstance());
        String text = "<a href='http://elb105.com/'>"
                + getActivity().getApplicationContext().getResources().getString(R.string.click_b105_link)+ "</a>";
        b105_link.setText(Html.fromHtml(text));

        return b105View;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof AboutB105InteractionListener) {
            mListener = (AboutB105InteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
    }

    /*
    @Override
    public void onBackPressed() {

        DrawerLayout drawer = (DrawerLayout) getView().findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
            return;
        }
        getActivity().getSupportFragmentManager().popBackStack(null, FragmentManager.POP_BACK_STACK_INCLUSIVE);
        Fragment fragment = null;
        Class fragmentClass = Content_main_fragment.class;
        try {
            fragment = (Fragment) fragmentClass.newInstance();
        } catch (Exception e) {
            e.printStackTrace();
        }
        FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
        fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack(null).commit();
    }
    */

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

}
