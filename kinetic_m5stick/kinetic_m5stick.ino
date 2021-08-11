
/* M5Stickc Muscle control for prostheses By Praga Michele
 *  
 * Select servo with corresponding define
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
 * ANY CLAIM, DAMAGES,OR OTHER LIABILITY. YOU AGREE TO USE AT YOUR OWN RISK.
 * Modified from Praga Michele 
*/
// Required libs:
// ESP32 Lite Pack
// ServoESP32
// OneButton
#include <Servo.h>
#include <OneButton.h>
#include <M5StickC.h>
#include <EEPROM.h>
//#include <DeltaTime.h>

// global defines
#define DEBUG
#define SERVO_270

#define MODE_PROGRESSIVE  1
#define MODE_HOLDTIME     2
#define LOOP_DELAY        50 // delay main loop of 10 mS, to keep higher pressure sample rate and delay hand opening it is also possible to use a servo increment prescaler (feature to be evaluated)
#define EEPROM_SIZE       1
#define INTERVAL_OPEN     0   // hand open interval id, used to calculate hold time since first muscle contraction -> activate hand/servo closure
#define INTERVAL_CLOSE    1   // hand close interval id, used to calculate hold time since first muscle contraction -> trigger hand open operation
#define HOLDTIME_INTERVAL_uS   (250 * 1000)  // 250 ms, hold time
#define PRESSURE_MAX      600           // could be 1200
#define PRESSURE_MIN      10            // min pressure value to trigger hand closure/opening
#define PROGRESSBAR_MIN   0
#define PROGRESSBAR_MAX   127
#define FORMAT_BUFFERSIZE 1024
// DMS 
// progress bar Y offset
//#define Y_OFFSET(x) (15+(127-x))
#define Y_OFFSET(x) (15+(254-x))

// PINS
#define SERVO_PIN   G26
#define LED_PIN     G10
#define SENSOR_PIN  G36

#ifdef DEBUG
#define _DEBUG(...)   console_debug(__func__, __VA_ARGS__)
#else
#degine _DEBUG(...)
#endif

// for servo 270 degrees maximum lever 6-7mm
#ifdef SERVO_270
#define SERVO_MAX 	254
#define SERVO_MIN	  1
#else // SERVO_270

// servo 180 degrees maximum lever 11-12mm
#ifdef SERVO_180
#define SERVO_MAX 	150
#define SERVO_MIN	  60
#endif // SERVO_180

#endif // SERVO_270

RTC_TimeTypeDef RTC_TimeStruct;

// value of pressure sensor
int pressure = 6;

// possible hand states, default IDLE_OPEN
enum _hand_state {
  IDLE_OPEN = 0,
  HOLD_OPENING,
  OPENING,
  HOLD_CLOSING,
  CLOSING,
  IDLE_CLOSED
} hand_state = IDLE_OPEN;

char *_state[] = {
  "IDLE_OPEN",
  "HOLD_OPENING",
  "OPENING",
  "HOLD_CLOSING",
  "CLOSING",
  "IDLE_CLOSED",
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
OneButton btnA(G39, true);
OneButton btnB(G37, true);
Servo servo;
DeltaTime interval;

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

//float getPercentError(float approx, float exact)
//{
//  return (abs(approx-exact)/exact)*100;
//}

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

int getPressure() {
    // logic
  int raw_pressure = analogRead(SENSOR_PIN);

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
}

// state update function
void updateState(int pressure) {
  
  _DEBUG("Entering state[%s]",_state[hand_state]);
  
  switch (hand_state) {
      case HOLD_OPENING:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check hold interval expired",pressure, PRESSURE_MIN);
              if (interval.delta(INTERVAL_OPEN) > HOLDTIME_INTERVAL_uS) {
                  _DEBUG("OPEN hold interval expired, start opening hand");
                  hand_state = OPENING;
              }
          } else {
              // discard small muscle contraction and return idle
              _DEBUG("Muscle contraction discarded, inteval too small");
              hand_state = IDLE_OPEN;
          }
          break;
      case OPENING:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can continue to open hand",pressure, PRESSURE_MIN);
              if (servo_angle < SERVO_MAX) {
                  servo_angle++;
                  _DEBUG("Current servo andle[%d]",servo_angle);
                  servo.write(servo_angle);
              } else {
                  _DEBUG("Current servo angle[%d], hand completely open",servo_angle);
                  hand_state = IDLE_CLOSED;
              }
          } else {
              _DEBUG("Pressure[%d] < PRESSURE_MIN[%d], stop closing hand",pressure, PRESSURE_MIN);
              hand_state = IDLE_CLOSED;
          }
          break;
       case HOLD_CLOSING:
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can close hand",pressure, PRESSURE_MIN);
              if (interval.delta(INTERVAL_CLOSE) > HOLDTIME_INTERVAL_uS) {
                  _DEBUG("CLOSE hold interval expired, closing hand");
                  hand_state = CLOSING;
              }
          } else {
              // discard small muscle contraction and return idle
              _DEBUG("Muscle contraction discarded, inteval too small");
              hand_state = IDLE_CLOSED;
          }
          break;
      case CLOSING:
          _DEBUG("Closing hand, servo handle -> [%d]",SERVO_MIN);
          servo.write(SERVO_MIN);
          hand_state = IDLE_OPEN;
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
  M5.begin();
  EEPROM.begin(EEPROM_SIZE);  //Initialize EEPROM
  Serial.begin(115200);       //Initialize Serial
  pinMode(LED_PIN, OUTPUT);    //Set up LED
  digitalWrite (LED_PIN, HIGH); // turn off the LED

  // Inizializzazione servo 
  servo.attach(SERVO_PIN);

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
}

void loop () {

  updateUI();
  
  //poll for button press
  btnA.tick();
  btnB.tick();

  pressure = getPressure();
  _DEBUG("Detected Pressure[%d]",pressure);

  // update progress bar and led state
  progressBar(pressure, pressure > setpoint);  

  if (mode == MODE_PROGRESSIVE)
      updateServoProgressive(pressure);
  else
      updateState(pressure);

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
