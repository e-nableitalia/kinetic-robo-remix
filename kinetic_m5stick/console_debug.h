//
// Console debug function
//
// Author: A.Navatta

#ifndef CONSOLE_DEBUG_H

#define CONSOLE_DEBUG_H

// debug enabled
#define DEBUG
#define FORMAT_BUFFERSIZE 1024

#ifdef DEBUG
void console_debug(const char *function, const char *format_str, ...);
#define _DEBUG(...)   console_debug(__func__, __VA_ARGS__)
#else
#define _DEBUG(...)
#endif

#endif // CONSOLE_DEBUG_H