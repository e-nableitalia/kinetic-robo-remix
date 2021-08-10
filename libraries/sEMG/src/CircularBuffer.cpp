
#if defined (STM32F2XX)
#include <particle.h>
#endif

#include <CircularBuffer.h>
// #include <Telemetry.hpp>

CircularBuffer::CircularBuffer() {
}

void CircularBuffer::size(int s) {
    sz = s;
    head = tail = _avail = 0;
    overflow = false;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        buffer[i] = 0;
}

void CircularBuffer::produce(int value) {
    
    if (value < 0) value = 0;

    buffer[head] = value;

    // String _dump = String::format("Buffer::produce value(%d), head(%d)",value,head);
    // telemetry.debug(_dump);

    head++;
    head = head % sz;
    
    _avail++;

    if (_avail > sz) {

        // telemetry.debug("overflow");

        overflow = true;
        // advance tail
        tail++;
        tail = tail % sz;
        _avail = sz;
    }
}

unsigned short CircularBuffer::consume() {
    
    if (!_avail) 
        return -1;
        
    unsigned short value = buffer[tail];
    // String _dump = String::format("Buffer::consume value(%d), tail(%d)",value,tail);
    // telemetry.debug(_dump);

    tail++;

    _avail--;
    
    tail = tail % sz;
    return value;
}

int CircularBuffer::consume(uint8_t *b, int s) {
    if (_avail < s)
        s = _avail;
    
    int pos = 0;

    for (int i=0; i < s; i++) {
        // unsigned short value = buffer[tail];
        // String _dump = String::format("Buffer::consume value(%d), tail(%d), index(%d)",value,tail,i);
        // telemetry.debug(_dump);
        uint8_t *s = (uint8_t *)&buffer[tail++];
	    b[pos++] = s[1];
	    b[pos++] = s[0];
        tail = tail % sz;
    }
    _avail -= s;

    return s;
}

void CircularBuffer::dump() {
    // String _dump = String::format("Buffer: head(%d), tail(%d), avail(%d), overflow(%s)",
    //     head,
    //     tail,
    //     _avail,
    //     (overflow ? "true" : "false"));

    // telemetry.debug(_dump);
}
