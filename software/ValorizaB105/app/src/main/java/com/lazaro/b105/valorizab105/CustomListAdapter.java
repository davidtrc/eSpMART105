package com.lazaro.b105.valorizab105;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.lazaro.b105.valorizab105.fragments.Edit_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.See_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.Show_results_db_fragment;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class CustomListAdapter extends ArrayAdapter<String> {

    private final Activity context;
    private final String[] names;
    private final String[] photopaths;
    private final String[] description;
    private final String[] ESP32IDS;
    private String search_text;
    private int search_method;

    public static Patient patient = null;

    private TextView txtTitle;
    private ImageView imageView;
    private TextView extratxt;

    public CustomListAdapter(Activity context, String[] names, String[] photopaths, String[] description, int search_method, String search_text, String[] ESP32IDS) {
        super(context, R.layout.listview_with_image, names);

        this.context = context;
        this.names = names;
        this.photopaths = photopaths;
        this.description = description;
        this.search_text = search_text;
        this.search_method = search_method;
        this.ESP32IDS = ESP32IDS;
    }

    public void showDialog(Context context, String title, CharSequence message, final int position) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);

        if (title != null) builder.setTitle(title);

        builder.setMessage(message);
        builder.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {

                Patient patient = MainActivity.patients_DB.getPatientByESP32_ID(ESP32IDS[position]);
                if(patient == null){
                    return;
                }

                MainActivity.patients_DB.deletePatient(patient);

                Fragment fragment = null;
                Class fragmentClass;
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
                FragmentManager fragmentManager =  ((FragmentActivity)getContext()).getSupportFragmentManager();
                //fragmentManager.popBackStack();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Show_results_db_fragment").commit();


            }
        });
        builder.setNegativeButton(R.string.no, null);
        builder.show();
    }

    public View getView(final int position, View view, ViewGroup parent) {
        LayoutInflater inflater=context.getLayoutInflater();
        View rowView=inflater.inflate(R.layout.listview_with_image, null,true);

        ImageButton see_patient = (ImageButton) rowView.findViewById(R.id.see_patient_button);
        ImageButton edit_patient = (ImageButton) rowView.findViewById(R.id.edit_patient_button);
        ImageButton remove_patient = (ImageButton) rowView.findViewById(R.id.remove_patient_button);



        see_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                patient = MainActivity.patients_DB.getPatientByESP32_ID(ESP32IDS[position]);

                Fragment fragment = null;
                Class fragmentClass = null;
                fragmentClass = See_patient_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                FragmentManager fragmentManager = ((FragmentActivity)getContext()).getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("See_patient_fragment").commit();
            }
        });

        edit_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                patient = MainActivity.patients_DB.getPatientByESP32_ID(ESP32IDS[position]);

                Fragment fragment = null;
                Class fragmentClass = Edit_patient_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }
                patient = MainActivity.patients_DB.getPatientByESP32_ID(ESP32IDS[position]);
                Bundle bundle = new Bundle();
                bundle.putInt("search_method", search_method);
                bundle.putString("search_text", search_text);
                fragment.setArguments(bundle);
                FragmentManager fragmentManager = ((FragmentActivity)getContext()).getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Edit_patient_fragment").commit();
            }
        });

        remove_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showDialog(getContext(), getContext().getResources().getString(R.string.delete_patient), getContext().getResources().getString(R.string.confirm_delete), position);
            }
        });

        String mCurrentPhotoPath = photopaths[position];
        File photo = new File(mCurrentPhotoPath);
        Bitmap b = null;
        Bitmap bcircular = null;
        try {
            b = BitmapFactory.decodeStream(new FileInputStream(photo));
            bcircular = ImageConverter.getRoundedCornerBitmap(b, 300);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        txtTitle = (TextView) rowView.findViewById(R.id.search_db_name);
        imageView = (ImageView) rowView.findViewById(R.id.icon);
        extratxt = (TextView) rowView.findViewById(R.id.search_db_pebbleid);

        txtTitle.setText(names[position]);
        imageView.setImageBitmap(bcircular);
        String description_str = getContext().getResources().getString(R.string.center) + " " + description[position];
        extratxt.setText(description_str);
        return rowView;

    };
}