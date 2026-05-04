#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>

#define SYNC_PIN 7 

// ACTIVATE THIS MODE FOR THE STRIKE
#define SYNC_MODE_EVENT 

void initSyncPin() {
    pinMode(SYNC_PIN, OUTPUT);
    digitalWrite(SYNC_PIN, LOW); // Forces LOW to confirm silence on power-up
}

inline void syncEventAnchor(void (*logicPayload)()) {
  #ifdef SYNC_MODE_EVENT
    digitalWrite(SYNC_PIN, HIGH); // Signal start of critical window
    if (logicPayload) {
        logicPayload(); // Execute the actual packet transmission[cite: 3]
    }
    digitalWrite(SYNC_PIN, LOW);  // Signal completion[cite: 3]
  #endif
}

#endif