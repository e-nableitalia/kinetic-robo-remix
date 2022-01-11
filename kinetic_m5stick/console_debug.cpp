

#include <stdarg.h>
#include <stdio.h>

#include "console_debug.h"

#if defined (STM32F2XX)	// Photon
#include <particle.h>
#else
#include <Arduino.h>
#endif

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

// Serial debug function
// print function name and formatted text
// accepted format arguments:
// %d, %i -> integer
// %s -> string
void console_debug(const char *function, const char *format_str, ...) {
  
    va_list argp;
    va_start(argp, format_str);
    char *bp=format_buffer;
    int bspace = FORMAT_BUFFERSIZE - 1;

    while ((*function) && (bspace)) {
      *bp++ = *function++;
      --bspace;
    }

    *bp++ = ':';
    --bspace;
    *bp++ = ' ';
    --bspace;

    while (*format_str != '\0' && bspace > 0) {
      if (*format_str != '%') {
        *bp++ = *format_str++;
        --bspace;
      } else if (format_str[1] == '%') // An "escaped" '%' (just print one '%').
      {
        *bp++ = *format_str++;    // Store first %
        ++format_str;             // but skip second %
        --bspace;
      } else {
         ++format_str;
        // parse format
        switch (*format_str) {
          case 's': {
            // string
            char *str = va_arg (argp, char *);
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          case 'd': case 'i': {
            // decimal
            char ibuffer[16];
            int val = va_arg (argp, int);
            snprintf(ibuffer,16,"%d",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          default: {
            // skip format
          }
        }
         
        ++format_str;
      }
    }
    // terminate string
    *bp = 0;
    Serial.println(format_buffer);
}