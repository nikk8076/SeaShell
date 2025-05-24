#ifndef COMMANDS_H
#define COMMANDS_H
#include "details.h"
#include "hop.h"
#include "log.h"
#include "reveal.h"
#include "redirect.h"
#include "fg.h"
#include "activities.h"
#include "neonate.h"
#include "iMan.h"
#include "bg.h"
#include "pipe.h"
#include "signals.h"
#include "nano.h"
#include "proclore.h"
#include "seek.h"
#include "process.h"

void handle_command(char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* home);

void tokenize_and_trim(const char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* home);

char* trim(const char* str);

void handle_reveal(char *input, Details *details,char* homedir);

#endif