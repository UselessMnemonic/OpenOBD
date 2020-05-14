package com.example.openobd;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothDevice;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        BluetoothDevice btd;


        setContentView(R.layout.activity_main);
    }
}
