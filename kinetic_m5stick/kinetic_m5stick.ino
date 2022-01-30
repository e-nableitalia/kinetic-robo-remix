/* M5Stickc & Arduino Nsno 33 BLE Muscle control for KINETIC Hand 
 * Rev 1.0, Original project, Praga Michele
 * Rev 1.1, added hold mode and support for Arduino Nano BLE 33, Alberto Navatta
 *  
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
 * ANY CLAIM, DAMAGES,OR OTHER LIABILITY. YOU AGREE TO USE AT YOUR OWN RISK.
 * Modified from Praga Michele 
*/

// M5StickC Required libs:
// * ESP32 Lite Pack
// * ServoESP32
// Generic libs:
// * OneButton
// Add cpp flag to xtensa-esp32-elf-gcc (platform.txt in C:\Users\%USER\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6): -std=gnu++11

#include <stdarg.h>
#include <stdio.h>
#include "nano33bleboard.h"
#include <Servo.h>
#include <ArduinoBLE.h>
#include "DeltaTime.h"

#include "global_config.h"
#include "EMGFilters.h"
#include "HandState.h"
#include "console_debug.h"


//#include <EMG_Hal.h>

// global defines
// uncomment to enable pressure sensor simulation
//#define SIMULATED

#ifdef SIMULATED
#define LOOP_DELAY        100  // delay main loop of 100 mS in simulation
#else
#define LOOP_DELAY        50  // delay main loop of 50 mS, to keep higher pressure sample rate and delay hand opening it is also possible to use a servo increment prescaler (feature to be evaluated)
#endif

#ifdef SIMULATED
struct SimulatedPressure {
  int value;    // PRESSURE VALUE
  int count;    // LOOP COUNTS
};

SimulatedPressure _simEvents[] = {
  { 0, 50 }, // pressure 0, duration 50*50ms -> 2.5s
  { 100, 2 }, // pressure 100, duration 150ms
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

#define MAX_SIMULATED_EVENTS 11
#endif

// default, updated by EEPROM
int setpoint = 240;  

// value of pressure sensor
int pressure = 6;
unsigned long threshold = 0;  // threshold: Relaxed baseline values.(threshold=0:in the calibration process)
unsigned long EMG_num = 0;      // EMG_num: The number of statistical signals

EMGFilters myFilter;
////////////////////////BLE SETTINGS//

const int ledPin = LED_BUILTIN; // set ledPin to on-board LED
const int buttonPin = 4; // set buttonPin to digital pin 4

BLEService ledService("Read Movement"); // create service

// create switch characteristic and allow remote device to read and write
BLEByteCharacteristic ledCharacteristic("Show Output", BLERead | BLEWrite);
// create button characteristic and allow remote device to get notifications
BLEByteCharacteristic buttonCharacteristic("Stuff", BLERead | BLENotify);



// max servo angle
//int servo_max_angle = SERVO_MAX;

// hand mode
int mode = MODE_HOLDTIME;

int corsaservo = 0;
//Globals Object initialization


DeltaTime interval;

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
  //return (raw_pressure < PRESSURE_MAX) ? raw_pressure : PRESSURE_MAX;
  return raw_pressure;
}

// state update function
void updateState(int pressure) {
  
#if 1

#else
  _DEBUG("Entering state[%s]",_state[hand_state]);
  
  switch (hand_state) {
      case HOLD_OPENING:
          if (pressure > PRESSURE_MIN) {
              int delta = interval.delta(INTERVAL_OPEN);
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check hold interval[%d] expired[%d]",pressure, PRESSURE_MIN, delta, INTERVAL_OPEN);
              if (delta > HOLDTIME_INTERVAL_uS) {
                  _DEBUG("OPEN hold interval expired, start closing hand");
                  hand_state = CLOSING;
              }
          } else {
              // discard small muscle contraction and return idle
              _DEBUG("Muscle contraction discarded, inteval too small");
              hand_state = IDLE_OPEN;
          }
          break;
      case CLOSING:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can continue to close hand",pressure, PRESSURE_MIN);
              if (servo_angle < SERVO_MAX) {
                  int magnitude = (pressure / 50) + 1;
                  servo_angle = servo_angle + magnitude;
                  _DEBUG("Current servo angle[%d], magnitude[%d]",servo_angle, magnitude);
                  servo.write(servo_angle);
              } else {
                  _DEBUG("Current servo angle[%d], hand completely closed",servo_angle);
                  hand_state = CLOSED;
              }
          } else {
              _DEBUG("Pressure[%d] < PRESSURE_MIN[%d], stop closing hand",pressure, PRESSURE_MIN);
              hand_state = CLOSED;
          }
          break;
       case CLOSED:
          _DEBUG("Check Pressure[%d] < PRESSURE_MIN[%d] to go in IDLE_CLOSE",pressure, PRESSURE_MIN);
          if (pressure < PRESSURE_MIN) {
              _DEBUG("Going in IDLE_CLOSED state");
              hand_state = IDLE_CLOSED;
          }
          break;
       case HOLD_CLOSING:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can close hand",pressure, PRESSURE_MIN);
              if (interval.delta(INTERVAL_CLOSE) > HOLDTIME_INTERVAL_uS) {
                  _DEBUG("CLOSE hold interval expired, closing hand");
                  hand_state = OPENING;
              }
          } else {
              // discard small muscle contraction and return idle
              _DEBUG("Muscle contraction discarded, inteval too small");
              hand_state = IDLE_CLOSED;
          }
          break;
      case OPENING:
          _DEBUG("Opening hand, servo hangle -> [%d]",SERVO_MIN);
          servo.write(SERVO_MIN);
          servo_angle = SERVO_MIN;
          hand_state = OPEN;
          break;
      case OPEN:
           _DEBUG("Check Pressure[%d] < PRESSURE_MIN[%d] to go in IDLE_OPEN",pressure, PRESSURE_MIN);
          if (pressure < PRESSURE_MIN) {
              _DEBUG("Going in IDLE_OPEN state");
              hand_state = IDLE_OPEN;
          }
          break;
      case IDLE_CLOSED:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], start hold close timer",pressure, PRESSURE_MIN);
              hand_state = HOLD_CLOSING;
              interval.start(INTERVAL_CLOSE);
          }
          break;
      case IDLE_OPEN:
      default:
        if (pressure > PRESSURE_MIN) {
          _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], start hold open timer",pressure, PRESSURE_MIN);
          hand_state = HOLD_OPENING;
          interval.start(INTERVAL_OPEN);
        }
  }
