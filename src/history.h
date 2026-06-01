#ifndef HISTORY_H
#define HISTORY_H

#include "custom_types.h"

void log_event(FileOp op, const char *prev, const char *aft);
int  load_logs(Log **out, int *count);

#endif
