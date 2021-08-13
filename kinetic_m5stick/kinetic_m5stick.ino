
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
// ESP32 Lite Pack
// ServoESP32
// Generic libs:
// OneButton

#include <stdarg.h>
#include <stdio.h>
#include <Servo.h>
#include <OneButton.h>

// global defines
#define DEBUG
// uncomment to enable pressure sensor simulation
// #define SIMULATED

#ifdef ESP32
#include <M5StickC.h>
#include <EEPROM.h>

#pragma message("Using ESP32 M5StickC board configuration");

#define M5STICKC

// PINS
#define SERVO_PIN   G26
#define LED_PIN     G10
#define SENSOR_PIN  G36
#define BUTTONA_PIN G39
#define BUTTONB_PIN G37

RTC_TimeTypeDef RTC_TimeStruct;

#else // ! ESP32
#if TARGET_NAME == ARDUINO_NANO33BLE

#pragma message("Using Arduino Nano 33 BLE board configuration");

// PINS
#define SERVO_PIN   D9
#define LED_PIN     PIN_LED
#define SENSOR_PIN  A0
#define BUTTONA_PIN D0
#define BUTTONB_PIN D1
#endif
#endif

#define SERVO_MAXANGLE    270
#define MODE_PROGRESSIVE  1   // firmware mode = hold
#define MODE_HOLDTIME     2   // 
#ifdef SIMULATED
#define LOOP_DELAY        100  // delay main loop of 100 mS in simulation
#else
#define LOOP_DELAY        50  // delay main loop of 50 mS, to keep higher pressure sample rate and delay hand opening it is also possible to use a servo increment prescaler (feature to be evaluated)
#endif
#define EEPROM_SIZE       1
#define INTERVAL_OPEN     0   // hand open interval id, used to calculate hold time since first muscle contraction -> activate hand/servo closure
#define INTERVAL_CLOSE    1   // hand close interval id, used to calculate hold time since first muscle contraction -> trigger hand open operation
#define HOLDTIME_INTERVAL_uS   (250 * 1000)  // 250 ms, hold time
#define PRESSURE_MAX      600 // could be 1200
#define PRESSURE_MIN      10  // min pressure value to trigger hand closure/opening
#define PROGRESSBAR_MIN   0
#define PROGRESSBAR_MAX   127
#define FORMAT_BUFFERSIZE 1024
// DMS 
// progress bar Y offset
//#define Y_OFFSET(x) (15+(127-x))
#define Y_OFFSET(x) (15+(254-x))

#ifdef DEBUG
#define _DEBUG(...)   console_debug(__func__, __VA_ARGS__)
#else
#degine _DEBUG(...)
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

// for servo 270 degrees maximum lever 6-7mm
#if (SERVO_MAXANGLE == 270)
#define SERVO_MAX 	254
#define SERVO_MIN	  1
#endif // SERVO 270°

// servo 180 degrees maximum lever 11-12mm
#if (SERVO_MAXANGLE == 180)
#define SERVO_MAX 	150
#define SERVO_MIN	  60
#endif // SERVO 180°

// value of pressure sensor
int pressure = 6;

// possible hand states, default IDLE_OPEN
enum _hand_state {
  IDLE_OPEN = 0,
  HOLD_OPENING,
  OPENING,
  HOLD_CLOSING,
  CLOSING,
  IDLE_CLOSED,
  CLOSED,
  OPEN
} hand_state = IDLE_OPEN;

char *_state[] = {
  "IDLE_OPEN",
  "HOLD_OPENING",
  "OPENING",
  "HOLD_CLOSING",
  "CLOSING",
  "IDLE_CLOSED",
  "CLOSED",
  "OPEN",
  0
};

// current servo angle
int servo_angle = 0;

// max servo angle
int servo_max_angle = SERVO_MAX;

// hand mode
int mode = MODE_HOLDTIME;

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

int setpoint = 240;  // deault, updated by EEPROM
int corsaservo = 0;
int count = 0;
int state = LOW;
int lastState = LOW;

// DeltaTime class, measures intervals in microseconds
#define MAX_TIMERS      5

class DeltaTime {
public:
    DeltaTime() {
        for (int i=0; i< MAX_TIMERS; i++)
            last_ticks[i] = 0;
#if defined (STM32F2XX)  // Photon     
        ticks_per_micro = System.ticksPerMicrosecond();
#else
        ticks_per_micro = 1;
#endif
    }
    void start(int i) {
#if defined (STM32F2XX) // Photon
      last_ticks[i] = System.ticks();
#else
      last_ticks[i] = micros();
#endif
    }

