#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"

// --- Logic Constants ---
const uint16_t bitPeriod = 104; // 9600 Baud (~104us)
extern TinyGPSPlus gps;
extern bool muzzleActive;

// --- Manual Bit-Bang Transmitter (TX) ---
void rossWrite(char c) {
    digitalWrite(GPS_TX_PIN, LOW); // Start Bit
    delayMicroseconds(bitPeriod);
    for (int i = 0; i < 8; i++) {
        digitalWrite(GPS_TX_PIN, (c >> i) & 0x01);
        delayMicroseconds(bitPeriod);
    }
    digitalWrite(GPS_TX_PIN, HIGH); // Stop Bit
    delayMicroseconds(bitPeriod);
}

void rossPrint(const char* str) {
    while (*str) rossWrite(*str++);
    rossWrite('\r');
    rossWrite('\n');
}

// --- Manual Bit-Bang Receiver (RX) ---
char readRossByte() {
    uint32_t startWait = micros();
    // 1. Wait for Start Bit (Line drops LOW)
    while (digitalRead(GPS_RX_PIN) == HIGH) {
        if (micros() - startWait > 50000) return 0; // 50ms timeout
    }

    // 2. Align to middle of Bit 0
    // We subtract 6us to account for digitalRead execution time
    delayMicroseconds(bitPeriod + (bitPeriod / 2) - 6);

    char incomingByte = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(GPS_RX_PIN) == HIGH) incomingByte |= (1 << i);
        delayMicroseconds(bitPeriod - 3); // Fine-tuned for Nano
    }
    delayMicroseconds(bitPeriod);
    return incomingByte;
}

// --- Lifecycle Functions ---
void setupGPS() {
    pinMode(GPS_RX_PIN, INPUT);
    pinMode(GPS_TX_PIN, OUTPUT);
    digitalWrite(GPS_TX_PIN, HIGH); // Idle High
}

void muzzleGPS(bool closed) {
    muzzleActive = closed;
    if (closed) {
        rossPrint("$PUBX,40,GSA,0,0,0,0*4E");
        rossPrint("$PUBX,40,GSV,0,0,0,0*59");
    } else {
        rossPrint("$PUBX,40,RMC,0,1,0,0*46");
        rossPrint("$PUBX,40,GGA,0,1,0,0*5A");
    }
}

void sipGPS(uint32_t charCount) {
    if (muzzleActive) return;

    for (uint32_t i = 0; i < charCount; i++) {
        char c = readRossByte();
        if (c == 0) break; // Timeout

        // Feed TinyGPS++
        gps.encode(c);

        // Debug output to Serial Monitor
        if (c >= 32 && c <= 126) Serial.print(c);
        else if (c == '\n') Serial.println();
    }
}

#endif