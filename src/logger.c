#include "../include/logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

// Enable disable logging
int debug_enabled = 0;

// Logs a message to the console if debugging is enabled, including a timestamp
// and using printf-style formatting.
void logger(const char *format, ...) {
  if (debug_enabled) {
    // get timestamp
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    printf("[%s] ", timestamp);

    // format message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // add newline character at the end
    printf("\n");
  }
}