#endif

  // loop delay
  _DEBUG("Sleeping %d ms",LOOP_DELAY);
  delay(LOOP_DELAY);
}
int o = 0 ;
void setupBLE()
{
    
  while (!Serial);

  pinMode(ledPin, OUTPUT); // use the LED as an output
  pinMode(buttonPin, INPUT); // use button pin as an input

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("Enable Kinetic");
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(ledService);

  // add the characteristics to the service
  ledService.addCharacteristic(ledCharacteristic);
  ledService.addCharacteristic(buttonCharacteristic);

  // add the service
  BLE.addService(ledService);

  ledCharacteristic.writeValue(o);
  buttonCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}
void blePoll(int val)
{
  
  BLE.poll();

  // read the current button pin state
  char buttonValue = digitalRead(buttonPin);

  // has the value changed since the last read
  boolean buttonChanged = (buttonCharacteristic.value() != buttonValue);

  if (buttonChanged) {

  }

  if (ledCharacteristic.written() || buttonChanged) {
    // update LED, either central has written to characteristic or button state has changed
    if (ledCharacteristic.value()) {
      
      
    } else {
      

    }
  }
  Serial.println(o);
  ledCharacteristic.writeValue(val);
    buttonCharacteristic.writeValue(val);
}
void setup() {
  //Initialize Serial
  Serial.begin(115200);       
  handInit();
  setupBLE();

#ifdef ARDUINO_M5Stick_C
  M5init();
#endif // ARDUINO_M5Stick_C
#ifdef SIMULATED
  _DEBUG("Init: sleep 5 seconds");
  delay(5000);
#endif
}



int getEMGCount(int gforce_envelope)
{
  static long integralData = 0;
  static long integralDataEve = 0;
  static bool remainFlag = false;
  static unsigned long timeMillis = 0;
  static unsigned long timeBeginzero = 0;
  static long fistNum = 0;
  static int  TimeStandard = 200;
  /*
    The integral is processed to continuously add the signal value
    and compare the integral value of the previous sampling to determine whether the signal is continuous
   */
  integralDataEve = integralData;
  integralData += gforce_envelope;
  /*
    If the integral is constant, and it doesn't equal 0, then the time is recorded;
    If the value of the integral starts to change again, the remainflag is true, and the time record will be re-entered next time
  */
  if ((integralDataEve == integralData) && (integralDataEve != 0))
  {
    timeMillis = millis();
    if (remainFlag)
    {
      timeBeginzero = timeMillis;
      remainFlag = false;
      return 0;
    }
    /* If the integral value exceeds 200 ms, the integral value is clear 0,return that get EMG signal */
    if ((timeMillis - timeBeginzero) > TimeStandard)
    {
      integralDataEve = integralData = 0;
      return 1;
    }
    return 0;
  }
  else {
    remainFlag = true;
    return 0;
   }
}
int emgfilterNano()
{
  int data = analogRead(SENSOR_PIN);
  int dataAfterFilter = myFilter.update(data);  // filter processing
  int envelope = sq(dataAfterFilter);   //Get envelope by squaring the input
  envelope = (envelope > threshold) ? envelope : 0;    // The data set below the base value is set to 0, indicating that it is in a relaxed state

  /* if threshold=0,explain the status it is in the calibration process,the code bollow not run.
     if get EMG singal,number++ and print
  */
  if (threshold > 0)
  {
    if (getEMGCount(envelope))
    {
      EMG_num++;
      
      Serial.println(EMG_num);
    }
  }
  else {
    
    if ( envelope > 250000)
    {
    Serial.println(envelope);
    }
    
  }
  delayMicroseconds(1000);
      Serial.println(envelope/10000);
  return envelope/10000;
}
void loop () {
  
#ifdef ARDUINO_M5Stick_C
  M5loop();
#endif // ARDUINO_M5Stick_C


  //pressure = getPressure();
  
  blePoll(emgfilterNano());
  
  //_DEBUG("Detected Pressure[%d]",pressure);

  // update progress bar and led state
#ifdef ARDUINO_M5Stick_C
  progressBar(pressure, pressure > setpoint);  
#endif // ARDUINO_M5Stick_C

 /* if (mode == MODE_PROGRESSIVE)
      updateServoProgressive(pressure);
  else
      updateState(pressure);*/
}  
