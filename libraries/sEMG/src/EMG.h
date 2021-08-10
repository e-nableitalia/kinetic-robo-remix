//
// sEMG: EMG Signal Acquisition
//
// Author: A.Navatta

#ifndef SEMG_H

#define SEMG_H

#if defined (STM32F2XX)
#include <application.h>
#endif

#include <CircularBuffer.h>
#include <EMGFilter.h>
#include <DeltaTime.h>

enum EMG_FILTER { NOTCH_FILTER = 1, HIGHPASS_FILTER = 2, LOWPASS_FILTER = 3 };

// Single EMG channel
class sEMG {
public:
    sEMG();
    
    // sampleRate   - samplingRate, could be 500Hz or 1000Hz
    // clockRate    - external clock rate
    // depth        - circular buffer depth, each sample = 2 bytes
    // p            - PIN to read signal
    // publish      - publish signal in cloud as "sEMG" value
    void init(int sampleRate = SAMPLE_FREQ_1000HZ, int clock_rate = SAMPLE_FREQ_1000HZ, int p = A0, bool publish = false);

    // filter is enabled if clock divider is not zero
    bool enabled() { return clock_divider != 0; };

    void disable() { clock_divider = 0; };

    void setBuffer(CircularBuffer *b) { buffer = b; };

    // filters functions
    bool enableFilter(EMG_FILTER filter, bool value);
    bool filtersEnabled();
    void disableFilters();

    // enable envelope calculation
    void enableEnvelope(int baseline);

    // enable signal simulation
    void simulate();

    // set signal offset
    // note: envelope disables offset
    void setOffset(int offset);

    // read latest value
    int read();

    // called by timer function
    void poll();

private:
    // clock divider
    int clock_divider;
    int clock;
    // name
    String name;
    // current value
    int  semg;
    // pin to read
    int  pin;
    // baseline for envelope
    int baseline;
    // signal offset
    int  offset;

    // internal circular buffer
    CircularBuffer *buffer;
    
    int  sRate;
    
    // EMG filter
    EMGFilters filter;
    bool notch_filter;
    bool lowpass_filter;
    bool highpass_filter;

    // for signal simulation
    bool simulated;
    DeltaTime dt;

    // flag to publish semg value
    bool publish;
    String filter_state;
    String mode;
};

#endif