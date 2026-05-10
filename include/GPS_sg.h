#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "Diagnostics.h"

// --- GLOBALS & CONFIG ---
#define SIPHON_WINDOW_MS 500
#define SILO_SIZE 160
char gpsSilo[SILO_SIZE];
int siloIndex = 0;
bool siloReady = false;

extern bool negotiationActive; // To check if NexStar is talking

unsigned long lastCarryTime = 0;
const unsigned long carryInterval = 10000;

extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;
extern int siloIndex;

// === PROTOTYPES ===
void setupGPS();
void captureGpsBurst();

// --- 1. THE BIT-READER (Resyncing on Every Byte) ---
char readRossByte()
{
  uint8_t pin = GPS_RX_PIN;
  bool inverted = false;

  noInterrupts();
  delayMicroseconds(135);

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    // USE the variables here to satisfy the compiler
    bool bitValue = (inverted) ? (digitalRead(pin) == LOW) : (digitalRead(pin) == HIGH);

    if (bitValue)
    {
      incomingByte |= (1 << i);
    }
    delayMicroseconds(101);
  }

  interrupts();
  return incomingByte;
}

// --- 2. THE VALIDATION GATE ---
bool isChecksumValid(char *sentence)
{
  char *start = strchr(sentence, '$');
  char *end = strchr(sentence, '*');

  if (!start || !end || end < start)
    return false;

  byte calculatedSum = 0;
  for (char *p = start + 1; p < end; p++)
  {
    calculatedSum ^= *p;
  }

  char hex[3] = {*(end + 1), *(end + 2), '\0'};
  byte providedSum = (byte)strtol(hex, NULL, 16);

  return (calculatedSum == providedSum);
}

// --- 3. THE ATOMIC SILO CAPTURE ---
void captureGpsBurst()
{
  unsigned long startSiphon = millis();

  // THE PERSISTENCE WINDOW
  // Stay in this loop for 500ms OR until we fill a silo/get a newline
  while (millis() - startSiphon < SIPHON_WINDOW_MS)
  {
    // MANTRA GUARD: Check NexStar Transmission (D5)
    // Because of MOSFET inversion, HIGH = Bus Activity (Start Bit)
    if (digitalRead(NEX_TX_PIN) == HIGH)
    {
      negotiationActive = true;
      siloIndex = 0;
      return; // Immediate exit to NexStar Pillar
    }

    // Check if a GPS Start Bit (LOW) is present
    if (digitalRead(GPS_RX_PIN) == LOW)
    {
      char c = readRossByte();

      if (siloIndex < SILO_SIZE)
      {
        gpsSilo[siloIndex++] = c;
      }

      // If we finish a sentence, we can leave early!
      if (c == '\n')
      {
        if (siloIndex > 20) // Safety: ensure we didn't just grab a stray newline
        {
          gpsSilo[siloIndex] = '\0'; // Null-terminate for string safety
          siloReady = true;
          break; // Success! Exit the 500ms window immediately
        }
      }
    }
  }
}

// --- 4. HARDWARE SETUP & UTILS ---
void rossWrite(char c)
{
  const uint16_t bitPeriod = 104;
  digitalWrite(GPS_TX_PIN, LOW);
  delayMicroseconds(bitPeriod);
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(GPS_TX_PIN, (c >> i) & 0x01);
    delayMicroseconds(bitPeriod);
  }
  digitalWrite(GPS_TX_PIN, HIGH);
  delayMicroseconds(bitPeriod);
}

void rossPrint(const char *str)
{
  while (*str)
    rossWrite(*str++);
}

void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Silent mode for unused NMEA
  rossPrint("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
  rossPrint("$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n");
  rossPrint("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
  rossPrint("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
  rossPrint("$PUBX,40,RMC,0,1,0,0,0,0*47\r\n");
  rossPrint("$PUBX,40,GGA,0,1,0,0,0,0*5A\r\n");

  currentState = STATE_IDLE;
}

#endif