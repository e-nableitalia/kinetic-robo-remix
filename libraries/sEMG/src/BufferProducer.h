//
// BufferProducer: BufferProducer interface
//
// Author: A.Navatta

#ifndef BUFFER_PRODUCER_H

#define BUFFER_PRODUCER_H

#include <stdint.h>

class BufferProducer {
    public:
        virtual int consume(uint8_t *buffer, int size, bool fill = true) = 0;
};

#endif