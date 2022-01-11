//
// DeltaTime: Simple class to precisely measure intervals
//
// Author: A.Navatta

#ifndef DELTA_TIME_H

#define DELTA_TIME_H

#if defined (STM32F2XX)	// Photon
#include <particle.h>
#else
#include <Arduino.h>
#endif

#define MAX_TIMERS      5

class DeltaTime {
public:
    DeltaTime() {
        for (int i=0; i< MAX_TIMERS; i++)
            last_ticks[i] = 0;
#if defined (STM32F2XX)	// Photon     
        ticks_per_micro = System.ticksPerMicrosecond();
#else
		ticks_per_micro = 1;
#endif
    }
    void start(int i) {
#if defined (STM32F2XX)	// Photon
		last_ticks[i] = System.ticks();
#else
		last_ticks[i] = micros();
#endif
    }

    uint32_t delta(int i) {
#if defined (STM32F2XX)	// Photon		
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

#endif // DELTA_TIME_H