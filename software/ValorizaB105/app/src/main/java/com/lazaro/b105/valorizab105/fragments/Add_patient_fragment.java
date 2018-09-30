package com.lazaro.b105.valorizab105.fragments;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.provider.MediaStore;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.content.FileProvider;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Spinner;
import android.widget.Toast;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.Patient_DBHandler;
import com.lazaro.b105.valorizab105.R;
import com.lazaro.b105.valorizab105.Settings;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import static android.app.Activity.RESULT_OK;
import static android.text.TextUtils.concat;

import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.last_patient_ID;

public class Add_patient_fragment extends Fragment {

    ImageButton addPhotoPatient;
    String mCurrentPhotoPath = null;
    String photo_name;
    Spinner blood_type;
    Spinner gender;
    ImageButton rotate_left;
    ImageButton rotate_right;
    EditText patient_name;
    EditText clinical_history;
    EditText age;
    Button add_patient;
    EditText center;
    Patient_DBHandler patients_db = MainActivity.patients_DB;

    Bitmap patient_icon;

    private Handler handler = new Handler();
    private View rootView;

    static final int REQUEST_TAKE_PHOTO = 1;
    private static final int SELECT_FILE = 2;
    private static final int AGE_UPPER_LIMIT = 115;
    private static final int AGE_BOTTOM_LIMIT = 30;

    public static final String none_str = "ninguno";
    public static final String NEVER_str = "NEVER";

    AddPatientInteractionListener mListener;

    public interface AddPatientInteractionListener {}

