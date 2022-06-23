// Provide global and board support configs
#ifndef GLOBAL_CONFIG_H

#define GLOBAL_CONFIG_H

#define PRESSURE_MAX      600 // could be 1200
#define PRESSURE_MIN      10  // min pressure value to trigger hand closure/opening

// Conditional M5StickC board support
#ifdef ARDUINO_M5Stick_C
#pragma message "Using ESP32 M5StickC board configuration"
#include "m5board.h"
#endif // ARDUINO_M5Stick_C

// Conditional Arduino Nano 33 BLE board support
#ifdef ARDUINO_ARDUINO_NANO33BLE
#pragma message "Using Arduino Nano 33 BLE board configuration"
#include "nano33bleboard.h"
#endif // ARDUINO_ARDUINO_NANO33BLE

#endif // GLOBAL_CONFIG_H