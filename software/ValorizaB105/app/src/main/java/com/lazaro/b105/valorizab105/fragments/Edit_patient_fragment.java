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
import android.provider.MediaStore;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.content.FileProvider;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.CardView;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.lazaro.b105.valorizab105.CustomListAdapter;
import com.lazaro.b105.valorizab105.ImageConverter;
import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import static android.app.Activity.RESULT_OK;

public class Edit_patient_fragment extends Fragment {

    private Patient patient_to_show = CustomListAdapter.patient;
    String mCurrentPhotoPath = null;
    Bitmap patient_icon;
    String photo_name;

    private ImageButton patient_photo;
    private Spinner gender = null;
    private Spinner blood_type = null;
    private TextView pebble_id =  null;
    private TextView patient_name = null;
    private TextView clinical_history = null;
    private TextView  age_text = null;
    private TextView center_text =  null;
    private Button edit_falls_form = null;
    ImageButton rotate_left = null;
    ImageButton rotate_right = null;
    Button update_patient = null;
    int new_photo = 0;

    private int search_method;
    private String search_text;

    private View edit_patient_view;
    static final int REQUEST_TAKE_PHOTO = 1;
    private static final int SELECT_FILE = 2;

    public interface EditPatientInteractionListener {}

    private EditPatientInteractionListener mListener;

