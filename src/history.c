#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_PATH "history.log"

static const char *op_to_str(FileOp op) {
    switch (op) {
        case UPLOAD:  return "fileadd";
        case MODIFY:  return "filemodify";
        case DELETE_: return "filedel";
        case MOVE:    return "filelocation";
        case RENAME:  return "filename";
        default:      return "unknown";
    }
}

void log_event(FileOp op, const char *prev, const char *aft) {
    FILE *fp = fopen(HISTORY_PATH, "ab");
    if (!fp) return;

    Log entry;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    entry.year  = t->tm_year + 1900;
    entry.month = t->tm_mon + 1;
    entry.day   = t->tm_mday;
    entry.hour  = t->tm_hour;
    entry.min   = t->tm_min;
    entry.sec   = t->tm_sec;

    strncpy(entry.type, op_to_str(op), sizeof(entry.type) - 1);
    entry.type[sizeof(entry.type) - 1] = '\0';
    strncpy(entry.prev, prev ? prev : "", sizeof(entry.prev) - 1);
    entry.prev[sizeof(entry.prev) - 1] = '\0';
    strncpy(entry.aft,  aft  ? aft  : "", sizeof(entry.aft)  - 1);
    entry.aft[sizeof(entry.aft) - 1] = '\0';

    fwrite(&entry, sizeof(Log), 1, fp);
    fclose(fp);
}

int load_logs(Log **out, int *count) {
    FILE *fp = fopen(HISTORY_PATH, "rb");
    if (!fp) { *out = NULL; *count = 0; return -1; }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    *count = (int)(size / sizeof(Log));
    *out = malloc(size);
    if (!*out) { fclose(fp); return -1; }

    fread(*out, sizeof(Log), *count, fp);
    fclose(fp);
    return 0;
}
