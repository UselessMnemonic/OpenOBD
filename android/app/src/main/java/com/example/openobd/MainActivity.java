package com.example.openobd;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import java.nio.charset.StandardCharsets;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;

public class MainActivity extends Activity implements View.OnClickListener {

    private class UARTGattHandler extends BluetoothGattCallback {

        /* Queues for UART RX/TX */
        private Queue<String> writeQueue;

        /* Keep track of when a read and write is happening */
        private boolean writeInProgress;

        public UARTGattHandler() {
            writeInProgress = false;
            writeQueue = new ConcurrentLinkedQueue<>();
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
                    else appendLine("-- Could not find OpenOBD module --\n");
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
                appendLine("-- Could not connect to OpenOBD module --\n");
                return;
            }

            // Save reference to each UART characteristic.
            mTX = gatt.getService(UART_UUID).getCharacteristic(TX_UUID);
            mRX = gatt.getService(UART_UUID).getCharacteristic(RX_UUID);

            // Enable notification upon RX
            gatt.setCharacteristicNotification(mRX, true);

            // Make sure TX notification is not enabled
            gatt.setCharacteristicNotification(mTX, false);

            enableUI(true);
            appendLine("-- Connected to OpenOBD module --\n");
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gatt, characteristic);

            // print message to text view, this should only ever come from mRX
            appendLine(characteristic.getStringValue(0));

            //TODO We could do some sort of message control as to reconstruct whole packets
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gatt, characteristic, status);

            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d(TAG,"Wrote: " + characteristic.getStringValue(0));
            }

            // continue writing if we have more chunks
            if (writeQueue.isEmpty()) writeInProgress = false;
            else writeQueueFront();
        }

        private void send(String message) {

            // We can only send 20 bytes per packet, so we partition each message
            for (int len = message.length(); len > BLE_UART_TX_BUFFSIZE; len -= BLE_UART_TX_BUFFSIZE) {
                writeQueue.add(message.substring(0, BLE_UART_TX_BUFFSIZE));
                message = message.substring(BLE_UART_TX_BUFFSIZE);
            }

            // lagging bit of message left over
            if (!message.isEmpty()) writeQueue.add(message);

            // tell system to start sending the chunks if it isn't already
            if (!writeInProgress && !writeQueue.isEmpty()) {
                writeInProgress = true;
                writeQueueFront();
            }
        }

        private void writeQueueFront() {
            String chunk = writeQueue.poll();
            mTX.setValue(chunk);
            mGatt.writeCharacteristic(mTX);
        }
    }

    /* Constants */
    private static final String OPEN_OBD_MAC = "e8:e1:d5:e1:73:cd";
    public static final UUID UART_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID TX_UUID   = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID RX_UUID   = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final int BLE_UART_TX_BUFFSIZE = 20;
    public static final String TAG = "OpenOBD";

    /* UI elements */
    private TextView mMessages;
    private EditText mInput;
    private Button   mSendButton;
    private CheckBox mUseNewline;

    /* BLE state */
    private BluetoothAdapter mBLEAdapter;
    private BluetoothDevice mOpenOBD;
    private BluetoothGatt mGatt;

    /* UART state */
    private UARTGattHandler mUART;
    private BluetoothGattCharacteristic mRX, mTX;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Grab references to UI elements.
        mMessages = (TextView) findViewById(R.id.messages);
        mInput = (EditText) findViewById(R.id.input);
        mUseNewline = (CheckBox) findViewById(R.id.newline);

        // Enable auto-scroll in the TextView
        mUseNewline.setMovementMethod(new ScrollingMovementMethod());

        // Disable the send button until we're connected.
        mSendButton = (Button) findViewById(R.id.send);
        mSendButton.setOnClickListener(this);
        enableUI(false);

        // Initialize Bluetooth
        mOpenOBD = null;
        mUART = null;
        mRX = null;
        mTX = null;
        mGatt = null;

        // Make sure BLE is enabled
        mBLEAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBLEAdapter == null || !mBLEAdapter.isEnabled()) {
            appendLine("-- Bluetooth must be enabled --\n");
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
                appendLine("-- OpenOBD module not paired, please pair --\n");
            }
            else {
                // Connect to OpenOBD (Async)
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
        runOnUiThread(() -> mMessages.append(text));
    }

    private void enableUI(final boolean enable) {
        runOnUiThread(() -> {
            mSendButton.setClickable(enable);
            mSendButton.setEnabled(enable);
        });
    }

    // Handler for mouse click on the send button.
    public void onClick(View view) {
        String message = mInput.getText().toString();
        if (message.isEmpty()) return;

        mInput.setText("");
        mInput.setHint("Sent: " + message);
        if (mUseNewline.isChecked())
            message += "\r\n";
        mUART.send(message);
    }
}