    public Add_patient_fragment() {}


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        rootView = inflater.inflate(R.layout.fragment_add_patient, container, false);

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.add_patient);
        MainActivity.navigationView.getMenu().getItem(3).setChecked(true);

        /* Create the blood type spinner*/
        blood_type = (Spinner) rootView.findViewById(R.id.blood_type_spinner);
        ArrayAdapter<CharSequence> blood_type_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.blood_type_array, R.layout.custom_spinner_item);
        blood_type_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        blood_type.setAdapter(blood_type_adapter);

        /* Create the gender spinner*/
        gender = (Spinner) rootView.findViewById(R.id.gender_spinner);
        ArrayAdapter<CharSequence> gender_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.gender_array, R.layout.custom_spinner_item);
        gender_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        gender.setAdapter(gender_adapter);

        age = (EditText) rootView.findViewById(R.id.add_patient_age_field);
        center = (EditText) rootView.findViewById(R.id.add_patient_center_name_field);

        /* Create the photo frame of the patient and associate its click listener
        * also set visible the rotate buttons and its click listener*/
        addPhotoPatient = (ImageButton) rootView.findViewById(R.id.addPhotoPatient);
        addPhotoPatient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), rootView);

                final CharSequence[] items = {getContext().getResources().getString(R.string.take_photo), getContext().getResources().getString(R.string.choose_from_gallery)};
                android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(getActivity());
                builder.setTitle( getContext().getResources().getString(R.string.add_photo));
                builder.setItems(items, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int item) {

                        if (items[item].equals(getContext().getResources().getString(R.string.take_photo))) {
                            Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
                            // Ensure that there's a camera activity to handle the intent
                            if (takePictureIntent.resolveActivity(getActivity().getPackageManager()) != null) {
                                // Create the File where the photo should go
                                File photoFile = null;
                                try {
                                    if(mCurrentPhotoPath != null){
                                        File previous_photo = new File(mCurrentPhotoPath);
                                        previous_photo.delete();
                                    }
                                    photoFile = createImageFile();
                                } catch (IOException ex) {
                                    // Error occurred while creating the File
                                }
                                // Continue only if the File was successfully created
                                if (photoFile != null) {
                                    Uri photoURI = FileProvider.getUriForFile(getActivity().getApplicationContext(),
                                            "com.lazaro.b105.valorizab105",
                                            photoFile);
                                    takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT, photoURI);
                                    startActivityForResult(takePictureIntent, REQUEST_TAKE_PHOTO);
                                }
                            }
                        } else if (items[item].equals( getContext().getResources().getString(R.string.choose_from_gallery))) {
                            try {
                                if(mCurrentPhotoPath != null){
                                    File previous_photo = new File(mCurrentPhotoPath);
                                    previous_photo.delete();
                                }
                                createImageFile();
                            } catch (IOException ex) {
                                // Error occurred while creating the File
                            }

                            Intent takePictureIntent = new Intent();
                            takePictureIntent.setType("image/*");
                            takePictureIntent.setAction(Intent.ACTION_GET_CONTENT);
                            startActivityForResult(Intent.createChooser(takePictureIntent, "Select File"),SELECT_FILE);
                        }
                    }
                });
                builder.show();
            }
        });

        rotate_left = (ImageButton) rootView.findViewById(R.id.button_rotate_l);
        rotate_right = (ImageButton) rootView.findViewById(R.id.button_rotate_r);
        rotate_left.setVisibility(View.INVISIBLE);
        rotate_right.setVisibility(View.INVISIBLE);
        rotate_right.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), rootView);
                try {
                    File f= new File(mCurrentPhotoPath);
                    Bitmap b = BitmapFactory.decodeStream(new FileInputStream(f));
                    Matrix matrix = new Matrix();
                    matrix.postRotate(90);
                    patient_icon = Bitmap.createBitmap(patient_icon, 0, 0, patient_icon.getWidth(), patient_icon.getHeight(), matrix, true);
                    FileOutputStream out = null;
                    try {
                        out = new FileOutputStream(mCurrentPhotoPath);
                        patient_icon.compress(Bitmap.CompressFormat.PNG, 100, out);
                    } catch (Exception e) {
                        e.printStackTrace();
                    } finally {
                        try {
                            if (out != null) {
                                out.close();
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    addPhotoPatient.setImageBitmap(patient_icon);
                }
                catch (FileNotFoundException e){
                    e.printStackTrace();
                }
            }
        });

        rotate_left.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), rootView);
                try {
                    File f= new File(mCurrentPhotoPath);
                    Bitmap b = BitmapFactory.decodeStream(new FileInputStream(f));
                    Matrix matrix = new Matrix();
                    matrix.postRotate(270);
                    patient_icon = Bitmap.createBitmap(patient_icon, 0, 0, patient_icon.getWidth(), patient_icon.getHeight(), matrix, true);
                    FileOutputStream out = null;
                    try {
                        out = new FileOutputStream(mCurrentPhotoPath);
                        patient_icon.compress(Bitmap.CompressFormat.PNG, 100, out);
                    } catch (Exception e) {
                        e.printStackTrace();
                    } finally {
                        try {
                            if (out != null) {
                                out.close();
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    addPhotoPatient.setImageBitmap(patient_icon);
                }
                catch (FileNotFoundException e){
                    e.printStackTrace();
                }
            }
        });

        patient_name = (EditText) rootView.findViewById(R.id.add_patient_name_field);
        clinical_history = (EditText) rootView.findViewById(R.id.clinical_history_field);

        /* Add the functions to add a patient in the database */
        add_patient = (Button) rootView.findViewById(R.id.add_patient_button);
        add_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), rootView);

                if(patient_name.getText().toString().matches("")) {
                    CharSequence text = getContext().getResources().getString(R.string.name_not_empty);
                    int duration = Toast.LENGTH_SHORT;
                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else if (mCurrentPhotoPath == null) {
                    CharSequence text = getContext().getResources().getString(R.string.photo_not_empty);
                    int duration = Toast.LENGTH_LONG;
                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else if((Integer.parseInt(age.getText().toString())>AGE_UPPER_LIMIT) || (Integer.parseInt(age.getText().toString())<AGE_BOTTOM_LIMIT) ){
                    CharSequence text = getContext().getResources().getString(R.string.age_out_of_bounds);
                    CharSequence text2 = getContext().getResources().getString(R.string.and);
                    CharSequence text3 = " " +String.valueOf(AGE_BOTTOM_LIMIT)+ " ";
                    CharSequence text4 = " " +String.valueOf(AGE_UPPER_LIMIT)+ " ";
                    CharSequence text5 = concat(text, text3, text2, text4);
                    int duration = Toast.LENGTH_LONG;
                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), text5, duration);
                    toast.show();
                }
                else if(center.getText().toString().matches("")){
                    CharSequence text = getContext().getResources().getString(R.string.center_not_empty);
                    int duration = Toast.LENGTH_LONG;
                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else {
                    Patient new_p = new Patient();
                    new_p.setName(patient_name.getText().toString());
                    new_p.setGender(gender.getSelectedItem().toString());
                    new_p.setBlood_type(blood_type.getSelectedItem().toString());
                    new_p.setPhoto_id(mCurrentPhotoPath);
                    new_p.setEsp32_ID(none_str);
                    new_p.setAge(age.getText().toString());
                    new_p.setCenter(center.getText().toString());
                    new_p.setLast_connection_date(NEVER_str);
                    if(!clinical_history.getText().toString().matches("")){
                       new_p.setClinical_history(clinical_history.getText().toString());
                    }
                    long existence = patients_db.addPatient(new_p, last_patient_ID);
                    if(existence != -1) {
                        Settings settemp = MainActivity.settings_DB.getSettingsbyName(MainActivity.setting1);
                        settemp.setSettingValue(String.valueOf(existence));
                        MainActivity.settings_DB.updateSettings(settemp);
                        Content_main_fragment.last_patient_ID = String.valueOf(existence);

                        CharSequence text1 = getContext().getResources().getString(R.string.sucessfully_added);
                        int duration1 = Toast.LENGTH_LONG;
                        Toast toast1 = Toast.makeText(getActivity().getApplicationContext(), text1, duration1);
                        toast1.show();

                        Fragment fragment = null;
                        Class fragmentClass = Content_main_fragment.class;
                        try {
                            fragment = (Fragment) fragmentClass.newInstance();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                        FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
                        fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Content_main_fragment").commit();

                    } else if(existence == 0) {
                        CharSequence text2 = getContext().getResources().getString(R.string.pebble_already_associated);
                        int duration2 = Toast.LENGTH_LONG;
                        Toast toast2 = Toast.makeText(getActivity().getApplicationContext(), text2, duration2);
                        toast2.show();
                    } else {
                        CharSequence text2 = getContext().getResources().getString(R.string.fail_add_patient);
                        int duration2 = Toast.LENGTH_LONG;
                        Toast toast2 = Toast.makeText(getActivity().getApplicationContext(), text2, duration2);
                        toast2.show();
                    }

                }
            }
        });

        return rootView;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            if (requestCode == REQUEST_TAKE_PHOTO) {
                try {

                    File f = new File(mCurrentPhotoPath);
                    Bitmap b = BitmapFactory.decodeStream(new FileInputStream(f));
                    patient_icon = scaleDown(b, 600, false);
                    int width = patient_icon.getWidth();
                    int height = patient_icon.getHeight();
                    int crop = (Math.max(width, height) - Math.min(width, height))/2;
                    int final_side;
                    if(crop + height < patient_icon.getWidth()){
                        final_side = height;
                    } else {
                        final_side = patient_icon.getWidth()-crop;
                    }
                    patient_icon = Bitmap.createBitmap(patient_icon, crop, 0, final_side, final_side);

                    FileOutputStream out = null;
                    try {
                        out = new FileOutputStream(mCurrentPhotoPath);
                        patient_icon.compress(Bitmap.CompressFormat.JPEG, 100, out); // bmp is your Bitmap instance
                        // PNG is a lossless format, the compression factor (100) is ignored
                    } catch (Exception e) {
                        e.printStackTrace();
                    } finally {
                        try {
                            if (out != null) {
                                out.close();
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    addPhotoPatient.setImageBitmap(patient_icon);
                    rotate_right.setVisibility(View.VISIBLE);
                    rotate_left.setVisibility(View.VISIBLE);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                }
            } else if (requestCode == SELECT_FILE) {
                Bitmap bm = null;
                if (data != null) {
                    try {
                        bm = MediaStore.Images.Media.getBitmap(getActivity().getApplicationContext().getContentResolver(), data.getData());
                        patient_icon = scaleDown(bm, 600, false);
                        int width = patient_icon.getWidth();
                        int height = patient_icon.getHeight();
                        int crop = (Math.max(width, height) - Math.min(width, height))/2;
                        patient_icon = Bitmap.createBitmap(patient_icon, 0, 0, Math.min(patient_icon.getWidth(), 300), Math.min(patient_icon.getWidth(), 300));
                        FileOutputStream out = null;
                        try {
                            out = new FileOutputStream(mCurrentPhotoPath);
                            patient_icon.compress(Bitmap.CompressFormat.JPEG, 100, out); // bmp is your Bitmap instance
                            // PNG is a lossless format, the compression factor (100) is ignored
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            try {
                                if (out != null) {
                                    out.close();
                                }
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                        addPhotoPatient.setImageBitmap(patient_icon);
                        rotate_right.setVisibility(View.VISIBLE);
                        rotate_left.setVisibility(View.VISIBLE);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    private File createImageFile() throws IOException {
        // Create an image file name
        photo_name = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        String imageFileName = "JPEG_" + photo_name + "_";
        File storageDir = getActivity().getExternalFilesDir(Environment.DIRECTORY_PICTURES);
        File image = File.createTempFile(
                imageFileName,  /* prefix */
                ".jpg",         /* suffix */
                storageDir      /* directory */
        );

        // Save a file: path for use with ACTION_VIEW intents
        mCurrentPhotoPath = image.getAbsolutePath();
        return image;
    }

    public static Bitmap scaleDown(Bitmap realImage, float maxImageSize,
                                   boolean filter) {
        float ratio = Math.min(
                (float) maxImageSize / realImage.getWidth(),
                (float) maxImageSize / realImage.getHeight());
        int width = Math.round((float) ratio * realImage.getWidth());
        int height = Math.round((float) ratio * realImage.getHeight());

        Bitmap newBitmap = Bitmap.createScaledBitmap(realImage, width,
                height, filter);

        return newBitmap;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof AddPatientInteractionListener) {
            mListener = (AddPatientInteractionListener) context;
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


    public void setTitle(CharSequence title) {
        getActivity().getActionBar().setTitle(title);
    }

    public static void hideKeyboardFrom(Context context, View view) {
        InputMethodManager imm = (InputMethodManager) context.getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

}
