package com.lazaro.b105.valorizab105;

import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;

import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.lazaro.b105.valorizab105.fragments.About_b105_fragment;
import com.lazaro.b105.valorizab105.fragments.Add_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.Connect_ESP32_fragment;
import com.lazaro.b105.valorizab105.fragments.Connect_valoriza_fragment;
import com.lazaro.b105.valorizab105.fragments.Content_main_fragment;
import com.lazaro.b105.valorizab105.fragments.About_valoriza_fragment;
import com.lazaro.b105.valorizab105.fragments.Edit_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.Hr_registers_fragment;
import com.lazaro.b105.valorizab105.fragments.More_info_fragment;
import com.lazaro.b105.valorizab105.fragments.No_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.Sat_registers_fragment;
import com.lazaro.b105.valorizab105.fragments.Search_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.See_patient_fragment;
import com.lazaro.b105.valorizab105.fragments.Settings_fragment;
import com.lazaro.b105.valorizab105.fragments.Show_results_db_fragment;
import com.lazaro.b105.valorizab105.fragments.Steps_registers_fragment;
import com.lazaro.b105.valorizab105.fragments.Temp_registers_fragment;
import com.lazaro.b105.valorizab105.fragments.Falls_form_fragment;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import rx.Observable;
import rx.Subscriber;
import rx.android.schedulers.AndroidSchedulers;

