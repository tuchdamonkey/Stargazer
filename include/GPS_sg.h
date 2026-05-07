#ifndef GPS_SG_H
#define GPS_SG_H

#include <Arduino.h>
#include "Hardware_config.h"
#include "Diagnostics.h"

#define SILO_SIZE 160
char gpsSilo[SILO_SIZE];
int siloIndex = 0;
bool siloReady = false;

// We'll use this to keep track of the 10s Porthole
unsigned long lastCarryTime = 0;
const unsigned long carryInterval = 10000;

// --- Step 1: The Validation Filter ---
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

// --- Step 2: The Atomic Capture ---
void captureGpsBurst()
{
  siloIndex = 0;
  siloReady = false;
  memset(gpsSilo, 0, SILO_SIZE);

  unsigned long startWait = millis();
  while (digitalRead(GPS_RX_PIN) == HIGH)
  {
    if (millis() - startWait > 1500)
      return;
    // Future NEX interrupt check here
  }

  while (siloIndex < SILO_SIZE)
  {
    char c = readRossByte();
    gpsSilo[siloIndex++] = c;

    toggleDiagnostic(); // D7 progress pulses

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
          // Visual "Stutter" for checksum failure
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

// --- Global Buffer & State (Volatile is required for ISR safety) ---
extern char goldenPacket[85];
extern volatile int bufIndex;
extern volatile SystemState currentState;

void captureGpsBurst()
{
  // Reset Silo
  siloIndex = 0;
  siloReady = false;
  memset(gpsSilo, 0, SILO_SIZE);

  // Vigilance: Wait for Start Bit (GPS RX goes LOW)
  unsigned long startWait = millis();
  while (digitalRead(GPS_RX_PIN) == HIGH)
  {
    if (millis() - startWait > 1500)
      return;

    // Potential NEX check would live here
    // if (Serial.available()) return;
  }

  // Atomic Fill
  while (siloIndex < SILO_SIZE)
  {
    char c = readRossByte(); // Your validated timing code
    gpsSilo[siloIndex++] = c;

    // Diagnostic toggle on D7 - Physical progress bar
    toggleDiagnostic();

    // End of burst detection (Look for the second newline)
    if (c == '\n' && siloIndex > 100)
    {
      // --- THE VALIDATION GATE ---
      // We look for the start of both expected sentences
      char *rmcStart = strstr(gpsSilo, "$GPRMC");
      char *ggaStart = strstr(gpsSilo, "$GPGGA");

      if (rmcStart && ggaStart)
      {
        // Only set ready if BOTH checksums pass
        if (isChecksumValid(rmcStart) && isChecksumValid(ggaStart))
        {
          siloReady = true;
        }
        else
        {
          // Checksum Fail: Quick D7 stutter for visual warning on LA
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

// --- 1. The Core Utilities (Precision Timing) ---

char readRossByte()
{
  // 1. Pause the world so timing is perfect
  noInterrupts();

  delayMicroseconds(146); // Using your calibrated jump

  char incomingByte = 0;
  for (int i = 0; i < 8; i++)
  {
    SYNC_HIGH();

    if (digitalRead(GPS_RX_PIN) == HIGH)
    {
      incomingByte |= (1 << i);
    }

    SYNC_LOW();

    // We use a slightly tighter delay because
    // digitalRead and SYNC_HIGH/LOW take ~5-6us total
    delayMicroseconds(98);
  }

  // 2. Resume the world
  interrupts();

  return incomingByte;
}

void rossWrite(char c)
{
  const uint16_t bitPeriod = 104;
  digitalWrite(GPS_TX_PIN, LOW); // Start Bit
  delayMicroseconds(bitPeriod);
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(GPS_TX_PIN, (c >> i) & 0x01);
    delayMicroseconds(bitPeriod);
  }
  digitalWrite(GPS_TX_PIN, HIGH); // Stop Bit
  delayMicroseconds(bitPeriod);
}

void rossPrint(const char *str)
{
  while (*str)
    rossWrite(*str++);
}

// --- 2. The Bridge-Guard Filter ---

void processGPSByte(char c)
{
  if (c == '$')
  {
    bufIndex = 0;
  }

  if (bufIndex < 84)
  {
    goldenPacket[bufIndex++] = c;
    goldenPacket[bufIndex] = '\0';
  }

  // HEADER FILTER: Reject non-RMC/GGA sentences after first 6 chars
  if (bufIndex == 6)
  {
    if (strstr(goldenPacket, "RMC") == NULL && strstr(goldenPacket, "GGA") == NULL)
    {
      bufIndex = 0;
      currentState = STATE_IDLE; // Kill the sentence early
      return;
    }
  }

  // END OF SENTENCE: Trigger validation
  if (c == '\n')
  {
    currentState = STATE_VALIDATE;
  }
  else
  {
    // STAGE 2 FIX: Stay in IDLE while waiting for the NEXT character's start bit.
    // The ISR will flip us back to ACQUIRE when the next byte starts falling.
    currentState = STATE_IDLE;
  }
}

// --- 3. The Setup & Interrupt Architecture ---

void setupGPS()
{
  pinMode(GPS_RX_PIN, INPUT_PULLUP);
  pinMode(GPS_TX_PIN, OUTPUT);
  digitalWrite(GPS_TX_PIN, HIGH);

  // Initial hardware muzzle (UBX commands for NEO-7M)
  rossPrint("$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n");
  rossPrint("$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n");
  rossPrint("$PUBX,40,GSV,0,0,0,0,0,0*59\r\n");
  rossPrint("$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n");
  rossPrint("$PUBX,40,RMC,0,1,0,0,0,0*47\r\n");
  rossPrint("$PUBX,40,GGA,0,1,0,0,0,0*5A\r\n");

  // ENABLE PCINT (Pin Change Interrupt) for D3
  cli();
  PCICR |= (1 << PCIE2);    // Enable Port D interrupts
  PCMSK2 |= (1 << PCINT19); // Trigger on Digital Pin 3
  sei();

  currentState = STATE_IDLE;
}

// THE SENTRY: This ISR catches the GPS talking in the background
ISR(PCINT2_vect)
{
  // If the pin dropped LOW and we are IDLE, start bit detected
  if (digitalRead(GPS_RX_PIN) == LOW && currentState == STATE_IDLE)
  {
    currentState = STATE_ACQUIRE;
  }
}

#endif