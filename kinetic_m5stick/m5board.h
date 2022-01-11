// m5board.h
// Class abstracting hand states
// Author: A. Navatta

#ifndef M5_BOARD_H

#define M5_BOARD_H

#include <AXP192.h>
#include <IMU.h>
#include <M5Display.h>
#include <M5StickC.h>
#include <RTC.h>

#include <EEPROM.h>

// PINS
#define SERVO_PIN   G26
#define LED_PIN     G10
#define SENSOR_PIN  G36
#define BUTTONA_PIN G39
#define BUTTONB_PIN G37

#define EEPROM_SIZE       1

#define PROGRESSBAR_MIN   0
#define PROGRESSBAR_MAX   127

// DMS 
// progress bar Y offset
//#define Y_OFFSET(x) (15+(127-x))
#define Y_OFFSET(x) (15+(254-x))

void M5init();
void M5loop();
void updateCalibration(int value);
void readCalibration();
void updateUI();
void btnBClick();
void btnAClick();
void setLED(bool isON);
unsigned int rainbow(int value);
void progressBar(int p, bool ledon);

#endif // M5_BOARD_H