import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT32;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT16;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.ESP32_TH;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.esp32_battery_alarm;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.hr_alarm;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.hr_var_alarm;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.ht_alarm;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.last_patient_ID;
import static com.lazaro.b105.valorizab105.fragments.Content_main_fragment.po_alarm;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener,
        About_valoriza_fragment.AboutValorizaInteractionListener,
        Content_main_fragment.OnFragmentInteractionListener,
        Add_patient_fragment.AddPatientInteractionListener,
        Search_patient_fragment.SearchPatientInteractionListener,
        Show_results_db_fragment.ResultsDBInteractionListener,
        About_b105_fragment.AboutB105InteractionListener,
        Settings_fragment.SettingsInteractionListener,
        See_patient_fragment.SeePatientInteractionListener,
        Edit_patient_fragment.EditPatientInteractionListener,
        No_patient_fragment.NoPatientInteractionListener,
        Connect_valoriza_fragment.ConnectValorizaInteractionListener,
        Connect_ESP32_fragment.ConnectESP32InteractionListener,
        Temp_registers_fragment.TempRegistersInteractionListener,
        Hr_registers_fragment.HrRegistersInteractionListener,
        Steps_registers_fragment.StepsRegistersInteractionListener,
        Sat_registers_fragment.SatRegistersInteractionListener,
        More_info_fragment.MoreInfoInteractionListener,
        Falls_form_fragment.FallsFormFragmentInteractionListener{

    /**** TODO
     * --Find near patients view is not developed. If an ESP32 device is found and we are in this view, that device should be added to the adapter
     * --Steps count could be reset everyday at 0:00, the addition of the lectures of all the day, stored in the database as a one.make
     * --When receiving measures, if the amount of measures is equals to the maximum array size of the ESP32 it is possible that an overflow has happend. In that case,
     * the order of the measures received could not be from the newer one to the oldest one. In that case, the firsts 4 bytes of OBJ SIZE BT characteristic indicates
     * the overflow in the array and MUST be considered.
     *****/

    public static Patient_DBHandler  patients_DB;
    public static Settings_DBHandler settings_DB;
    public static NavigationView navigationView;
    public static Patient patient_on_load;

    private final static char[] hexArray = "0123456789ABCDEF".toCharArray();
    public static final String setting1 = "Último paciente conectado";
    public static final String setting2 = "Medir temp. cada:";
    public static final String setting3 = "Medir sat. cada:";
    public static final String setting4 = "Medir pulso cada:";
    public static final String setting6 = "Modo de uso de la aplicación";
    public static final String setting7 = "Máximo valor de ritmo cardíaco";
    public static final String setting8 = "Mínimo valor de ritmo cardíaco";
    public static final String setting9 = "Máximo valor de variabilidad de ritmo cardíaco";
    public static final String setting10 = "Mínimo nivel de saturación";
    public static final String setting11 = "Máximo nivel de temperatura corporal";
    public static final String setting12 = "Mínimo nivel de temperatura corporal";


    private static boolean comm_started = false;
    private static boolean comm_finished = false;

    /* BLE */
    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    private BluetoothGatt mBluetoothGatt;
    List<BluetoothDevice> foundevices  = new ArrayList<>();
    BluetoothDevice eSpMART105;
    boolean eSpMART105_discovered = false;
    List<BluetoothGattService> gattServices;
    Map<UUID, List<BluetoothGattCharacteristic>> gattServandChar;
    Map<UUID, List<BluetoothGattDescriptor>> gattCharandDescr;

    public static int hr_up_th;
    public static int hr_bt_th;
    public static float hr_var_up_th;
    public static float ht_up_th;
    public static float ht_bt_th;
    public static int po_bt_th;

    private Patient Patient_BT;
    private static String current_esp32_name;
    private static String HR_SRV = "0000180d";
    private static String HR_MEAS_CHAR = "00002a37";
    public static int hr_meas_esp_flags = 0;
    public static List<Integer> hr_meas_esp_value = new ArrayList<>();
    public static List<Float> hr_variability_esp_value = new ArrayList<>();
    private static String HR_MI_CHAR = "00002a21";
    public static int hr_mi_esp_value = 0;
    public static int hr_obj_size_esp_max_value = 0;
    public static int hr_obj_size_esp_value = 0;
    private static String HR_VARIABLITY_CHAR = "00002adb";
    private static String HR_OBJ_SIZE_CHAR = "00002ac0";
    private boolean is_hr_subscribed = false;

    private static String HT_SRV = "00001809";
    private static String HT_MEAS_CHAR = "00002a1c";
    public static int ht_meas_esp_flags = 0;
    public static List<Float> ht_meas_esp_value = new ArrayList<>();
    private static String HT_MI_CHAR = "00002a21";
    public static int ht_mi_esp_value = 0;
    private static String HT_OBJ_SIZE_CHAR = "00002ac0";
    public static int ht_obj_size_esp_number_samples = 0;
    public static int ht_obj_size_esp_overflow = 0;
    private boolean is_ht_subscribed = false;

    private static String PO_SRV = "00001822";
    private static String PO_MEAS_CHAR = "00002a5e";
    private static String PO_MI_CHAR = "00002a21";
    private static String PO_OBJ_SIZE_CHAR = "00002ac0";
    public static List<Byte> po_meas_esp_value = new ArrayList<>();
    public static int po_mi_esp_value = 0;
    public static int po_obj_size_esp_max_value = 0;
    public static int po_obj_size_esp_value = 0;
    public static float po_meas_esp_pr = 0;
    public static byte[] po_meas_esp_measurement_status = new byte[2];
    public static int po_meas_esp_flags = 0;
    public static byte[] po_meas_esp_device_and_sensor_status = new byte[3];

    private static String FS_IDX_SVC = "00001811";
    private static String FS_FALL_DETECTED_CHAR = "00002a46";
    public static float esp32_percentage = 0;

    private static String SS_IDX_SVC = "00001826";
    private static String SS_STEPS_CHAR = "00002acf";

    public static DateFormat dateFormat = new SimpleDateFormat("HH:mm:ss    dd/MM/yyyy");

    //Device and Sensor Status(3), Measurement Status(2), PR(2), Sp02(2), Flags(1)
    private boolean is_po_subscribed = false;

    private static String BS_SRV = "0000180f";
    private static String BS_MEAS_CHAR = "00002a19";
    private static int esp32_battery_level = 0;
    private static int steps_number = 0;
    private static int times_requested_falls = 0;

    private static final int TIMEOUT_SCAN = 5; //Seconds, retrying scan
    private static final int TIMEOUT_TO_DISCOVER_SERVICES = 2000;
    private static final int DELAY_BETWEEN_BT_COMMANDS_MS = 80;
    private static final int DELAY_TO_START_BT_PACKETS_PROCESSING_MS = 10000;
    private static final int STATE_DISCONNECTED = 0;
    /* BLE */

    private Handler mHandler;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        patients_DB = new Patient_DBHandler(this);
        settings_DB = new Settings_DBHandler(this);
        Settings set;

        initDbs();

        try{
            set = settings_DB.getSettingsbyName(setting7);
            hr_up_th = Integer.parseInt(set.getSetting_value());
        }catch (Exception e){
            hr_up_th = 95;
        }
        try{
            set = settings_DB.getSettingsbyName(setting8);
            hr_bt_th = Integer.parseInt(set.getSetting_value());
        }catch (Exception e){
            hr_bt_th = 55;
        }
        try{
            set = settings_DB.getSettingsbyName(setting9);
            hr_var_up_th = Float.parseFloat(set.getSetting_value());
        }catch (Exception e){
            hr_var_up_th = (float) 200;
        }
        try{
            set = settings_DB.getSettingsbyName(setting10);
            po_bt_th = Integer.parseInt(set.getSetting_value());
        }catch (Exception e){
            po_bt_th = 95;
        }
        try{
            set = settings_DB.getSettingsbyName(setting11);
            ht_up_th = Float.parseFloat(set.getSetting_value());
        }catch (Exception e){
            ht_up_th = (float) 38.8;
        }
        try{
            set = settings_DB.getSettingsbyName(setting12);
            ht_bt_th = Float.parseFloat(set.getSetting_value());
        }catch (Exception e){
            ht_up_th = (float) 35;
        }

        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        if (savedInstanceState != null) {
            return;
        }

        Fragment fragment = null;
        Class fragmentClass;

        patient_on_load = null;
        last_patient_ID = settings_DB.getSettingsbyName(MainActivity.setting1).getSetting_value();
        try{
            patient_on_load = MainActivity.patients_DB.getPatientByESP32_ID(last_patient_ID);
        }catch (Exception e){}
        if(patient_on_load == null){
            for(int i=0; i<100; i++){
                patient_on_load = MainActivity.patients_DB.getPatientbyId(i);
                if(patient_on_load != null){
                    last_patient_ID = patient_on_load.getEsp32_ID();
                    settings_DB.getSettingsbyName(MainActivity.setting1).setSettingValue(last_patient_ID);
                    break;
                }
            }
        }

        if(patient_on_load == null){
            fragmentClass = Content_main_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
            FragmentManager fragmentManager = getSupportFragmentManager();
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).commit();
            fragmentClass = No_patient_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack(null).commit();
        } else{
            fragmentClass = Content_main_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
            FragmentManager fragmentManager = getSupportFragmentManager();
            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).commit();
        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.setDrawerListener(toggle);
        toggle.syncState();

        navigationView = (NavigationView) findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        getSupportActionBar().setTitle(R.string.main_label);

        gattServandChar = new HashMap<>();
        gattCharandDescr = new HashMap<>();
        mHandler = new Handler();

        btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        btScanner = btAdapter.getBluetoothLeScanner();
        btScanner.startScan(leScanCallback);

        find_eSpMART105();
    }

    private void initDbs(){
        Patient p2 = patients_DB.getPatientbyId(1);
        String path = "directorio";

        settings_DB.addSetting(setting1," ");
        settings_DB.addSetting(setting2,"60");
        settings_DB.addSetting(setting3, "60");
        settings_DB.addSetting(setting4,"60");
        settings_DB.addSetting(setting6,"1"); //was "Administrador" instead of 1
        settings_DB.addSetting(setting7, "95");
        settings_DB.addSetting(setting8, "55");
        settings_DB.addSetting(setting9, "200");
        settings_DB.addSetting(setting10, "95");
        settings_DB.addSetting(setting11, "38");
        settings_DB.addSetting(setting12, "35.5");

        if(p2 != null){
            path = p2.getPhoto_id();

            Patient p1 = new Patient();
            p1.setName("Juan Enrique García");
            p1.setPhoto_id(path);
            p1.setGender("Hombre");
            p1.setEsp32_ID("DEFAULT");
            p1.setBlood_type("A+");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            patients_DB.addPatient(p1, "DEFAULT");

            p1.setName("Juan Menéndez Valdés");
            p1.setGender("Hombre");
            p1.setEsp32_ID("DEFAULT2");
            p1.setBlood_type("AB+");
            p1.setAge("32");
            p1.setCenter("El arroyuelo");
            patients_DB.addPatient(p1, "DEFAULT2");

            p1.setName("Ernesto Mejía García");
            p1.setGender("Hombre");
            p1.setEsp32_ID("DEFAULT3");
            p1.setBlood_type("0-");
            p1.setAge("32");
            p1.setCenter("Vegetaltown");
            patients_DB.addPatient(p1, "DEFAULT3");

            p1.setName("Mercedes Concepción de Urrutia");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT4");
            p1.setBlood_type("B+");
            p1.setAge("32");
            p1.setCenter("Veteranos de Vietnam");
            patients_DB.addPatient(p1, "DEFAULT4");

            p1.setName("Felisa Sotomayor Fernández");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT5");
            p1.setBlood_type("B-");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            patients_DB.addPatient(p1, "DEFAULT5");

            p1.setName("Felisa Sotomayor García");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT6");
            p1.setBlood_type("B+");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            patients_DB.addPatient(p1, "DEFAULT6");

            p1.setName("Felisa Ruiz Sanchez");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT7");
            p1.setBlood_type("B+");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            patients_DB.addPatient(p1, "DEFAULT7");

            p1.setName("Felisa Sotomayor González");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT8");
            p1.setBlood_type("B+");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            patients_DB.addPatient(p1, "DEFAULT8");

            p1.setName("Felisa Sotomayor Barahona");
            p1.setGender("Mujer");
            p1.setEsp32_ID("DEFAULT9");
            p1.setBlood_type("B+");
            p1.setAge("32");
            p1.setCenter("Centro de Día");
            p1.setClinical_history("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam imperdiet sollicitudin odio eget euismod. " +
                    "Vestibulum vitae nisi eleifend, ornare justo vitae, fermentum ipsum. Cras lacinia ullamcorper arcu id pretium. " +
                    "Vestibulum ac quam velit. Donec odio massa, tempor facilisis mi nec, blandit egestas ante. Aenean nisi metus, " +
                    "cursus sed enim et, ultrices volutpat magna. Sed id justo accumsan, volutpat arcu mollis, congue turpis. " +
                    "Curabitur sed ipsum at ligula pellentesque ullamcorper. Suspendisse eu ante hendrerit tellus semper tempor " +
                    "nec maximus arcu. Praesent quis placerat lectus, quis malesuada tellus." + "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam imperdiet sollicitudin odio eget euismod. " +
                    "Vestibulum vitae nisi eleifend, ornare justo vitae, fermentum ipsum. Cras lacinia ullamcorper arcu id pretium. " +
                    "Vestibulum ac quam velit. Donec odio massa, tempor facilisis mi nec, blandit egestas ante. Aenean nisi metus, " +
                    "cursus sed enim et, ultrices volutpat magna. Sed id justo accumsan, volutpat arcu mollis, congue turpis. " +
                    "Curabitur sed ipsum at ligula pellentesque ullamcorper. Suspendisse eu ante hendrerit tellus semper tempor " +
                    "nec maximus arcu. Praesent quis placerat lectus, quis malesuada tellus." + "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam imperdiet sollicitudin odio eget euismod. " +
                    "Vestibulum vitae nisi eleifend, ornare justo vitae, fermentum ipsum. Cras lacinia ullamcorper arcu id pretium. " +
                    "Vestibulum ac quam velit. Donec odio massa, tempor facilisis mi nec, blandit egestas ante. Aenean nisi metus, " +
                    "cursus sed enim et, ultrices volutpat magna. Sed id justo accumsan, volutpat arcu mollis, congue turpis. " +
                    "Curabitur sed ipsum at ligula pellentesque ullamcorper. Suspendisse eu ante hendrerit tellus semper tempor " +
                    "nec maximus arcu. Praesent quis placerat lectus, quis malesuada tellus.");
            patients_DB.addPatient(p1, "DEFAULT9");
            patients_DB.updatePatient(p1);

            Patient p3 = patients_DB.getPatientByESP32_ID("C10000000008");
            //Patient p3 = patients_DB.getPatientByPebble_ID("C551396A0YV4");
            //p3.setTemperature("36.5 ºC");
            //p3.setTemperature("37 ºC");
            //p3.setSaturation("99%");
            //p3.setHeart_rate("100 ppm");
            //p3.setSteps("10.000");
            //patients_DB.updatePatient(p3);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return true;
    }

    @Override
    public void onBackPressed() {
        Fragment fragment = null;
        Class fragmentClass;
        FragmentManager fragmentManager = getSupportFragmentManager();

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
            return;
        }

        if(getSupportFragmentManager().getBackStackEntryCount() > 1){
            fragmentManager = getSupportFragmentManager();
            FragmentManager.BackStackEntry first = fragmentManager.getBackStackEntryAt(0);
            fragmentManager.popBackStack(first.getId(), FragmentManager.POP_BACK_STACK_INCLUSIVE);

            fragmentClass = Content_main_fragment.class;
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }

            fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack(null).commit();

        } else {
            super.onBackPressed();
        }
    }

    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        int id = item.getItemId();
        Fragment fragment = null;
        Class fragmentClass = null;
        String fr_name = "";
        FragmentManager fragmentManager = getSupportFragmentManager();

        if (id == R.id.nav_main){
            fragmentClass = Content_main_fragment.class;
            fr_name = "Content_main_fragment";
        } else if (id == R.id.nav_valoriza) {
            fragmentClass = Connect_valoriza_fragment.class;
            fr_name = "Connect_valoriza_fragment";
        } else if (id == R.id.nav_searchp) {
            fragmentClass = Search_patient_fragment.class;
            fr_name = "Search_patient_fragment";
        } else if (id == R.id.nav_addp) {
            fragmentClass = Add_patient_fragment.class;
            fr_name = "Add_patient_fragment";
        } else if (id == R.id.nav_settings) {
            fragmentClass = Settings_fragment.class;
            fr_name = "Settings_fragment";
        } else if (id == R.id.nav_aboutb105) {
            fragmentClass = About_b105_fragment.class;
            fr_name = "About_b105_fragment";
        } else if (id == R.id.nav_aboutvaloriza) {
            fragmentClass = About_valoriza_fragment.class;
            fr_name = "About_valoriza_fragment";
        }

        try {
            fragment = (Fragment) fragmentClass.newInstance();
        } catch (Exception e) {
            e.printStackTrace();
        }


        fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack(fr_name).commit();

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    public void find_eSpMART105(){
        Observable.timer(TIMEOUT_SCAN, TimeUnit.SECONDS)
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new Subscriber<Long>() {
                    @Override
                    public void onCompleted() {
                        btScanner.stopScan(leScanCallback);

                        for (BluetoothDevice td : foundevices) {
                            String btName = td.getName();
                            if(btName != null){
                                if (td.getName().startsWith("PATIENT")) {
                                    Log.i("TAG", "eSpMART found, trying to connect");
                                    eSpMART105_discovered = true;
                                    eSpMART105 = td;
                                    current_esp32_name = td.getName();
                                    break;
                                }
                            }
                        }
                        if(eSpMART105_discovered){
                            Patient_BT = MainActivity.patients_DB.getPatientByESP32_ID(current_esp32_name);
                            if(Patient_BT == null) {
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        //Create the alert dialog
                                        AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                                        builder.setTitle(MainActivity.this.getResources().getString(R.string.new_esp32));
                                        builder.setMessage(MainActivity.this.getResources().getString(R.string.esp32_not_associated));
                                        builder.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                                            @Override
                                            public void onClick(DialogInterface dialog, int which) {
                                                Patient_BT = MainActivity.patients_DB.getPatientByESP32_ID(last_patient_ID);
                                                if(Patient_BT != null) {
                                                    Patient_BT.setEsp32_ID(current_esp32_name);
                                                    MainActivity.patients_DB.updatePatient(Patient_BT);
                                                    mBluetoothGatt = eSpMART105.connectGatt(getApplicationContext(), true, mGattCallback);
                                                }
                                            }
                                        });
                                        builder.setNegativeButton(R.string.no, null);
                                        builder.show();
                                    }
                                });
                            } else {
                                mBluetoothGatt = eSpMART105.connectGatt(getApplicationContext(), true, mGattCallback);
                            }
                        } else{
                            foundevices.clear();
                            btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
                            btAdapter = btManager.getAdapter();
                            btScanner = btAdapter.getBluetoothLeScanner();
                            btScanner.startScan(leScanCallback);
                            find_eSpMART105();
                            Log.d("TAG", "eSpMART NOT found, restarting scan");
                        }

                    }

                    @Override
                    public void onError(Throwable e) {
                    }

                    @Override
                    public void onNext(Long aLong) {
                    }
                });
    }

    public byte[] format_new_object_size(int new_obj_size, int max_obj_size){

        byte[] temporal1;
        byte[] temporal2;
        byte[] temporal3 = new byte[8];
        temporal1 = ByteBuffer.allocate(4).putInt(new_obj_size).array();
        temporal2 = ByteBuffer.allocate(4).putInt(max_obj_size).array();
        temporal3[0] = temporal1[0];
        temporal3[1] = temporal1[1];
        temporal3[2] = temporal1[2];
        temporal3[3] = temporal1[3];
        temporal3[4] = temporal2[0];
        temporal3[5] = temporal2[1];
        temporal3[6] = temporal2[2];
        temporal3[7] = temporal2[3];
        return temporal3;

    }

    @Override
    public void setTitle(CharSequence title) {
        getActionBar().setTitle(title);
    }

    private ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice nd = result.getDevice();
            boolean exist = false;
            for (BluetoothDevice td : foundevices) {
                if (td.equals(nd)) {
                    exist = true;
                    break;
                }
            }
            if (!exist) {
                foundevices.add(nd);
            }
        }
    };

    // Various callback methods defined by the BLE API.
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.i("onConnectionStateChange", "Status: " + status);
            switch (newState) {
                case BluetoothProfile.STATE_CONNECTED:
                    Log.i("gattCallback", "STATE_CONNECTED");
                    Observable.timer(TIMEOUT_TO_DISCOVER_SERVICES, TimeUnit.MILLISECONDS)
                            .observeOn(AndroidSchedulers.mainThread())
                            .subscribe(new Subscriber<Long>() {
                                @Override
                                public void onCompleted() {
                                    gatt.discoverServices();
                                }

                                @Override
                                public void onError(Throwable e) {
                                }

                                @Override
                                public void onNext(Long aLong) {
                                }
                            });

                    //mBluetoothGatt.discoverServices();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    Log.e("gattCallback", "STATE_DISCONNECTED");
                    is_ht_subscribed = false;
                    is_hr_subscribed = false;
                    is_po_subscribed = false;
                    reset_ESP32_comm();
                    break;
                default:
                    Log.e("gattCallback", "STATE_OTHER");
            }
        }

        @Override
        // New services discovered
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            gattServices = mBluetoothGatt.getServices();

            if (status == BluetoothGatt.GATT_SUCCESS) {
                gattServices = mBluetoothGatt.getServices();
                Log.e("onServicesDiscovered", "Services count: "+gattServices.size());

                for (BluetoothGattService gattService : gattServices) {
                    UUID serviceUUID = gattService.getUuid();
                    List<BluetoothGattCharacteristic> gattCharacteristics = gattService.getCharacteristics();
                    gattServandChar.put(serviceUUID, gattCharacteristics );
                    Log.e("onServicesDiscovered", "Service uuid "+serviceUUID.toString());
                    //Log.e("onServicesDiscovered", "Characteristics "+gattCharacteristics.toString());
                    for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                        Log.e("onServicesDiscovered", "Characteristic uuid  "+gattCharacteristic.getUuid().toString());
                        Log.e("onServicesDiscovered", "Characteristic permissions  "+String.valueOf(gattCharacteristic.getPermissions()));
                        List<BluetoothGattDescriptor> gattDescriptors = gattCharacteristic.getDescriptors();
                        gattCharandDescr.put(gattCharacteristic.getUuid(), gattDescriptors);
                        switch(gattCharacteristic.getProperties()){
                            case 1:
                                Log.e("Properties ", "Broadcast");
                                break;
                            case 2:
                                Log.e("Properties ", "Read");
                                break;
                            case 4:
                                Log.e("Properties ", "Write without response");
                                break;
                            case 8:
                                Log.e("Properties ", "Write");
                                break;
                            case 16:
                                Log.e("Properties ", "Notify");
                                break;
                            case 26:
                                Log.e("Properties ", "Read, Write, Notify");
                                break;
                            case 32:
                                Log.e("Properties ", "Indicate");
                                break;
                            case 64:
                                Log.e("Properties ", "Authenticated Signed Write");
                                break;
                            case 128:
                                Log.e("Properties ", "Extended Properties");
                                break;
                            default:
                                break;

                        }
                        for (BluetoothGattDescriptor gattDescriptor : gattDescriptors) {
                            Log.e("onServicesDiscovered", "Characteristic descriptors  "+gattDescriptor.getUuid().toString());
                        }
                    }
                }
                Log.i("OnServicesDiscovered", "Calling start_eSpMART_comm_process function");

                read_falls();
                //start_eSpMART_comm_process();
                comm_started = true;

            } else {
                Log.w("OSD", "onServicesDiscovered received: " + status);
            }

        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                processTxQueue();
                Settings current_set;
                int current_set_value = 0;
                byte[] current_set_value_bits;

                if(characteristic.getUuid().toString().startsWith(FS_FALL_DETECTED_CHAR) && characteristic.getService().getUuid().toString().startsWith(FS_IDX_SVC)){
                    int fall_detected = characteristic.getIntValue(FORMAT_UINT8, 0);
                    if(fall_detected != 0){
                        get_fall_probability_and_notify(fall_detected);
                    }
                    times_requested_falls++;
                    if(times_requested_falls == 1){
                        start_eSpMART_comm_process();
                    }
                    if(times_requested_falls == 2){
                        times_requested_falls = 0;
                        comm_finished = true;
                        comm_started = false;
                        processBT_packets();
                    }
                }

                if(characteristic.getUuid().toString().startsWith(HT_MI_CHAR) && characteristic.getService().getUuid().toString().startsWith(HT_SRV)){
                    ht_mi_esp_value = characteristic.getIntValue(FORMAT_UINT16, 0);
                    Log.i("HT MI READ", "Value: " +ht_mi_esp_value + " s");
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(HT_OBJ_SIZE_CHAR) &&
                                    entry.getKey().toString().startsWith(HT_SRV)) {
                                queueRequestCharacteristicValue(gattCharacteristic);
                                break;
                            }
                        }
                    }

                }

                else if(characteristic.getUuid().toString().startsWith(HT_OBJ_SIZE_CHAR) && characteristic.getService().getUuid().toString().startsWith(HT_SRV)) {
                    byte[] temporal = characteristic.getValue();
                    byte[] obj_actual_size = {temporal[0], temporal[1], temporal[2], temporal[3]};
                    byte[] obj_max_size = {temporal[4], temporal[5], temporal[6], temporal[7]};
                    ht_obj_size_esp_number_samples = ByteBuffer.wrap(obj_max_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    ht_obj_size_esp_overflow = ByteBuffer.wrap(obj_actual_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    Log.i("HT OBJ SIZE READ", "Max value: " +ht_obj_size_esp_number_samples + "Actual value: " +ht_obj_size_esp_overflow );
                    if(ht_obj_size_esp_number_samples>0){
                        if(!is_ht_subscribed){
                            activate_deactivate_not_or_ind_of_service(1 ,true);
                        }
                    } else{
                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(HT_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(HT_SRV)) {
                                    current_set = settings_DB.getSettingsbyName(setting2);
                                    if(current_set != null){
                                        current_set_value = Integer.parseInt(current_set.getSetting_value());
                                        ByteBuffer byteBuffer = ByteBuffer.allocate(4);
                                        byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                                        byteBuffer.putInt(current_set_value);
                                        current_set_value_bits = byteBuffer.array();
                                        queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits);
                                        Log.i("HT MI WRITE", "Value: " +current_set_value);
                                    }
                                }
                            }
                        }

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(HR_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(HR_SRV)) {
                                    queueRequestCharacteristicValue(gattCharacteristic);
                                    break;
                                }
                            }
                        }
                    }
                }

                else if(characteristic.getUuid().toString().startsWith(HR_MI_CHAR) && characteristic.getService().getUuid().toString().startsWith(HR_SRV)){
                    hr_mi_esp_value = characteristic.getIntValue(FORMAT_UINT16, 0);
                    Log.i("HR MI READ", "Value: " +hr_mi_esp_value + " s");
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(HR_OBJ_SIZE_CHAR) &&
                                    entry.getKey().toString().startsWith(HR_SRV)) {
                                queueRequestCharacteristicValue(gattCharacteristic);
                                break;
                            }
                        }
                    }

                }

                else if(characteristic.getUuid().toString().startsWith(HR_VARIABLITY_CHAR) && characteristic.getService().getUuid().toString().startsWith(HR_SRV)){
                    byte[] temporal = characteristic.getValue();
                    float f = ByteBuffer.wrap(temporal).order(ByteOrder.BIG_ENDIAN).getFloat();
                    hr_variability_esp_value.add(f);
                    Log.i("onCharacteristicChanged", "HR VARIABILITY NOTIF RECEIVED: " + f );
                    hr_obj_size_esp_value--;
                    if(hr_obj_size_esp_value>0){
                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(HR_OBJ_SIZE_CHAR) &&
                                        entry.getKey().toString().startsWith(HR_SRV)) {
                                    byte[] obj_to_send = format_new_object_size(hr_obj_size_esp_value, hr_obj_size_esp_max_value);
                                    Log.i("onCharacteristicChanged", "HR NEW OBJ SIZE: " + Arrays.toString(obj_to_send) );
                                    queueWriteDataToCharacteristic(gattCharacteristic, obj_to_send);
                                    break;
                                }
                            }
                        }
                    } else {
                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(HR_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(HR_SRV)) {
                                    current_set = settings_DB.getSettingsbyName(setting4);
                                    if(current_set != null){
                                        current_set_value = Integer.parseInt(current_set.getSetting_value());
                                        ByteBuffer byteBuffer = ByteBuffer.allocate(4);
                                        byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                                        byteBuffer.putInt(current_set_value);
                                        current_set_value_bits = byteBuffer.array();
                                        queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits);
                                        Log.i("HR MI WRITE", "Value: " +current_set_value);
                                    }
                                }
                            }
                        }

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(PO_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(PO_SRV)) {
                                    queueRequestCharacteristicValue(gattCharacteristic);
                                    break;
                                }
                            }
                        }
                    }
                }

                else if(characteristic.getUuid().toString().startsWith(HR_OBJ_SIZE_CHAR) && characteristic.getService().getUuid().toString().startsWith(HR_SRV)) {
                    byte[] temporal = characteristic.getValue();
                    byte[] obj_actual_size = {temporal[0], temporal[1], temporal[2], temporal[3]};
                    byte[] obj_max_size = {temporal[4], temporal[5], temporal[6], temporal[7]};
                    hr_obj_size_esp_max_value = ByteBuffer.wrap(obj_max_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    hr_obj_size_esp_value = ByteBuffer.wrap(obj_actual_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    Log.i("HR OBJ SIZE READ", "Max value: " +hr_obj_size_esp_max_value);
                    Log.i("HR OBJ SIZE READ", "Actual value: " +hr_obj_size_esp_value);
                    if(hr_obj_size_esp_value>0){
                        if(!is_hr_subscribed){
                            activate_deactivate_not_or_ind_of_service(0 ,true);
                        }
                    } else{

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(HR_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(HR_SRV)) {
                                    current_set = settings_DB.getSettingsbyName(setting4);
                                    if(current_set != null){
                                        current_set_value = Integer.parseInt(current_set.getSetting_value());
                                        ByteBuffer byteBuffer = ByteBuffer.allocate(4);
                                        byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                                        byteBuffer.putInt(current_set_value);
                                        current_set_value_bits = byteBuffer.array();
                                        queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits);
                                        Log.i("HR MI WRITE", "Value: " +current_set_value);
                                    }
                                }
                            }
                        }

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(PO_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(PO_SRV)) {
                                    queueRequestCharacteristicValue(gattCharacteristic);
                                    break;
                                }
                            }
                        }
                    }
                }

                else if(characteristic.getUuid().toString().startsWith(PO_MI_CHAR) && characteristic.getService().getUuid().toString().startsWith(PO_SRV)){
                    po_mi_esp_value = characteristic.getIntValue(FORMAT_UINT16, 0);
                    Log.i("PO MI READ", "Value: " +po_mi_esp_value + " s");
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(PO_OBJ_SIZE_CHAR) &&
                                    entry.getKey().toString().startsWith(PO_SRV)) {
                                queueRequestCharacteristicValue(gattCharacteristic);
                                break;
                            }
                        }
                    }

                }

                else if(characteristic.getUuid().toString().startsWith(PO_OBJ_SIZE_CHAR) && characteristic.getService().getUuid().toString().startsWith(PO_SRV)) {
                    byte[] temporal = characteristic.getValue();
                    byte[] obj_actual_size = {temporal[0], temporal[1], temporal[2], temporal[3]};
                    byte[] obj_max_size = {temporal[4], temporal[5], temporal[6], temporal[7]};
                    po_obj_size_esp_max_value = ByteBuffer.wrap(obj_max_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    po_obj_size_esp_value = ByteBuffer.wrap(obj_actual_size).order(ByteOrder.BIG_ENDIAN).getInt();
                    Log.i("PO OBJ SIZE READ", "Max value: " +po_obj_size_esp_max_value);
                    Log.i("PO OBJ SIZE READ", "Actual value: " +po_obj_size_esp_value);
                    if(po_obj_size_esp_value>0){
                        if(!is_po_subscribed){
                            activate_deactivate_not_or_ind_of_service(2 ,true);
                        }
                    } else{
                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(PO_MI_CHAR) &&
                                        entry.getKey().toString().startsWith(PO_SRV)) {
                                    current_set = settings_DB.getSettingsbyName(setting3);
                                    if(current_set != null){
                                        current_set_value = Integer.parseInt(current_set.getSetting_value());
                                        ByteBuffer byteBuffer = ByteBuffer.allocate(4);
                                        byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                                        byteBuffer.putInt(current_set_value);
                                        current_set_value_bits = byteBuffer.array();
                                        queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits);
                                        Log.i("PO MI WRITE", "Value: " +current_set_value);
                                    }
                                }
                            }
                        }

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(SS_STEPS_CHAR) &&
                                        entry.getKey().toString().startsWith(SS_IDX_SVC)) {
                                    Log.i("onCharacteristicChanged", "STEPS REQUESTED");
                                    queueRequestCharacteristicValue(gattCharacteristic);
                                }

                            }
                        }

                        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                                if (gattCharacteristic.getUuid().toString().startsWith(BS_MEAS_CHAR) &&
                                        entry.getKey().toString().startsWith(BS_SRV)) {
                                    Log.i("onCharacteristicChanged", "BATTERY REQUESTED");
                                    queueRequestCharacteristicValue(gattCharacteristic);
                                }
                            }
                        }
                    }
                }

                else if (characteristic.getUuid().toString().startsWith(SS_STEPS_CHAR) &&
                        characteristic.getService().getUuid().toString().startsWith(SS_IDX_SVC)){
                    steps_number = characteristic.getIntValue(FORMAT_UINT32, 0);
                    Log.i("onCharacteristicRead", "STEPS RECEIVED: " + steps_number);

                }

                else if(characteristic.getUuid().toString().startsWith(BS_MEAS_CHAR) &&
                        characteristic.getService().getUuid().toString().startsWith(BS_SRV)){

                    esp32_battery_level = characteristic.getIntValue(FORMAT_UINT8, 0);
                    Log.i("onCharacteristicChanged", "BS MEAS RECEIVED: " + esp32_battery_level );

                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(FS_FALL_DETECTED_CHAR) &&
                                    entry.getKey().toString().startsWith(FS_IDX_SVC)) {
                                queueRequestCharacteristicValue(gattCharacteristic);
                                break;
                            }
                        }
                    }
                }

            }
        }

        @Override
        // Result of a characteristic change operation
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic) {
            processTxQueue();
            Settings current_set;
            String setting_value;
            int current_set_value = 0;
            byte[] current_set_value_bits;
            byte[] current_set_value_bits_hr = new byte[2];
            byte[] current_set_value_bits_ht = new byte[2];
            byte[] current_set_value_bits_po = new byte[2];


            if(characteristic.getUuid().toString().startsWith(HT_MEAS_CHAR) &&
                    characteristic.getService().getUuid().toString().startsWith(HT_SRV)){
                byte[] temporal = characteristic.getValue();
                byte[] temporal2 = {temporal[1], temporal[2], temporal[3], temporal[4]};
                ht_meas_esp_flags = temporal[0];
                float f = ByteBuffer.wrap(temporal2).order(ByteOrder.BIG_ENDIAN).getFloat();
                ht_meas_esp_value.add(f);
                Log.i("onCharacteristicChanged", "HT MEAS NOTIF RECEIVED: " + f );
                ht_obj_size_esp_number_samples--;
                if(ht_obj_size_esp_number_samples>0){
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(HT_OBJ_SIZE_CHAR) &&
                                    entry.getKey().toString().startsWith(HT_SRV)) {
                                byte[] obj_to_send = format_new_object_size(ht_obj_size_esp_overflow, ht_obj_size_esp_number_samples);
                                Log.i("onCharacteristicChanged", "HT NEW OBJ SIZE: " + Arrays.toString(obj_to_send) );
                                queueWriteDataToCharacteristic(gattCharacteristic, obj_to_send);
                                break;
                            }
                        }
                    }
                } else {

                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(HT_MI_CHAR) &&
                                    entry.getKey().toString().startsWith(HT_SRV)) {
                                current_set = settings_DB.getSettingsbyName(setting2);
                                if(current_set != null){
                                    current_set_value = Integer.parseInt(current_set.getSetting_value());
                                    ByteBuffer byteBuffer = ByteBuffer.allocate(4);
                                    byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                                    byteBuffer.putInt(current_set_value);
                                    current_set_value_bits = byteBuffer.array();
                                    queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits);
                                    Log.i("HT MI WRITE", "Value: " +current_set_value);
                                }
                            }
                        }
                    }


                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(HR_MI_CHAR) &&
                                    entry.getKey().toString().startsWith(HR_SRV)) {
                                queueRequestCharacteristicValue(gattCharacteristic);
                                break;
                            }
                        }
                    }
                }
            }

            else if(characteristic.getUuid().toString().startsWith(HR_MEAS_CHAR) &&
                    characteristic.getService().getUuid().toString().startsWith(HR_SRV)) {
                byte[] temporal = characteristic.getValue();
                hr_meas_esp_flags = temporal[0];
                int hr_value = characteristic.getIntValue(FORMAT_UINT8, 0);
                hr_meas_esp_value.add(hr_value);
                Log.i("onCharacteristicChanged", "HR MEAS NOTIF RECEIVED: " + hr_value);
                for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                    List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                    for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                        if (gattCharacteristic.getUuid().toString().startsWith(HR_VARIABLITY_CHAR) &&
                                entry.getKey().toString().startsWith(HR_SRV)) {
                            queueRequestCharacteristicValue(gattCharacteristic);
                            break;
                        }
                    }
                }
            }

            else if(characteristic.getUuid().toString().startsWith(PO_MEAS_CHAR) &&
                    characteristic.getService().getUuid().toString().startsWith(PO_SRV)){
                byte[] temporal = characteristic.getValue();
                po_meas_esp_device_and_sensor_status = new byte[] {temporal[0], temporal[1], temporal[2]};
                po_meas_esp_measurement_status = new byte [] {temporal[3], temporal[4]};
                byte[] pr = {temporal[5], temporal[6]};
                byte spo2 = temporal[8];
                po_meas_esp_flags = temporal[9];
                //po_meas_esp_pr = ByteBuffer.wrap(pr).getFloat();
                //float spo2_value = ByteBuffer.wrap(spo2).getFloat();

                po_meas_esp_value.add(spo2);
                Log.i("onCharacteristicChanged", "PO MEAS NOTIF RECEIVED: " + spo2);
                po_obj_size_esp_value--;
                if(po_obj_size_esp_value>0){
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(PO_OBJ_SIZE_CHAR) &&
                                    entry.getKey().toString().startsWith(PO_SRV)) {
                                byte[] obj_to_send = format_new_object_size(po_obj_size_esp_value, po_obj_size_esp_max_value);
                                Log.i("onCharacteristicChanged", "PO NEW OBJ SIZE: " + Arrays.toString(obj_to_send) );
                                queueWriteDataToCharacteristic(gattCharacteristic, obj_to_send);
                                break;
                            }
                        }
                    }
                } else {
                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(PO_MI_CHAR) &&
                                    entry.getKey().toString().startsWith(PO_SRV)) {
                                current_set = settings_DB.getSettingsbyName(setting3);
                                if(current_set != null){
                                    setting_value = current_set.getSetting_value();
                                    Log.i("Setting value", setting_value);
                                    current_set_value = Integer.parseInt(setting_value);
                                    current_set_value_bits_po[0] = (byte) (current_set_value & 0xFF);
                                    current_set_value_bits_po[1] = (byte) ((current_set_value >> 8) & 0xFF);
                                    Log.i("Setting value byte", Arrays.toString(current_set_value_bits_po));
                                    queueWriteDataToCharacteristic(gattCharacteristic, current_set_value_bits_po);
                                }
                            }
                        }
                    }

                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(SS_STEPS_CHAR) &&
                                    entry.getKey().toString().startsWith(SS_IDX_SVC)) {
                                Log.i("onCharacteristicChanged", "STEPS REQUESTED");
                                queueRequestCharacteristicValue(gattCharacteristic);
                            }

                        }
                    }

                    for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                        List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                        for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                            if (gattCharacteristic.getUuid().toString().startsWith(BS_MEAS_CHAR) &&
                                    entry.getKey().toString().startsWith(BS_SRV)) {
                                Log.i("onCharacteristicChanged", "BATTERY REQUESTED");
                                queueRequestCharacteristicValue(gattCharacteristic);
                            }
                        }
                    }
                }
            }

        }
        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic, int status) {
            processTxQueue();
        }
    };

    private void reset_ESP32_comm(){
        if(comm_started == true) {
            processBT_packets();
        }
    }

    private void get_fall_probability_and_notify(int esp32_fall_value){

        switch(esp32_fall_value){
            case 1:
                esp32_percentage = 50;
                break;
            case 2:
                esp32_percentage = 60;
                break;
            case 3:
                esp32_percentage = 60;
                break;
            case 4:
                esp32_percentage = 70;
                break;
            case 5:
                esp32_percentage = 70;
                break;
            case 6:
                esp32_percentage = 80 ;
                break;
            case 7:
                break;
            case 8:
                esp32_percentage = -1;
                break;
            default:
                break;

        }
        if(esp32_percentage != -1){
            Patient patient = patients_DB.getPatientByESP32_ID(Content_main_fragment.last_patient_ID);
            if(patient != null){
                float mc = patient.getMuscle_condition();
                float bc = patient.getBones_condition();
                float md = patient.getMobility_degree();
                float uf = (float) patient.getUsually_faints();
                float pf = (float) patient.getPrevious_falls();
                esp32_percentage = (float) (esp32_percentage + (mc*12.5+bc*12.5+md*15+uf*15)*(1+(0.1*pf)));
            }
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //Create the alert dialog
                AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);

                builder.setTitle(MainActivity.this.getResources().getString(R.string.warning));

                if(esp32_percentage == -1){
                    builder.setMessage(MainActivity.this.getResources().getString(R.string.fall_notified));
                } else {
                    builder.setMessage(MainActivity.this.getResources().getString(R.string.fall_detected) + esp32_percentage
                            + " " +MainActivity.this.getResources().getString(R.string.percetage));
                }

                builder.setPositiveButton(R.string.accept, null);
                builder.show();
            }
        });

        MediaPlayer mp = MediaPlayer.create(getApplicationContext(), R.raw.alarm_sound);
        mp.start();
    }

    private void processBT_packets(){
        Date date = new Date();
        date.getTime();
        Calendar currentTime = Calendar.getInstance();
        currentTime.setTime(date);
        Calendar currentTime_ht = Calendar.getInstance();
        currentTime_ht.setTime(date);
        Calendar currentTime_hr = Calendar.getInstance();
        currentTime_hr.setTime(date);
        Calendar currentTime_po = Calendar.getInstance();
        currentTime_po.setTime(date);
        String measure_string;
        String measure_string2;
        String date_string;
        date_string = dateFormat.format(currentTime.getTime());
        Patient_BT.setLast_connection_date(date_string);
        float current_hr;
        int hr_warning_index = 0;
        float current_hr_var;
        int hr_var_warning_index = 0;
        float current_temp;
        int temp_warning_index = 0;
        float current_sat;
        int sat_warning_index = 0;

        int i_length = ht_meas_esp_value.size();
        //Content_main_fragment.temp_color = 1;
        //Tener en cuenta el numero de overflow a la hora de almacenar datos
        currentTime_ht.add(Calendar.SECOND, -(ht_mi_esp_value*i_length));
        if(i_length>0) {
            for (int i = i_length; i>0; i--) {
                date_string = dateFormat.format(currentTime_ht.getTime());
                current_temp = ht_meas_esp_value.get(i - 1);
                if (current_temp > ht_up_th || current_temp < ht_bt_th) {
                    ht_alarm = true;
                    temp_warning_index = i-1;
                }
                measure_string = Float.toString(current_temp);
                Patient_BT.setTemperature(measure_string);
                Patient_BT.setTemperatureDate(date_string);
                currentTime_ht.add(Calendar.SECOND, ht_mi_esp_value);
                Log.i("position", " i: " + Integer.toString(i) + " Measure: " + measure_string + " Date: " + date_string);
            }
        }


        i_length = hr_meas_esp_value.size();
        //Content_main_fragment.hr_color = 1;
        currentTime_hr.add(Calendar.SECOND, -(hr_mi_esp_value*i_length));
        if(i_length>0) {
            for (int i = i_length; i>0; i--) {
                date_string = dateFormat.format(currentTime_hr.getTime());
                current_hr = hr_meas_esp_value.get(i-1);
                if(current_hr>hr_up_th || current_hr<hr_bt_th){
                    hr_alarm = true;
                    hr_warning_index = i-1;
                }
                measure_string = Float.toString(current_hr);
                measure_string = measure_string.replaceAll("\\.[0]*$", "");
                current_hr_var = hr_variability_esp_value.get(i-1);
                if(current_hr_var>hr_var_up_th){
                    Content_main_fragment.hr_var_alarm = true;
                    hr_var_warning_index = i-1;
                }
                measure_string2 = Float.toString(current_hr_var);
                measure_string = measure_string + "/" + measure_string2;
                Patient_BT.setHeart_rate(measure_string);
                Patient_BT.setHeart_rateDate(date_string);
                currentTime_hr.add(Calendar.SECOND, -hr_mi_esp_value);
                Log.i("position", " i: " + Integer.toString(i) + " Measure: " + measure_string + " Date: " + date_string);
            }
        }


        i_length = po_meas_esp_value.size();
        // Content_main_fragment.sat_color = 1;
        currentTime_po.add(Calendar.SECOND, -(po_mi_esp_value*i_length));
        if(i_length>0) {
            for (int i = i_length; i>0; i--) {
                date_string = dateFormat.format(currentTime_po.getTime());
                current_sat = po_meas_esp_value.get(i-1);
                measure_string = Float.toString(current_sat);
                if(current_sat<po_bt_th){
                    po_alarm = true;
                    sat_warning_index = i-1;
                }
                Patient_BT.setSaturation(measure_string);
                Patient_BT.setSaturationDate(date_string);
                currentTime_po.add(Calendar.SECOND, -po_mi_esp_value);
                Log.i("position", " i: " + Integer.toString(i) + " Measure: " + measure_string + " Date: " + date_string);
            }
        }

        Patient_BT.setSteps(String.valueOf(steps_number));
        date_string = dateFormat.format(currentTime.getTime());
        Patient_BT.setStepsDate(String.valueOf(date_string));
        steps_number = 0;

        Patient_BT.setESP32_battery(Integer.toString(esp32_battery_level));
        if(esp32_battery_level <= ESP32_TH){
            esp32_battery_alarm = true;
        }

        if(esp32_battery_alarm || hr_alarm || ht_alarm || po_alarm || hr_var_alarm){
            final String hr_text;
            final String ht_text;
            final String po_text;
            final String bat_text;
            final String hr_var_text;
            if(esp32_battery_alarm){
                bat_text = MainActivity.this.getResources().getString(R.string.low_battery) + " " + Integer.toString(esp32_battery_level) + "%";
            }else{
                bat_text = "";
            }
            if(hr_alarm){
                hr_text = String.format("%s %d ppm", MainActivity.this.getResources().getString(R.string.hr_cards),  hr_meas_esp_value.get(hr_warning_index) );

            }else{
                hr_text = "";
            }
            if(hr_var_alarm){
                hr_var_text = String.format("%s %.2f ms", MainActivity.this.getResources().getString(R.string.hr_variability),  hr_variability_esp_value.get(hr_var_warning_index) );
            } else {
                hr_var_text = "";
            }
            if(ht_alarm){
                ht_text = String.format("%s %.2f ºC", MainActivity.this.getResources().getString(R.string.temperature),  ht_meas_esp_value.get(temp_warning_index) );
            } else{
                ht_text = "";
            }
            if(po_alarm){
                po_text = String.format("%s %d %s", MainActivity.this.getResources().getString(R.string.saturation),  po_meas_esp_value.get(sat_warning_index), "%" );
            } else {
                po_text = "";
            }
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    //Create the alert dialog
                    AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);

                    builder.setTitle(MainActivity.this.getResources().getString(R.string.warning));

                    builder.setMessage(MainActivity.this.getResources().getString(R.string.parameters_out_of_range)+"\n\n"
                            +ht_text+ "\n" + po_text+ "\n"+ hr_text+ "\n" + hr_var_text+ "\n" + bat_text);
                    builder.setPositiveButton(R.string.accept, null);
                    builder.show();
                }
            });

            MediaPlayer mp = MediaPlayer.create(getApplicationContext(), R.raw.alarm_sound);
            mp.start();
            hr_alarm = false;
            ht_alarm = false;
            po_alarm = false;
            hr_var_alarm = false;
            esp32_battery_alarm = false;
        }

        ht_meas_esp_value.clear();
        hr_meas_esp_value.clear();
        hr_variability_esp_value.clear();
        po_meas_esp_value.clear();
        ht_obj_size_esp_overflow = 0;
        ht_obj_size_esp_number_samples = 0;
        hr_obj_size_esp_max_value = 0;
        hr_obj_size_esp_value = 0;
        po_obj_size_esp_max_value = 0;
        po_obj_size_esp_value = 0;
        times_requested_falls = 0;
        comm_finished = false;
        comm_started = false;

        MainActivity.patients_DB.updatePatient(Patient_BT);
        last_patient_ID = Patient_BT.getEsp32_ID();
        Settings set = MainActivity.settings_DB.getSettingsbyName(setting1);
        set.setSettingValue(last_patient_ID);
        MainActivity.settings_DB.updateSettings(set);
        Fragment fragment = null;
        Class fragmentClass = null;
        fragmentClass = Content_main_fragment.class;
        try {
            fragment = (Fragment) fragmentClass.newInstance();
        } catch (Exception e) {
            e.printStackTrace();
        }

        FragmentManager fragmentManager = getSupportFragmentManager();
        fragmentManager.beginTransaction().replace(R.id.flContent, fragment).addToBackStack("Content_main_fragment").commit();
    }

    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for ( int j = 0; j < bytes.length; j++ ) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }

    public void read_falls(){
        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                if (gattCharacteristic.getUuid().toString().startsWith(FS_FALL_DETECTED_CHAR) &&
                        entry.getKey().toString().startsWith(FS_IDX_SVC)) {
                    queueRequestCharacteristicValue(gattCharacteristic);
                    break;
                }
            }
        }
    }

    public void start_eSpMART_comm_process(){
        for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
            List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                if (gattCharacteristic.getUuid().toString().startsWith(HT_MI_CHAR) &&
                        entry.getKey().toString().startsWith(HT_SRV)) {
                    queueRequestCharacteristicValue(gattCharacteristic);
                    break;
                }
            }
        }
    }

    //0 HR, 1 HT, 2 PO
    public void activate_deactivate_not_or_ind_of_service(int service, boolean on_off){
        byte value[] = {0x00, 0x00};
        switch (service){
            case 0:
                for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                    List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                    for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                        if (gattCharacteristic.getUuid().toString().startsWith(HR_MEAS_CHAR) &&
                                entry.getKey().toString().startsWith(HR_SRV)) {
                            if(on_off) {
                                queueSetNotificationForCharacteristic(gattCharacteristic, true, false);
                                is_hr_subscribed = true;
                            } else{
                                queueSetNotificationForCharacteristic(gattCharacteristic, false, false);
                                is_hr_subscribed = false;
                            }

                            break;

                        }
                    }
                }
                break;
            case 1:
                for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                    List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                    for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                        if (gattCharacteristic.getUuid().toString().startsWith(HT_MEAS_CHAR) &&
                                entry.getKey().toString().startsWith(HT_SRV)) {
                            if(on_off) {
                                queueSetNotificationForCharacteristic(gattCharacteristic, true, true);
                                is_ht_subscribed = true;
                            } else{
                                queueSetNotificationForCharacteristic(gattCharacteristic, false, true);
                                is_ht_subscribed = false;
                            }
                            break;
                        }
                    }
                }
                break;
            case 2:
                for (Map.Entry<UUID, List<BluetoothGattCharacteristic>> entry : gattServandChar.entrySet()) {
                    List<BluetoothGattCharacteristic> gattCharacteristics = entry.getValue();
                    for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                        if (gattCharacteristic.getUuid().toString().startsWith(PO_MEAS_CHAR) &&
                                entry.getKey().toString().startsWith(PO_SRV)) {
                            if(on_off) {
                                queueSetNotificationForCharacteristic(gattCharacteristic, true, true);
                                is_po_subscribed = true;
                            } else{
                                queueSetNotificationForCharacteristic(gattCharacteristic, false, true);
                                is_po_subscribed = false;
                            }
                            break;

                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    /* set new value for particular characteristic */
    public void writeDataToCharacteristic(final BluetoothGattCharacteristic ch, final byte[] dataToWrite) {
        if (btAdapter == null || mBluetoothGatt == null || ch == null) return;

        // first set it locally....
        ch.setValue(dataToWrite);
        // ... and then "commit" changes to the peripheral
        mBluetoothGatt.writeCharacteristic(ch);
        Log.i("onWriteDataToChar", "Data Write: " +dataToWrite+ " on characteristic: " +ch.getUuid().toString());
    }

    /* enables/disables notification for characteristic */
    public void setNotificationForCharacteristic(BluetoothGattCharacteristic ch, boolean enabled, boolean indication) {
        if (btAdapter == null || mBluetoothGatt == null) return;
        boolean success = mBluetoothGatt.setCharacteristicNotification(ch, enabled);
        if(!success) {
            Log.e("------", "Seting proper notification status for characteristic failed!");
        }
        // This is also sometimes required (e.g. for heart rate monitors) to enable notifications/indications
        // see: https://developer.bluetooth.org/gatt/descriptors/Pages/DescriptorViewer.aspx?u=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
        BluetoothGattDescriptor descriptor = ch.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
        if(descriptor != null) {
            byte[] val;
            if(indication){
                val = enabled ? BluetoothGattDescriptor.ENABLE_INDICATION_VALUE : BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE;
            } else {
                val = enabled ? BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE : BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE;
            }
            descriptor.setValue(val);
            mBluetoothGatt.writeDescriptor(descriptor);
        } else {
            Log.i("onSetNotifForChar", "Descriptor CCC associated to a char was not found");
        }

        Log.i("onSetNotifForChar", "Notification Status: " +enabled+ " on characteristic: " +ch.getUuid().toString());
    }

    /* request to fetch newest value stored on the remote device for particular characteristic */
    public void requestCharacteristicValue(BluetoothGattCharacteristic ch) {
        if (btAdapter == null || mBluetoothGatt == null) return;

        mBluetoothGatt.readCharacteristic(ch);
        Log.i("onRequestCharValue", "Data request on characteristic: " +ch.getUuid().toString());
    }

    /* An enqueueable write operation - notification subscription or characteristic write */
    private class TxQueueItem {
        BluetoothGattCharacteristic characteristic;
        byte[] dataToWrite; // Only used for characteristic write
        boolean enabled; // Only used for characteristic notification subscription
        boolean indication;
        public TxQueueItemType type;
    }

    /**
     * The queue of pending transmissions
     */
    private Queue<TxQueueItem> txQueue = new LinkedList<TxQueueItem>();

    private boolean txQueueProcessing = false;

    private enum TxQueueItemType {
        SubscribeCharacteristic,
        ReadCharacteristic,
        WriteCharacteristic
    }

    /* queues enables/disables notification for characteristic */
    public void queueSetNotificationForCharacteristic(BluetoothGattCharacteristic ch, boolean enabled, boolean indication) {
        // Add to queue because shitty Android GATT stuff is only synchronous
        TxQueueItem txQueueItem = new TxQueueItem();
        txQueueItem.characteristic = ch;
        txQueueItem.enabled = enabled;
        txQueueItem.indication = indication;
        txQueueItem.type = TxQueueItemType.SubscribeCharacteristic;
        addToTxQueue(txQueueItem);
    }

    /* queues enables/disables notification for characteristic */
    public void queueWriteDataToCharacteristic(final BluetoothGattCharacteristic ch, final byte[] dataToWrite) {
        // Add to queue because shitty Android GATT stuff is only synchronous
        TxQueueItem txQueueItem = new TxQueueItem();
        txQueueItem.characteristic = ch;
        txQueueItem.dataToWrite = dataToWrite;
        txQueueItem.type = TxQueueItemType.WriteCharacteristic;
        addToTxQueue(txQueueItem);
    }

    /* request to fetch newest value stored on the remote device for particular characteristic */
    public void queueRequestCharacteristicValue(BluetoothGattCharacteristic ch) {

        // Add to queue because shitty Android GATT stuff is only synchronous
        TxQueueItem txQueueItem = new TxQueueItem();
        txQueueItem.characteristic = ch;
        txQueueItem.type = TxQueueItemType.ReadCharacteristic;
        addToTxQueue(txQueueItem);
    }

    /**
     * Add a transaction item to transaction queue
     * @param txQueueItem
     */
    private void addToTxQueue(TxQueueItem txQueueItem) {
        Observable.timer(DELAY_BETWEEN_BT_COMMANDS_MS, TimeUnit.MILLISECONDS)
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new Subscriber<Long>() {
                    @Override
                    public void onCompleted() {
                        txQueue.add(txQueueItem);

                        // If there is no other transmission processing, go do this one!
                        if (!txQueueProcessing) {
                            processTxQueue();
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                    }

                    @Override
                    public void onNext(Long aLong) {
                    }
                });
    }

    /**
     * Call when a transaction has been completed.
     * Will process next transaction if queued
     */
    private void processTxQueue() {
        if (txQueue.size() <= 0)  {
            txQueueProcessing = false;
            return;
        }
        txQueueProcessing = true;
        TxQueueItem txQueueItem = txQueue.remove();
        switch (txQueueItem.type) {
            case WriteCharacteristic:
                writeDataToCharacteristic(txQueueItem.characteristic, txQueueItem.dataToWrite);
                break;
            case SubscribeCharacteristic:
                setNotificationForCharacteristic(txQueueItem.characteristic, txQueueItem.enabled, txQueueItem.indication);
                break;
            case ReadCharacteristic:
                requestCharacteristicValue(txQueueItem.characteristic);
        }

    }

}
