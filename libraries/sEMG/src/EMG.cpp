#include <math.h>
#include <EMG.h>
#include <Telemetry.h>

#define SIMULATION_TIMER    0

sEMG::sEMG() 
{ 
    // empty
    sRate = clock_divider = pin = baseline = offset = clock = 0;

    simulated = false;

    buffer = nullptr;

    // filter disabled by default
    filter.disable();
}

void sEMG::init(int sampleRate, int clockRate, int p, bool publish) {
    
    sRate = sampleRate;

    clock_divider = clockRate / sampleRate;

    pin = p;
    
    C_DEBUG("enabling SEMG(freq=%d Hz, clock=%d Hz, PIN=A%d, publish=%s)",
        sampleRate,
        clockRate,
        p,
        BOOL_STR(publish));

#if defined (STM32F2XX)	// Photon
    name = String::format("sEMG%d",p);
#else
	name = String("sMEG") + String(p);
#endif

    C_DEBUG("Configuring timer, clock divider[%d], sample rate[%d]", clock_divider, sampleRate);

    baseline = offset = 0;

    // filter disabled by default
    filter.disable();

    simulated = false;
}

bool sEMG::enableFilter(EMG_FILTER f, bool value) {
    
    switch (f) {
        case NOTCH_FILTER:
            notch_filter = value;
            break;
        case HIGHPASS_FILTER:
            highpass_filter = value;
            break;
        case LOWPASS_FILTER:
            lowpass_filter = value;
            break;
    }
   
    C_DEBUG("Pin[%d], setting filters, notch[%s], lowpass[%s], highpass[%s]",
        pin,
        BOOL_STR(notch_filter),
        BOOL_STR(highpass_filter),
        BOOL_STR(lowpass_filter));
    filter.init((SAMPLE_FREQUENCY)sRate, NOTCH_FREQ_50HZ, notch_filter, lowpass_filter, highpass_filter);

#if defined (STM32F2XX)	// Photon
    filter_state = String::format("Filters notch[%s], lowpass[%s], highpass[%s]",
        BOOL_STR(notch_filter),
        BOOL_STR(lowpass_filter),
        BOOL_STR(highpass_filter));
#else
	filter_state = String("Filters notch[") + String(BOOL_STR(notch_filter)) + String("], lowpass[") + String(BOOL_STR(lowpass_filter)) +String("], highpass[") + String(BOOL_STR(highpass_filter)) + String("s]");
#endif


    return filter.isEnabled();
}

bool sEMG::filtersEnabled() {
    C_DEBUG("Filters enabled[%s]",BOOL_STR(filter.isEnabled()));
    return filter.isEnabled();
}

void sEMG::disableFilters() {
    C_DEBUG("Disabling filters");
    filter.disable();
    filter_state = "Filters disabled";
}

void sEMG::simulate() {
    C_DEBUG("Simulating EMG signal");
    dt.start(SIMULATION_TIMER);
    simulated = true;
}

void sEMG::enableEnvelope(int value) {
    C_DEBUG("Enabling envelope, baseline value[%d], clearing offset", value);
    baseline = value;
    offset = 0;
#if defined (STM32F2XX)	// Photon	
    mode = String::format("Envelope, baseline[%d]", value);
#else
	mode = String("Envelope, baseline[") + String(value) + String("]");
#endif
}

void sEMG::setOffset(int value) {
    C_DEBUG("Enabling offset, offset value[%d], clearing baseline", value);
    baseline = 0;
    offset = value;
#if defined (STM32F2XX)	// Photon	
    mode = String::format("Envelope, offset[%d]", value);
#else
	mode = String("Envelope, offset[") + String(value) + String("]");
#endif
}

int sEMG::read() {
    semg = analogRead(pin);
    
    C_DEBUG("EMG[%d]", semg);

    return semg;
}

void sEMG::poll() {
//    clock++;
//    if (clock < clock_divider)
//        return;

    clock = 0;

    int data = (simulated ? dt.delta(SIMULATION_TIMER) : analogRead(pin));

    return;

    if (baseline) {
        data -= baseline;
        // normalize as positive
        if (data < 0) data = -data;
        //C_DEBUG("Pin[%d], baseline normalized Value[%d]", pin, data);
    } else if (offset) {
        data -= offset;
        //C_DEBUG("Pin[%d], offset normalized Value[%d]", pin, data);
    }

    if (filter.isEnabled()) {
        // update data
        data = filter.update(data);
        //C_DEBUG("Pin[%d], filtered Value[%d]", pin, data);
    }

    semg = data;

    if (buffer) {
        buffer->produce(semg);
        //C_DEBUG("Pin[%d] >> sEMG[%d]", pin, semg);
    } else {
        //C_DEBUG("Pin[%d] -> sEMG[%d]", pin, semg);
    }
}

