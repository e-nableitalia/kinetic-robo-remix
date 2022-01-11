#include <Servo.h>

#include "HandState.h"

// current servo angle
int servo_angle = 0;
int state = LOW;
int lastState = LOW;
// for progressive
extern int setpoint;
// debug
int count = 0;

Servo servo;

void handInit()
{
  // Inizializzazione servo 
  servo.attach(SERVO_PIN);
}

void updateServoProgressive(int pressure)
 {
  
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

HandState& HandStateHoldOpening::instance()
{
    static HandStateHoldOpening    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

HandState *HandStateHoldOpening::enter() {
    m_interval.start(INTERVAL_OPEN);
    return this;
}

HandState *HandStateHoldOpening::update(int pressure) {
    if (pressure > PRESSURE_MIN) {
        int delta = m_interval.delta(INTERVAL_OPEN);
        _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check hold interval[%d] expired[%d]",pressure, PRESSURE_MIN, delta, INTERVAL_OPEN);
        if (delta > HOLDTIME_INTERVAL_uS) {
            _DEBUG("OPEN hold interval expired, start closing hand");
            return HandStateClosing::instance().enter();
            //hand_state = CLOSING;
        }
    } else {
        // discard small muscle contraction and return idle
        _DEBUG("Muscle contraction discarded, inteval too small");
        return HandStateIdleOpen::instance().enter();
    }
}

HandState& HandStateIdleOpen::instance()
{
    static HandStateIdleOpen    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

HandState *HandStateIdleOpen::update(int pressure) {
    if (pressure > PRESSURE_MIN) {
            _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], start hold open timer",pressure, PRESSURE_MIN);
            return HandStateHoldOpening::instance().enter();
        } else
            return this;
}

HandState& HandStateClosed::instance()
{
    static HandStateClosed    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

HandState *HandStateClosed::update(int pressure) {
    if (pressure < PRESSURE_MIN) {
        _DEBUG("Check Pressure[%d] < PRESSURE_MIN[%d] to go in IDLE_CLOSE",pressure, PRESSURE_MIN);
        _DEBUG("Going in IDLE_CLOSED state");
         return HandStateIdleClosed::instance().enter();
    } else
        return this;
}

HandState& HandStateClosing::instance()
{
    static HandStateClosing    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

HandState *HandStateClosing::update(int pressure) {
         if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can continue to close hand",pressure, PRESSURE_MIN);
              if (servo_angle < SERVO_MAX) {
                  int magnitude = (pressure / 50) + 1;
                  servo_angle = servo_angle + magnitude;
                  _DEBUG("Current servo angle[%d], magnitude[%d]",servo_angle, magnitude);
                  servo.write(servo_angle);
                  return this;
              } else {
                  _DEBUG("Current servo angle[%d], hand completely closed",servo_angle);
                  return HandStateClosed::instance().enter();
              }
          } else {
              _DEBUG("Pressure[%d] < PRESSURE_MIN[%d], stop closing hand",pressure, PRESSURE_MIN);
              return HandStateClosed::instance().enter();
          }
}

HandState& HandStateHoldClosing::instance()
{
    static HandStateHoldClosing    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

HandState *HandStateHoldClosing::enter() {
    m_interval.start(INTERVAL_CLOSE);
    return this;
}

HandState *HandStateHoldClosing::update(int pressure) {
          if (pressure > PRESSURE_MIN) {
              _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], check can close hand",pressure, PRESSURE_MIN);
              if (m_interval.delta(INTERVAL_CLOSE) > HOLDTIME_INTERVAL_uS) {
                  _DEBUG("CLOSE hold interval expired, closing hand");
                  return HandStateOpening::instance().enter();
              } else
                return this;
          } else {
              // discard small muscle contraction and return idle
              _DEBUG("Muscle contraction discarded, inteval too small");
              return HandStateIdleClosed::instance().enter();
          }          
}

HandState& HandStateOpening::instance()
{
    static HandStateOpening    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}
HandState *HandStateOpening::update(int pressure) {
    _DEBUG("Opening hand, servo hangle -> [%d]",SERVO_MIN);
    servo.write(SERVO_MIN);
    servo_angle = SERVO_MIN;
    return HandStateOpen::instance().enter();
}

HandState& HandStateOpen::instance()
{
    static HandStateOpen    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}
HandState *HandStateOpen::update(int pressure) {
    _DEBUG("Check Pressure[%d] < PRESSURE_MIN[%d] to go in IDLE_OPEN",pressure, PRESSURE_MIN);
    if (pressure < PRESSURE_MIN) {
        _DEBUG("Going in IDLE_OPEN state");
        return HandStateIdleOpen::instance().enter();
    } else
        return this;
}

HandState& HandStateIdleClosed::instance()
{
    static HandStateIdleClosed    instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}
HandState *HandStateIdleClosed::update(int pressure) {
    if (pressure > PRESSURE_MIN) {
        _DEBUG("Pressure[%d] > PRESSURE_MIN[%d], start hold close timer",pressure, PRESSURE_MIN);
        m_interval.start(INTERVAL_CLOSE);
        return HandStateHoldClosing::instance().enter();
    } else
        return this;
}