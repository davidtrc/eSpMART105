package com.lazaro.b105.valorizab105.fragments;

import android.Manifest;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.v4.app.Fragment;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.lazaro.b105.valorizab105.MainActivity;
import com.lazaro.b105.valorizab105.Patient;
import com.lazaro.b105.valorizab105.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import rx.Observable;
import rx.Subscriber;
import rx.android.schedulers.AndroidSchedulers;

import static android.content.Context.LOCATION_SERVICE;

public class Connect_ESP32_fragment extends Fragment {

    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    private BluetoothGatt mBluetoothGatt;
    List<BluetoothDevice> foundevices;
    BluetoothDevice eSpMART105;
    boolean eSpMART105_discovered = false;
    List<BluetoothGattService> gattServices;
    Map<UUID, List<BluetoothGattCharacteristic>> gattServandChar;
    Map<UUID, List<BluetoothGattDescriptor>> gattCharandDescr;
    RecyclerView mRecyclerView;
    private BTAdapter mBTAdapter;


    private static final int TIMEOUT_SCAN = 10;
    private TextView mCheckCountTV;

    private static final int REQUEST_PERMISSION = 1;
    private ProgressDialog dialog_connecting;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;
    private int mConnectionState = STATE_DISCONNECTED;

    //private volatile long mStartConnectTime;
    private ProgressDialog dialog;

    public interface ConnectESP32InteractionListener {}

    private ConnectESP32InteractionListener mListener;

    public Connect_ESP32_fragment() {}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View connect_ESP32_view = inflater.inflate(R.layout.fragment_connect__esp32, container, false);
        dialog = new ProgressDialog(getContext());

        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.find_patients);
        MainActivity.navigationView.getMenu().getItem(0).setChecked(true);

        mRecyclerView = (RecyclerView) connect_ESP32_view.findViewById(R.id.recycler_view);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity().getApplicationContext(), LinearLayoutManager.VERTICAL, false));

        gattServandChar = new HashMap<>();

        /*
        send_hi.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                dialog.setMessage(getResources().getString(R.string.searching_wait));
                dialog.setCancelable(false);
                dialog.show();

                foundevices = new ArrayList<>();
                btManager = (BluetoothManager) getActivity().getSystemService(Context.BLUETOOTH_SERVICE);
                btAdapter = btManager.getAdapter();
                mBTAdapter = new BTAdapter();
                mRecyclerView.setAdapter(mBTAdapter);
                scan();
            }
        });
        */

        return connect_ESP32_view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof ConnectESP32InteractionListener) {
            mListener = (ConnectESP32InteractionListener) context;
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

    private void removeGarbageDevices() {
        for (int i = foundevices.size() - 1; i >= 0; i--) {
            BluetoothDevice device = foundevices.get(i);
            for (int j = i - 1; j >= 0; j--) {
                BluetoothDevice compareDevice = foundevices.get(j);
                if (device.equals(compareDevice)) {
                    foundevices.remove(i);
                    mBTAdapter.notifyItemRemoved(i);
                    break;
                }
            }
        }
    }

    private void scan() {
        btManager = (BluetoothManager) getActivity().getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        btScanner = btAdapter.getBluetoothLeScanner();

        // Checks if Bluetooth is supported on the device.
        if (btAdapter == null) {
            Toast.makeText(getActivity(), R.string.bt_disabled, Toast.LENGTH_SHORT).show();
            return;
        }

        if (!BluetoothAdapter.getDefaultAdapter().isEnabled()) {
            Toast.makeText(this.getContext(), R.string.bt_disabled, Toast.LENGTH_SHORT).show();
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Check location enable
            LocationManager locationManager = (LocationManager) getActivity().getSystemService(LOCATION_SERVICE);
            boolean locationGPS = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
            boolean locationNetwork = locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
            if (!locationGPS && !locationNetwork) {
                Toast.makeText(getContext(), R.string.location_disabled, Toast.LENGTH_SHORT).show();
                return;
            }
        }

        /*
        scanHandler = new Handler();

        scanHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                btScanner.stopScan(leScanCallback);
            }
        }, TIMEOUT_SCAN);
        BleScanner.startLeScan(leScanCallback);
        */
        btScanner.startScan(leScanCallback);

        Observable.timer(TIMEOUT_SCAN, TimeUnit.SECONDS)
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new Subscriber<Long>() {
                    @Override
                    public void onCompleted() {
                        //BleScanner.stopLeScan(mBTCallback);
                        btScanner.stopScan(leScanCallback);
                        for(int i=0; i<foundevices.size(); i++){
                            Log.i("DEVICE", String.valueOf(i) + " : " + foundevices.get(i).toString() );
                        }
                        removeGarbageDevices();
                        dialog.dismiss();
                    }

                    @Override
                    public void onError(Throwable e) {
                    }

                    @Override
                    public void onNext(Long aLong) {
                    }
                });
    }

    private static class BackgroundThread extends HandlerThread {
        BackgroundThread() {
            super("Bluetooth-Update-BackgroundThread", android.os.Process.THREAD_PRIORITY_BACKGROUND);
        }
    }

    private class BTHolder extends RecyclerView.ViewHolder //davidtrc LA USO. ESTA CLASE RELACIONA la vista bluetooth_device_item con cada elemento
            implements View.OnClickListener {
        View view;
        TextView text1;
        TextView text2;

        TextView status1;
        ProgressBar progress1;

        BluetoothDevice ble;

        BTHolder(View itemView) {
            super(itemView);

            text1 = (TextView) itemView.findViewById(R.id.text1);
            text2 = (TextView) itemView.findViewById(R.id.text2);
            status1 = (TextView) itemView.findViewById(R.id.status1);
            progress1 = (ProgressBar) itemView.findViewById(R.id.progress1);

            itemView.setOnClickListener(BTHolder.this);
            view = itemView;
        }

        @Override
        public void onClick(View v) {
            if (mBTAdapter.mCheckable) {
                return;
            }
        }
    }

    private class BTAdapter extends RecyclerView.Adapter<BTHolder> {
        LayoutInflater mInflater = getActivity().getLayoutInflater();

        boolean mCheckable = false;

        @Override
        public BTHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View itemView = mInflater.inflate(R.layout.bluetooth_device_item, parent, false);
            return new BTHolder(itemView);
        }

        @Override
        public void onBindViewHolder(BTHolder holder, int position) {
            holder.ble = foundevices.get(position);

            String name = holder.ble.getName();


            if (TextUtils.isEmpty(name)) {
                name = getResources().getString(R.string.unnamed);
            }
            Patient p_temp = MainActivity.patients_DB.getPatientByESP32_ID(name);

            holder.text1.setText(String.format("%d. %s", position + 1, name));
            if(p_temp == null) {
                holder.text2.setText(getResources().getString(R.string.no_associated));
            } else {
                holder.text2.setText(getResources().getString(R.string.associated_to)+ " " +p_temp.getName().toString());
            }

        }

        @Override
        public int getItemCount() {
            return foundevices.size();
        }
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
            if (!exist){
                if(nd.getName() != null){
                    if (nd.getName().toString().startsWith("PATIENT")) {
                        foundevices.add(nd);
                        mBTAdapter.notifyDataSetChanged();
                    }
                }
            }
        }
    };

}
