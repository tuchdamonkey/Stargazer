#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "Hardware_config.h"
#include "Diagnostics.h"

// --- GLOBALS & CONFIG ---
#define SILO_SIZE 160
char gpsSilo[SILO_SIZE];
int siloIndex = 0;
bool siloReady = false;

unsigned long lastCarryTime = 0;
const unsigned long carryInterval = 10000;

extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

// === PROTOTYPES ===
void setupGPS();
void captureGpsBurst();

// --- 1. THE BIT-READER (Resyncing on Every Byte) ---
char readRossByte()
{
  // We arrive here exactly when the Start Bit (LOW) is detected
  noInterrupts();

  // Jump to the middle of Bit 0 (~150us from leading edge)
  delayMicroseconds(135);

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    // Strike the bit
    if (digitalRead(GPS_RX_PIN) == HIGH)
    {
      incomingByte |= (1 << i);
    }

    // Delay to reach the next bit center (~104us total with overhead)
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
  siloIndex = 0;
  siloReady = false;
  memset(gpsSilo, 0, SILO_SIZE);

  unsigned long startWait = millis();

  // Wait for the very first sign of life from the GPS
  while (digitalRead(GPS_RX_PIN) == HIGH)
  {
    if (millis() - startWait > 1500)
      return;
  }

  // Once life is detected, fill the bucket byte-by-byte
  while (siloIndex < SILO_SIZE)
  {
    // Re-sync: Wait for the NEXT character's Start Bit (LOW)
    while (digitalRead(GPS_RX_PIN) == HIGH)
      ;

    char c = readRossByte();
    gpsSilo[siloIndex++] = c;

    toggleDiagnostic(); // One pulse per character on D7

    // Detection: End of burst (Look for the second newline)
    if (c == '\n' && siloIndex > 100)
    {
      char *rmcStart = strstr(gpsSilo, "$GPRMC");
      char *ggaStart = strstr(gpsSilo, "$GPGGA");

      if (rmcStart && ggaStart)
      {
        if (isChecksumValid(rmcStart) && isChecksumValid(ggaStart))
        {
          siloReady = true;
        }
        else
        {
          // Failure: Quick stutter on D7
          for (int i = 0; i < 6; i++)
          {
            toggleDiagnostic();
            delayMicroseconds(500);
          }
          siloReady = false;
        }
      }
      break;
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