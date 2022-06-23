/* M5Stickc & Arduino Nano 33 BLE Muscle control for KINETIC Hand 
 * Rev 1.0, Original project, Praga Michele <pragamichele@gmail.com>
 * Rev 1.1, added hold mode and support for Arduino Nano BLE 33, Alberto Navatta <alberto@e-nableitalia.it>
 *  
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
 * ANY CLAIM, DAMAGES,OR OTHER LIABILITY. YOU AGREE TO USE AT YOUR OWN RISK.
*/

// M5StickC Required libs:
// * ESP32 Lite Pack
// * ServoESP32
// Generic libs:
// * OneButton
// Add cpp flag to xtensa-esp32-elf-gcc (platform.txt in C:\Users\%USER\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6): -std=gnu++11

#include <stdarg.h>
#include <stdio.h>
#include <Servo.h>

#include "DeltaTime.h"

#include "global_config.h"

#include "HandState.h"
#include "console_debug.h"

// global defines

// uncomment to enable pressure sensor simulation
// #define SIMULATED

// enable one of the two operating modes:
// PROGRESSIVE -> open / close progressively according to pressure sensor value (original code)
// HOLD TIME -> discard small muscle contractions (duration < 250ms)
//              trigger closure after pressure is detected for a period > hold time ( > 250ms)
//              closure lasts till pressure is detected (closure speed is proportional to pressure)
//              opening is triggered by a pressude detection for a period > hold time ( > 250ms)
//              opening continues till the hand is completely open
//
//#define OPERATING_MODE MODE_PROGRESSIVE
#define OPERATING_MODE MODE_HOLDTIME

#ifdef SIMULATED
#define LOOP_DELAY        50  // delay main loop of 50 mS in simulation
#else
#define LOOP_DELAY        10  // delay main loop of 10 mS, to keep higher sample rate for pressure and low delay during hand opening
                              // TBV, use of a prescaler for servo increment (feature to be evaluated)

#endif

#ifdef SIMULATED
// simulate sensor pressure with an array of pressure values and loop count
// once the pressure simulator scans all the _simEvents in the array the 
// event counter restarts
struct SimulatedPressure {
  int value;    // PRESSURE VALUE, the pressure value returned in simulated mode
  int count;    // LOOP COUNTS, number of times the value is returned before to jump to the next one
};

SimulatedPressure _simEvents[] = {
  { 0, 50 }, // pressure 0, duration 50*50ms -> 2.5s
  { 100, 2 }, // pressure 100, duration 100ms
  { 0, 50 }, // pressure 0, delay 50*50ms -> 2.5s
  { 200, 3 }, // pressure 200, duration 150ms
  { 300, 5 }, // pressure 300, duration 250ms -> should trigger handle close
  { 500, 100 }, // pressure 500, duration 5s -> continue closing
  { 300, 5 }, // pressure 300, duration 250ms -> continue closing
  { 200, 3 }, // pressure 200, duration 150ms
  { 0, 50 }, // pressure 0, delay 50*50ms -> 2.5s
  { 500, 10 }, // pressure 500, duration 500ms -> start opening
  { 0, 200 }, // pressure 0, delay 10s
};

int _simIndex = -1;
int _simPressure = 0;
int _simCount = 0;

#define MAX_SIMULATED_EVENTS sizeof(_simEvents) / sizeof(SimulatedPressure)
#endif

// default, updated by EEPROM
int setpoint = 240;  

// value of pressure sensor
int pressure = 6;

// max servo angle
//int servo_max_angle = SERVO_MAX;

// hand operating mode
int mode = OPERATING_MODE;

int corsaservo = 0;

DeltaTime interval;

#if (OPERATING_MODE == MODE_HOLDTIME)
HandState *handState = HandStateIdleOpen::instance().enter();
#endif
//End Globals Objects initialization

int getPressure() {
  // logic
#ifdef SIMULATED
  if (_simCount == 0) {
    _simIndex = (_simIndex+1) %  MAX_SIMULATED_EVENTS;
    _DEBUG("New simIndex[%d]", _simIndex);
    _simCount = _simEvents[_simIndex].count;
  } else
    _simCount--;

  int raw_pressure = _simEvents[_simIndex].value;
  _DEBUG("Simulated Pressure, index[%d], value[%d], count[%d]",_simIndex,raw_pressure,_simCount);
#else
  int raw_pressure = analogRead(SENSOR_PIN);
#endif

  // normalized pressure
  return (raw_pressure < PRESSURE_MAX) ? raw_pressure : PRESSURE_MAX;
}

void setup() {
  //Initialize Serial
  Serial.begin(115200);       
  handInit();

#ifdef ARDUINO_M5Stick_C
  M5init();
#endif // ARDUINO_M5Stick_C
#ifdef SIMULATED
  _DEBUG("Init: sleep 5 seconds");
  delay(5000);
#endif
}

void loop () {
#ifdef ARDUINO_M5Stick_C
  M5loop();
#endif // ARDUINO_M5Stick_C

  pressure = getPressure();
  _DEBUG("Detected Pressure[%d]",pressure);

  // update progress bar and led state
#ifdef ARDUINO_M5Stick_C
  progressBar(pressure, pressure > setpoint);  
#endif // ARDUINO_M5Stick_C

  if (mode == MODE_PROGRESSIVE)
      updateServoProgressive(pressure);
  else {
    if (handState) { // just to be safe check pointer valid
      _DEBUG("State[%s]", handState->getName());
      handState = handState->update(pressure);
    }
  }
}  