    public Edit_patient_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        edit_patient_view = inflater.inflate(R.layout.fragment_edit_patient, container, false);
        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.edit_patient);

        search_method = getArguments().getInt("search_method");
        search_text = getArguments().getString("search_text");

        if(patient_to_show != null){
            gender = (Spinner) edit_patient_view.findViewById(R.id.edit_gender_spinner);
            blood_type = (Spinner) edit_patient_view.findViewById(R.id.edit_blood_type_spinner);
            pebble_id =  (TextView) edit_patient_view.findViewById(R.id.edit_Pebble_ID);
            patient_name = (TextView) edit_patient_view.findViewById(R.id.edit_add_patient_name_field);
            patient_photo = (ImageButton) edit_patient_view.findViewById(R.id.edit_addPhotoPatient);
            clinical_history = (TextView) edit_patient_view.findViewById(R.id.edit_clinical_history_field);
            rotate_left = (ImageButton) edit_patient_view.findViewById(R.id.edit_button_rotate_l);
            rotate_right = (ImageButton) edit_patient_view.findViewById(R.id.edit_button_rotate_r);
            update_patient = (Button) edit_patient_view.findViewById(R.id.edit_add_patient_button);
            age_text = (TextView) edit_patient_view.findViewById(R.id.edit_patient_age_field);
            center_text = (TextView) edit_patient_view.findViewById(R.id.edit_patient_center_name_field);
            edit_falls_form = (Button) edit_patient_view.findViewById(R.id.edit_falls_form_button);
        }
        setPatientToShow(patient_to_show);

        rotate_left.setVisibility(View.INVISIBLE);
        rotate_right.setVisibility(View.INVISIBLE);
        rotate_right.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), edit_patient_view);
                try {
                    File f= new File(mCurrentPhotoPath);
                    Bitmap b = BitmapFactory.decodeStream(new FileInputStream(f));
                    Matrix matrix = new Matrix();
                    matrix.postRotate(90);
                    patient_icon = Bitmap.createBitmap(patient_icon, 0, 0, patient_icon.getWidth(), patient_icon.getHeight(), matrix, true);
                    FileOutputStream out = null;
                    try {
                        out = new FileOutputStream(mCurrentPhotoPath);
                        patient_icon.compress(Bitmap.CompressFormat.PNG, 100, out); // bmp is your Bitmap instance
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
                    patient_photo.setImageBitmap(patient_icon);
                }
                catch (FileNotFoundException e){
                    e.printStackTrace();
                }
            }
        });

        edit_falls_form.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Patient patient;
                Bundle bundle = new Bundle();
                if(patient_to_show != null){
                    patient = patient_to_show;
                    bundle.putInt("ID", patient.getId() );
                }

                Fragment fragment = null;
                Class fragmentClass = Falls_form_fragment.class;
                try {
                    fragment = (Fragment) fragmentClass.newInstance();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                fragment.setArguments(bundle);
                FragmentManager fragmentManager = ((FragmentActivity)getContext()).getSupportFragmentManager();
                fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Falls_form_fragment").commit();
            }
        });

        rotate_left.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), edit_patient_view);
                try {
                    File f= new File(mCurrentPhotoPath);
                    Bitmap b = BitmapFactory.decodeStream(new FileInputStream(f));
                    Matrix matrix = new Matrix();
                    matrix.postRotate(270);
                    patient_icon = Bitmap.createBitmap(patient_icon, 0, 0, patient_icon.getWidth(), patient_icon.getHeight(), matrix, true);
                    FileOutputStream out = null;
                    try {
                        out = new FileOutputStream(mCurrentPhotoPath);
                        patient_icon.compress(Bitmap.CompressFormat.PNG, 100, out); // PNG is a lossless format, the compression factor (100) is ignored
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
                    patient_photo.setImageBitmap(patient_icon);
                }
                catch (FileNotFoundException e){
                    e.printStackTrace();
                }
            }
        });

        patient_photo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), edit_patient_view);


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
                            //PROFILE_PIC_COUNT = 1;
                            //Intent intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
                            //startActivityForResult(intent, SELECT_FILE);
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

                            //startActivityForResult(takePictureIntent, SELECT_FILE);

                        }
                    }
                });
                builder.show();
            }
        });

        update_patient.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboardFrom(getActivity().getApplicationContext(), edit_patient_view);

                if(patient_name.getText().toString().matches("")) {
                    CharSequence text = getContext().getResources().getString(R.string.name_not_empty);
                    int duration = Toast.LENGTH_SHORT;
                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), text, duration);
                    toast.show();
                } else {
                    Patient new_p = MainActivity.patients_DB.getPatientByESP32_ID(patient_to_show.getEsp32_ID());
                    if(new_p != null) {
                        new_p.setName(patient_name.getText().toString());
                        new_p.setGender(gender.getSelectedItem().toString());
                        new_p.setBlood_type(blood_type.getSelectedItem().toString());
                        if(new_photo == 1) {
                            new_p.setPhoto_id(mCurrentPhotoPath);
                        }
                        new_p.setClinical_history(clinical_history.getText().toString());
                        new_p.setAge(age_text.getText().toString());
                        new_p.setCenter(center_text.getText().toString());

                        int result = MainActivity.patients_DB.updatePatient(new_p);
                        if (result == 1) {
                            CharSequence text1 = getContext().getResources().getString(R.string.sucessfully_updated);
                            int duration1 = Toast.LENGTH_LONG;
                            Toast toast1 = Toast.makeText(getActivity().getApplicationContext(), text1, duration1);
                            toast1.show();

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

                        } else {
                            CharSequence text2 = getContext().getResources().getString(R.string.error_updating);
                            int duration2 = Toast.LENGTH_LONG;
                            Toast toast2 = Toast.makeText(getActivity().getApplicationContext(), text2, duration2);
                            toast2.show();
                        }
                    }
                }
            }
        });

        return edit_patient_view;
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

    public void setPatientToShow(Patient patient){

        if(gender ==  null || blood_type == null || pebble_id == null || patient_name == null ||
                 patient_photo == null || clinical_history == null || age_text == null || center_text ==  null ){
            Fragment fragment = null;
            Class fragmentClass = null;
            fragmentClass = See_patient_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }

            FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("See_patient_fragment").commit();

            gender = (Spinner) edit_patient_view.findViewById(R.id.edit_gender_spinner);
            blood_type = (Spinner) edit_patient_view.findViewById(R.id.edit_blood_type_spinner);
            pebble_id =  (TextView) edit_patient_view.findViewById(R.id.edit_Pebble_ID);
            patient_name = (TextView) edit_patient_view.findViewById(R.id.edit_add_patient_name_field);
            patient_photo = (ImageButton) edit_patient_view.findViewById(R.id.edit_addPhotoPatient);
            clinical_history = (TextView) edit_patient_view.findViewById(R.id.edit_clinical_history_field);
            age_text = (TextView) edit_patient_view.findViewById(R.id.edit_patient_age_field);
            center_text = (TextView) edit_patient_view.findViewById(R.id.edit_patient_center_name_field);
            edit_falls_form = (Button) edit_patient_view.findViewById(R.id.edit_falls_form_button);
        }

        ArrayAdapter<CharSequence> blood_type_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.blood_type_array, R.layout.custom_spinner_item);
        blood_type_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        blood_type.setAdapter(blood_type_adapter);
        String blood = patient.getBlood_type();
        int spinnerPosition = blood_type_adapter.getPosition(blood);
        blood_type.setSelection(spinnerPosition);

        ArrayAdapter<CharSequence> gender_adapter = ArrayAdapter.createFromResource(getActivity().getApplicationContext(),
                R.array.gender_array, R.layout.custom_spinner_item);
        gender_adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        gender.setAdapter(gender_adapter);
        String gender2 = patient.getGender();
        int spinnerPosition2 = gender_adapter.getPosition(gender2);
        gender.setSelection(spinnerPosition2);

        pebble_id.setText(getResources().getString(R.string.pebble_ID) + " " + patient_to_show.getEsp32_ID());
        patient_name.setText(patient.getName());
        age_text.setText(patient.getAge());
        center_text.setText(patient.getCenter());
        String history = null;
        if(patient_to_show.getClinical_history() != null){
            history = patient_to_show.getClinical_history();
        }
        if(history == null || history.matches("") || history.matches(" ") || history.matches("null") ){
            clinical_history.setText(getResources().getString(R.string.no_history));
        } else {
            clinical_history.setText(history);
        }
        clinical_history.setMovementMethod(new ScrollingMovementMethod());
        String mCurrentPhotoPath = patient.getPhoto_id();
        File photo = new File(mCurrentPhotoPath);
        Bitmap b = null;
        //Bitmap bcircular = null;
        try {
            b = BitmapFactory.decodeStream(new FileInputStream(photo));
            //bcircular = ImageConverter.getRoundedCornerBitmap(b, 300);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        patient_photo.setImageBitmap(b);

    }

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
                    patient_photo.setImageBitmap(patient_icon);
                    rotate_right.setVisibility(View.VISIBLE);
                    rotate_left.setVisibility(View.VISIBLE);
                    new_photo = 1;
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
                        patient_photo.setImageBitmap(patient_icon);
                        rotate_right.setVisibility(View.VISIBLE);
                        rotate_left.setVisibility(View.VISIBLE);
                        new_photo = 1;
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
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
        if (context instanceof EditPatientInteractionListener) {
            mListener = (EditPatientInteractionListener) context;
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
