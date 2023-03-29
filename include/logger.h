#ifndef LOG_H
#define LOG_H

// This function is used for logging debug information, and it will print the
// formatted string to standard output if the debug_enabled variable is set to a
// non-zero value.
extern int debug_enabled;
void logger(const char *format, ...);

#endif
