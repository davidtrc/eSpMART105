package com.lazaro.b105.valorizab105.fragments;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.provider.Settings;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.R;

public class Connect_valoriza_fragment extends Fragment {

    public interface ConnectValorizaInteractionListener {}

    private ConnectValorizaInteractionListener mListener;
    private TextView wifi_text;
    private TextView wifi_ssid;
    private ImageView wifi_icon;
    private Button wifi_button;
    private WifiManager wifiManager;
    private WifiInfo info;

    public Connect_valoriza_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View connect_valoriza_view = inflater.inflate(R.layout.fragment_connect__valoriza, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.connect_to_valoriza);
        MainActivity.navigationView.getMenu().getItem(1).setChecked(true);

        wifi_text = (TextView) connect_valoriza_view.findViewById(R.id.connected_to_wifi);
        wifi_ssid = (TextView) connect_valoriza_view.findViewById(R.id.wifi_ssid);
        wifi_icon = (ImageView) connect_valoriza_view.findViewById(R.id.wifi_icon);
        wifi_button = (Button) connect_valoriza_view.findViewById(R.id.connect_wifi_button);


        wifi_button.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
            }
        });

        return connect_valoriza_view;
    }

    @Override
    public void onResume(){
        super.onResume();

        wifiManager = (WifiManager) getActivity().getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        info = null;

        info = wifiManager.getConnectionInfo();
        String ssid = info.getSSID();
        wifi_ssid.setVisibility(View.VISIBLE);

        if (ssid.matches("0x")) {
            wifi_icon.setImageResource(R.drawable.wifi_unconnected);
            wifi_ssid.setVisibility(View.INVISIBLE);
            wifi_text.setText(R.string.unconnected_wifi);
            wifi_button.setText(R.string.connect_to_wifi);
        } else{
            wifi_icon.setImageResource(R.drawable.wifi_icon);
            wifi_text.setText(R.string.connected_to_wifi);
            wifi_button.setText(R.string.connect_to_another_wifi);
        }
        wifi_ssid.setText(ssid);
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof ConnectValorizaInteractionListener) {
            mListener = (ConnectValorizaInteractionListener) context;
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
