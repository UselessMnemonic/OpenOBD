package com.example.openobd;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.text.method.ScrollingMovementMethod;
import android.util.JsonReader;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.StringReader;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;

public class MainActivity extends Activity implements View.OnClickListener {

    /* This class in the interface between BLE and the Activity */
    private class UARTGattHandler extends BluetoothGattCallback {

        /* Queues for UART RX/TX */
        private Queue<String> txQueue;
        private StringBuilder rxBuffer;

        /* Keep track of when a read and write is happening */
        private boolean txInProgress;
        private final Object rxLock; // the piped streams can block on on concurrent ops

        public UARTGattHandler() {
            rxLock = new Object();
            txInProgress = false;
            txQueue = new ConcurrentLinkedQueue<>();
            rxBuffer = new StringBuilder();
        }

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);

            // first step of connection process is to wait for connection state to change
            // afterwards, we have to ask the module for services
            switch (newState) {

                // When Bluetooth connects
                case BluetoothGatt.STATE_CONNECTED:
                    if (status == BluetoothGatt.GATT_SUCCESS) {
                        appendLine("-- OpenOBD found --\n");
                        gatt.discoverServices();
                    }
                    else appendLine("-- Could not find OpenOBD module --\n\n");
                    break;

                // When Bluetooth disconnects, we disabled the UI
                case BluetoothGatt.STATE_DISCONNECTED:
                    appendLine("-- Connection closed --\n\n");
                    enableUI(false);
                    mRX = null;
                    mTX = null;
                    break;
                default:
                    break;
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            super.onServicesDiscovered(gatt, status);

            if (status == BluetoothGatt.GATT_FAILURE) {
                appendLine("-- Could not connect to OpenOBD module --\n\n");
                return;
            }

            // Save reference to each UART characteristic.
            mTX = gatt.getService(UART_UUID).getCharacteristic(TX_UUID);
            mRX = gatt.getService(UART_UUID).getCharacteristic(RX_UUID);

            // Enable notification upon RX
            gatt.setCharacteristicNotification(mRX, true);

            // Make sure TX notification is not enabled
            gatt.setCharacteristicNotification(mTX, false);

            appendLine("-- Connected to OpenOBD module --\n\n");
            enableUI(true);
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gatt, characteristic);

