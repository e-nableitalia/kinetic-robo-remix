#ifdef ARDUINO_M5Stick_C

#include <OneButton.h>
#include "global_config.h"
#include "m5board.h"

extern int setpoint;  // deault, updated by EEPROM
extern int count;
extern int pressure;

RTC_TimeTypeDef RTC_TimeStruct;

// global objects
OneButton btnA(BUTTONA_PIN, true);
OneButton btnB(BUTTONB_PIN, true);

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

void M5init() {
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
}

void M5loop() {
  updateUI();
  
  //poll for button press
  btnA.tick();
  btnB.tick();
}

#endif // ARDUINO_M5Stick_C