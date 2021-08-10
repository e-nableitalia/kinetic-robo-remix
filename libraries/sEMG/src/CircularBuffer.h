//
// CircularBuffer: Circular buffer for real time data buffering
//
// Author: A.Navatta

#ifndef CIRCULAR_BUFFER_H

#define CIRCULAR_BUFFER_H

//#include <string.h>
//#include <stdlib.h>
//#include <stdarg.h>
#include <stdint.h>

#define MAX_BUFFER_SIZE 1000

class CircularBuffer {
public:
    CircularBuffer();
    
    void produce(int value);
    unsigned short consume();
    int consume(uint8_t *buffer, int size);
    
    void dump();

    int avail() { return _avail; }
    void size(int s);
    inline int size() { return sz; };
    inline int data_size() { return sizeof(unsigned short); };

private:

    int head;
    int tail;
    bool overflow;
    int sz;
    int _avail;
    unsigned short buffer[MAX_BUFFER_SIZE];
};

#endif