            //TODO We could do some sort of message control as to reconstruct whole packets
            String data = characteristic.getStringValue(0);
            Log.d(TAG, "Read: " + data);
            synchronized (rxLock) {
                // As we receive data, we will continuously parse JSON
                try {
                    rxBuffer.append(data);

                    // per spec, all JSON is line-delimited
                    if (data.contains("\n")) {
                        JSONObject response = JsonParser.parseObject(new JsonReader(new StringReader(rxBuffer.toString())));
                        appendLine("-- Got Response --\n" +
                                response.toString(1) +
                                "\n\n");
                        rxBuffer = new StringBuilder();
                    }

                } catch (IOException | JSONException e) {
                    Log.wtf(TAG, e);
                    appendLine("-- Internal Error: " + e.getMessage() + " --\n\n");
                }
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gatt, characteristic, status);

            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d(TAG,"Wrote: " + characteristic.getStringValue(0));
            }

            // continue writing if we have more chunks
            if (txQueue.isEmpty()) txInProgress = false;
            else writeQueueFront();
        }

        private void send(JSONObject jobj) {

            // Per spec we will delimit JSON with newlines
            String message = jobj.toString();
            if (message.charAt(message.length() - 1) != '\n')
                message += '\n';

            // We can only send ~20 bytes per packet, so we partition each message
            for (int len = message.length(); len > BLE_UART_BUFSIZE; len -= BLE_UART_BUFSIZE) {
                txQueue.add(message.substring(0, BLE_UART_BUFSIZE));
                message = message.substring(BLE_UART_BUFSIZE);
            }

            // lagging bit of message left over
            if (!message.isEmpty()) txQueue.add(message);

            // tell system to start sending the chunks if it isn't already
            if (!txInProgress && !txQueue.isEmpty()) {
                txInProgress = true;
                writeQueueFront();
            }
        }

        /* Sends the next TX unit in the cache */
        private void writeQueueFront() {
            String chunk = txQueue.poll();
            mTX.setValue(chunk);
            mGatt.writeCharacteristic(mTX);
        }
    }

    /* Constants */
    private static final String OPEN_OBD_MAC = "e8:e1:d5:e1:73:cd";
    public static final UUID UART_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID TX_UUID   = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID RX_UUID   = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final int BLE_UART_BUFSIZE = 20;
    public static final String TAG = "OpenOBD";

    /* UI elements */
    private Spinner  mSpinner;
    private Button   mRequestButton;
    private TextView mUserLog;

    /* BLE state */
    private BluetoothAdapter mBLEAdapter;
    private BluetoothDevice mOpenOBD;
    private BluetoothGatt mGatt;

    /* UART state */
    private UARTGattHandler mUART;
    private BluetoothGattCharacteristic mRX, mTX;

    /* Yay, globals */
    private int sequenceCounter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Grab references to UI elements.
        mUserLog = (TextView) findViewById(R.id.userLog);
        mSpinner = (Spinner) findViewById(R.id.pidSpinner);
        mRequestButton = (Button) findViewById(R.id.requestButton);

        // Enable auto-scroll in the TextView
        mUserLog.setMovementMethod(new ScrollingMovementMethod());

        // Disable the send button until we're connected.
        mRequestButton.setOnClickListener(this);
        enableUI(false);

        // Add items to spinner
        ArrayAdapter<OBDPID> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, OBDPID.values());
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mSpinner.setAdapter(adapter);

        // Initialize Bluetooth
        mOpenOBD = null;
        mUART = null;
        mRX = null;
        mTX = null;
        mGatt = null;
        sequenceCounter = 0;

        // Make sure BLE is enabled
        mBLEAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBLEAdapter == null || !mBLEAdapter.isEnabled()) {
            new AlertDialog.Builder(this)
                    .setMessage("You must have Bluetooth enabled first.")
                    .setNeutralButton("OK", (DialogInterface dialog, int which) -> finish())
                    .show();
        }
        else {
            // find OpenOBD device
            Set<BluetoothDevice> pairedDevices = mBLEAdapter.getBondedDevices();
            for (BluetoothDevice device : pairedDevices) {
                if (device.getAddress().toLowerCase().equals(OPEN_OBD_MAC)) {
                    mOpenOBD = device;
                    break;
                }
            }

            if (mOpenOBD == null) {
                new AlertDialog.Builder(this)
                        .setMessage("You must pair with the OpenOBD module")
                        .setNeutralButton("OK", (DialogInterface dialog, int which) -> {
                            startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
                        })
                        .show();
            }
            else {
                // Connect to OpenOBD (Async)
                appendLine("************************************\n" +
                        "*  Your device will automatically  *\n" +
                        "*  connect and reconnect to the    *\n" +
                        "*  module when available. You can  *\n" +
                        "*  make several requests in rapid  *\n" +
                        "*  succession, but some may drop.  *\n" +
                        "************************************\n");
                appendLine("-- Searching for OpenOBD module --\n");
                mUART = new UARTGattHandler();
                mGatt = mOpenOBD.connectGatt(this, true, mUART);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    /* Write text to the text view */
    private void appendLine(final CharSequence text) {
        runOnUiThread(() -> {
            synchronized (mUserLog) {
                mUserLog.append(text);
            }
        });
    }

    private void enableUI(final boolean enable) {
        runOnUiThread(() -> {
            mRequestButton.setClickable(enable);
            mRequestButton.setEnabled(enable);
        });
    }

    // Handler for mouse click on the send button.
    public void onClick(View view) {
        OBDPID selection = (OBDPID) mSpinner.getSelectedItem();

        JSONObject jsonRequest = new JSONObject();
        try {
            jsonRequest.put("pid", selection.getPID());
            jsonRequest.put("seq", sequenceCounter++);
            appendLine("-- Requesting " + selection.toString() + " --\n");
            appendLine(jsonRequest.toString(1) + "\n\n");
            mUART.send(jsonRequest);
        }
        catch (JSONException e) {
            appendLine("\n-- Internal Error: " + e.getMessage() + "\n\n");
            Log.wtf(TAG, e);
        }
    }
}