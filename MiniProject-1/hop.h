#ifndef HOP_H
#define HOP_H
#include "details.h"
#define MAX_PATH 256

void upd();

void handle_hop(char* command,Details* details);

void hop(const char *path,Details* details);

#endif  