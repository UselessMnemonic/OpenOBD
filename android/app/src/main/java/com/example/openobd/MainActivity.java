package com.example.openobd;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothManager;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Set;
import java.util.UUID;

public class MainActivity extends Activity {

    private class UARTGattHandler extends BluetoothGattCallback {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);

            // first step of connection process is to wait for connection state to change
            // afterwards, we have to ask the module for services
            switch (newState) {

                // When Bluetooth connects
                case BluetoothGatt.STATE_CONNECTED:
                    if (status == BluetoothGatt.GATT_SUCCESS) gatt.discoverServices();
                    else appendLine("-- Could not connect to " + gatt.getDevice().getAddress() + " --\n");
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
                appendLine("-- Could not connect to " + gatt.getDevice().getAddress() + "  --\n");
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
            appendLine("-- Connected to " + gatt.getDevice().getAddress() + "  --\n");
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gatt, characteristic);

            // print message to text view
            appendLine(characteristic.getStringValue(0));

            //TODO We could do some sort of message control as to reconstruct whole packets
        }

        public void send(String message) {

            // We can only send 20 bytes per packet, so we partition each message
            for (int len = message.length(); len > BLE_UART_TX_BUFFSIZE; len -= BLE_UART_TX_BUFFSIZE) {
                mTX.setValue(message.substring(0, BLE_UART_TX_BUFFSIZE));
                message = message.substring(BLE_UART_TX_BUFFSIZE);
            }

            // lagging bit of message left over
            if (!message.isEmpty()) {
                mTX.setValue(message);
            }
        }
    }

    /* Constants */
    private static final String OPEN_OBD_MAC = "e8:e1:d5:e1:73:cd";
    public static final UUID UART_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID TX_UUID   = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final UUID RX_UUID   = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    public static final int BLE_UART_TX_BUFFSIZE = 20;

    /* UI elements */
    private TextView mMessages;
    private EditText mInput;
    private Button   mSendButton;
    private CheckBox mUseNewline;

    /* BLE state */
    private BluetoothManager mBLEManager;
    private BluetoothAdapter mBLEAdapter;
    private BluetoothGatt mGatt;
    private BluetoothDevice mOpenOBD;

    /* UART state */
    private UARTGattHandler mUART;
    private BluetoothGattCharacteristic mRX, mTX;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Grab references to UI elements.
        mMessages = findViewById(R.id.messages);
        mInput = findViewById(R.id.input);
        mUseNewline = findViewById(R.id.newline);

        // Enable auto-scroll in the TextView
        mUseNewline.setMovementMethod(new ScrollingMovementMethod());

        // Disable the send button until we're connected.
        mSendButton = findViewById(R.id.send);
        enableUI(false);

        // Initialize Bluetooth
        mOpenOBD = null;
        mUART = null;
        mRX = null;
        mTX = null;

        // Make sure BLE is enabled
        mBLEAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBLEAdapter == null || !mBLEAdapter.isEnabled()) {
            Toast.makeText(this, "Bluetooth must be enabled.", Toast.LENGTH_LONG).show();
            finish();
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
                Toast.makeText(this, "You must pair with the OpenOBD module.", Toast.LENGTH_LONG).show();
                finish();
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
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mMessages.append(text);
            }
        });
    }

    private void enableUI(final boolean enable) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mSendButton.setClickable(enable);
                mSendButton.setEnabled(enable);
            }
        });
    }

    // Handler for mouse click on the send button.
    public void sendClick(View view) {

        // Make string and add newline if requested
        StringBuilder stringBuilder = new StringBuilder();

        String message = mInput.getText().toString();
        if (mUseNewline.isChecked())
            message += "\r\n";
        mUART.send(message);
    }
}