#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- GPS (NEO-7M) ---
#define GPS_RX_PIN 3 // Vetted: Connects to Nano D3
#define GPS_TX_PIN 2 // Vetted: Connects to Nano D2

// --- NexStar (Aux Port) ---
#define NEX_TX_PIN 4 // Vetted: Hardware Serial TX
#define NEX_RX_PIN 5 // Vetted: Hardware Serial RX

// --- System Indicators ---
#define STATUS_LED_RED 16 // Built-in Nano LED
#define STATUS_LED_YEL 15 // Built-in Nano LED

// --- Bridge-Guard States ---
enum SystemState
{
    STATE_NEX_LISTENING, // Default: Nano is paying full attention to the telescope bus
    STATE_NEX_ENGAGED,   // Locked: Telescope is actively talking. Deep "Do Not Disturb" mode
    STATE_GPS_SIPHON     // Lease: Telescope is silent, Nano has permission to sip GPS data
};

// These tell other files that these variables exist somewhere else (in main.cpp)
extern volatile SystemState currentState;
extern volatile int bufIndex;
extern char goldenPacket[85];

#endif