    uint32_t delta(int i) {
#if defined (STM32F2XX) // Photon   
        uint32_t current_ticks = System.ticks();
#else
        uint32_t current_ticks = micros();
#endif
        if (last_ticks[i] != 0) {
            uint32_t _d = (current_ticks - last_ticks[i])/ticks_per_micro;
            //last_ticks[i] = current_ticks;
            if (current_ticks < last_ticks[i]) // normalize
              return (UINT32_MAX - _d);
            else
              return _d;
        } 
        //else
        //    last_ticks[i] = current_ticks;

        return 0;
    }

    uint32_t deltaandrestart(int i) {
#if defined (STM32F2XX) // Photon   
        uint32_t current_ticks = System.ticks();
#else
        uint32_t current_ticks = micros();
#endif
        if (last_ticks[i] != 0) {
            uint32_t _d = (current_ticks - last_ticks[i])/ticks_per_micro;
            last_ticks[i] = current_ticks;
            if (current_ticks < last_ticks[i]) // normalize
              return (UINT32_MAX - _d);
            else
              return _d;
        } else
            last_ticks[i] = current_ticks;

        return 0;
    }

private:
    uint32_t last_ticks[MAX_TIMERS];  
    double ticks_per_micro = 0;
};

// Serial debug function
// print function name and formatted text
// accepted format arguments:
// %d, %i -> integer
// %s -> string
void console_debug(const char *function, const char *format_str, ...) {
  
    va_list argp;
    va_start(argp, format_str);
    char *bp=format_buffer;
    int bspace = FORMAT_BUFFERSIZE - 1;

    while ((*function) && (bspace)) {
      *bp++ = *function++;
      --bspace;
    }

    *bp++ = ':';
    --bspace;
    *bp++ = ' ';
    --bspace;

    while (*format_str != '\0' && bspace > 0) {
      if (*format_str != '%') {
        *bp++ = *format_str++;
        --bspace;
      } else if (format_str[1] == '%') // An "escaped" '%' (just print one '%').
      {
        *bp++ = *format_str++;    // Store first %
        ++format_str;             // but skip second %
        --bspace;
      } else {
         ++format_str;
        // parse format
        switch (*format_str) {
          case 's': {
            // string
            char *str = va_arg (argp, char *);
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          case 'd': case 'i': {
            // decimal
            char ibuffer[16];
            int val = va_arg (argp, int);
            snprintf(ibuffer,16,"%d",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          default: {
            // skip format
          }
        }
         
        ++format_str;
      }
    }
    // terminate string
    *bp = 0;
    Serial.println(format_buffer);
}

//Globsl Object initialization
OneButton btnA(BUTTONA_PIN, true);
OneButton btnB(BUTTONB_PIN, true);
Servo servo;
DeltaTime interval;

#ifdef M5STICKC
void updateCalibration(int value)
{
  EEPROM.write(0, value);  // save in EEPROM
  EEPROM.commit();
  // clear old line
  M5.Lcd.drawLine(1, Y_OFFSET(setpoint), 4, Y_OFFSET(setpoint), BLACK);
  M5.Lcd.drawLine(26, Y_OFFSET(setpoint), 29, Y_OFFSET(setpoint), BLACK);
  // set new line
  setpoint = value; //set global
  M5.Lcd.drawLine(1, Y_OFFSET(setpoint), 4, Y_OFFSET(setpoint), 0x7bef);
  M5.Lcd.drawLine(26, Y_OFFSET(setpoint), 29, Y_OFFSET(setpoint), 0x7bef);
}

void readCalibration()
{
  setpoint = EEPROM.read(0);  // retrieve calibration in EEPROM
  // set new line
  M5.Lcd.drawLine(1, Y_OFFSET(setpoint), 4, Y_OFFSET(setpoint), 0x7bef);
  M5.Lcd.drawLine(26, Y_OFFSET(setpoint), 29, Y_OFFSET(setpoint), 0x7bef);
}

//Show MP instructions
void updateUI() {
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(37, 10);
  M5.Lcd.printf("MP");

  // Show count instruction
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(31, 40);
  M5.Lcd.println(count);

  //Show OFF instructions
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(33, 140);
  M5.Lcd.printf("OFF");

  //Show Limit instructions
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(31, 60);
  M5.Lcd.printf("Limit ->");

  //Show Bat instructions
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(31, 95);
  M5.Lcd.printf("Bat");

  //Show vbat
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(31, 117);
  M5.Lcd.printf("%.2f\r\n ", M5.Axp.GetBatVoltage());
  M5.Lcd.setSwapBytes(true);
  delay (90);
  M5.Lcd.fillScreen(BLACK);
 
  //Show MP instructions
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(37, 10);
  M5.Lcd.printf("MP");

  //Show OFF instructions
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(33, 140);
  M5.Lcd.printf("OFF");

  //Show Limit instructions
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(31, 60);
  M5.Lcd.printf("Limit ->");

  //Show time
   M5.Rtc.GetTime(&RTC_TimeStruct);
   M5.Lcd.setTextColor(WHITE);
   M5.Lcd.setCursor(31, 79);
   M5.Lcd.printf("%2d:%2d:%2d\n",RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
   
  //Show Bat instructions
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(31, 95);
  M5.Lcd.printf("Bat");

  //Show vbat
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(31, 117);
  M5.Lcd.printf("%.2f\r\n ", M5.Axp.GetBatVoltage());
  M5.Lcd.setSwapBytes(true);
}

// B button -> poweroff
void btnBClick()
{
  M5.Axp.PowerOff();
}

// A button -> update calibration
void btnAClick()
{
  updateCalibration(pressure);
}

void setLED(bool isON)
{
  digitalWrite (LED_PIN, !isON); // set the LED
}

unsigned int rainbow(int value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to red = blue
  //int value = random (128);
  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

void progressBar(int p, bool ledon)
{
  // update LED
  setLED(ledon);

  if (p > 20) {
      // DMS check maximum closing block
      // M5.Axp.ScreenBreath(10);
      M5.Axp.ScreenBreath(11);
   
  } else {
      // DMS check maximum closing block
      //M5.Axp.ScreenBreath(8);
      M5.Axp.ScreenBreath(9);
  }

  int value = map(p, 0, PRESSURE_MAX, PROGRESSBAR_MIN, PROGRESSBAR_MAX);
    
  for (int i = 0; i <= 128; i++) {  //draw bar
    M5.Lcd.fillRect(8, 142-i, 15, 1, (i <= value) ? rainbow(i) : BLACK);
  }
}
#endif // M5STICKC

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

void updateServoProgressive(int pressure) {
  
  if (pressure > setpoint) {
      servo_angle = map(setpoint, 0, PRESSURE_MAX, SERVO_MIN, SERVO_MAX);  //limite impostabile
  } else {
      servo_angle = map(pressure, 0, PRESSURE_MAX, SERVO_MIN, SERVO_MAX);
  }

  servo.write(servo_angle);

#ifdef DEBUG
  Serial.println(pressure);

  // print count when there is a low/high transition (pressure above threshold)
  if (state==HIGH && lastState==LOW){
      count++;
      delay (10);
      Serial.println(count);
  }
  lastState=state;
#endif
}

// state update function
void updateState(int pressure) {
  
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

  // loop delay
  _DEBUG("Sleeping %d ms",LOOP_DELAY);
  delay(LOOP_DELAY);
}

void setup() {
  //Initialize Serial
  Serial.begin(115200);       
  // Inizializzazione servo 
  servo.attach(SERVO_PIN);

#ifdef M5STICKC
  M5.begin();
  EEPROM.begin(EEPROM_SIZE);  //Initialize EEPROM
  pinMode(LED_PIN, OUTPUT);    //Set up LED
  digitalWrite (LED_PIN, HIGH); // turn off the LED

  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(TFT_BLACK);
  btnA.attachClick(btnAClick);  //BtnA handle
  btnA.setDebounceTicks(40);
  btnB.attachClick(btnBClick);  //BtnB handle
  btnB.setDebounceTicks(25);
  
  M5.Lcd.drawRect(6, 6, 21, 136, 0x7bef);  // draw frame for progressbar
  
  readCalibration(); // read calibration from EEPROM

  // set time
  RTC_TimeStruct.Hours = 0;
  RTC_TimeStruct.Minutes = 0;
  RTC_TimeStruct.Seconds = 0;
  M5.Rtc.SetTime(&RTC_TimeStruct); 
#endif // M5STICKC
#ifdef SIMULATED
  _DEBUG("Init: sleep 5 seconds");
  delay(5000);
#endif
}

void loop () {
#ifdef M5STICKC
  updateUI();
  
  //poll for button press
  btnA.tick();
  btnB.tick();
#endif

  pressure = getPressure();
  _DEBUG("Detected Pressure[%d]",pressure);

  // update progress bar and led state
#ifdef M5STICKC
  progressBar(pressure, pressure > setpoint);  
#endif

  if (mode == MODE_PROGRESSIVE)
      updateServoProgressive(pressure);
  else
      updateState(pressure);
}  
