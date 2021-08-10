//
// Telemetry: Telemetry subsystem
//
// Author: A.Navatta

#ifndef TELEMETRY_H

#define TELEMETRY_H

#if defined (STM32F2XX)
#include <application.h>
#else
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <stdarg.h>
#include <stdio.h>
#endif

#include <BufferProducer.h>
#include <RTPPackets.h>

#define TELEMETRY_MAX_CHANNELS  2

#define SERIAL_DEFAULT_SPEED    115200

#define FORMAT_BUFFERSIZE 1024
 

#define DEBUG_CHANNEL       0
#define SEMG_CHANNEL0       1
#define SEMG_CHANNEL1       2
#define TELEMETRY_DEFAULT_PAYLOAD_SIZE  1000 // 1000 bytes -> 1 pkt every 500ms

// enable API debug
#define _DEBUG              1

#define STR(x)        #x
#define BOOL_STR(x)   (x ? "true" : "false")

#ifdef _DEBUG
#define T_DEBUG(...)  telemetry.vdebug(__func__, __VA_ARGS__)
#define C_DEBUG(...)  telemetry.cdebug(__func__, __VA_ARGS__)

#else
#define T_DEBUG(...)     do {} while (0)
#define C_DEBUG(...)     do {} while (0)
#endif

class Telemetry {
public:
    Telemetry();
    void init(bool debug = false, int speed = SERIAL_DEFAULT_SPEED);
    bool streaming_enable(String remoteip, int port);
    void streaming_disable();
    void console(bool value);
    void enable(int channel, int payload_size = TELEMETRY_DEFAULT_PAYLOAD_SIZE, bool fill = true);
    void disable(int channel);
    void attach(int channel, BufferProducer *producer);
    BufferProducer *detach(int channel);

    void poll();
    void send();

    // local & remote debug functions
    void debug(const char *data);
    void debug(String &s);
    void vdebug(const char *func, const char *format, ...);
    void cdebug(const char *func, const char *format, ...);

private:
#if defined (STM32F2XX) // Photon Board
    void format(const char *func, const char *format_str, va_list argp);
#endif
    void send(unsigned int channel, uint8_t *data, int size);
    void sendBuffer(unsigned int channel, int size);
    uint8_t *getBuffer(int channel);

#if defined (STM32F2XX) // Photon Board
    UDP  udp;
#else // Arduino
	EthernetUDP udp;
#endif
    IPAddress remoteIp;

    int  remotePort;
    
    RTPPacket packet[TELEMETRY_MAX_CHANNELS];
    BufferProducer *producers[TELEMETRY_MAX_CHANNELS];
    bool            channel_enabled[TELEMETRY_MAX_CHANNELS]; 
    int             channel_pktsize[TELEMETRY_MAX_CHANNELS];
    int             channel_pktready[TELEMETRY_MAX_CHANNELS];
    bool            channel_fillmode[TELEMETRY_MAX_CHANNELS];
    bool            _debug;
    bool            _streaming_enable;
    // format buffer
    char            format_buffer[FORMAT_BUFFERSIZE];
};

extern Telemetry telemetry;

#endif

