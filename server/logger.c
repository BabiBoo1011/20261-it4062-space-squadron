#include "logger.h"
#include <stdio.h>
#include <time.h>

#define LOG_FILE "logs/server.log"

void log_event(const char *tag, const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] [%s] %s\n", ts, tag, msg);
    fclose(f);
}
