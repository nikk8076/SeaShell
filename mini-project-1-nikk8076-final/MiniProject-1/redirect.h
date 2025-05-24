#ifndef REDIRECT_H
#define REDIRECT_H
#include "commands.h"
#include "details.h"
#include "log.h"
char* remove_extra_space(char* s);

void redirect(char* s,Details* details, Queue* queue, char** last_command, int* elapsed_time,char* homedir);

#endif