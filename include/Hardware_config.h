#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- GPS (NEO-7M) ---
#define GPS_RX_PIN 3 // Vetted: Connects to Nano D3
#define GPS_TX_PIN 2 // Vetted: Connects to Nano D2

// --- NexStar (Aux Port) ---
#define NEX_TX_PIN 5 // Vetted: Hardware Serial TX
#define NEX_RX_PIN 4 // Vetted: Hardware Serial RX

// --- System Indicators ---
#define STATUS_LED_RED 16 // Built-in Nano LED
#define STATUS_LED_YEL 15 // Built-in Nano LED

// --- Bridge-Guard States ---
enum SystemState
{
    STATE_IDLE,     // Waiting for a Start Bit from GPS
    STATE_ACQUIRE,  // Actively bit-banging a GPS character
    STATE_VALIDATE, // Full sentence received, checking Checksum
    STATE_RELAY     // Pushing "Golden Packet" to NexStar
};

// These tell other files that these variables exist somewhere else (in main.cpp)
extern volatile SystemState currentState;
extern volatile int bufIndex;
extern char goldenPacket[85];

#endif