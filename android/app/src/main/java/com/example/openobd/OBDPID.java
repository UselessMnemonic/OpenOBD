package com.example.openobd;

import androidx.annotation.NonNull;

/* Enumeration of OBD PID choices */
public enum OBDPID {
    SUPPORT("PIDs Supported", 0x00),
    FUEL("Fuel System Status", 0x03),
    ENGINE("Calculated Engine Load", 0x04),
    TEMP("Engine Coolant Temperature", 0x05),
    RPM("Engine RPM", 0x0C);

    private String name;
    private int pid;
    OBDPID(String name, int pid) {
        this.name = name;
        this.pid = pid;
    }
    String getName() { return name; }
    int getPID() { return pid; }

    @NonNull
    @Override
    public String toString() {
        return getName() + " (" + getPID() + ")";
    }
}
