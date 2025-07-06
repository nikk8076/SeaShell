#ifndef PIPE_H
#define PIPE_H
#include "commands.h"
void tokenize_commands(char *input, char *commands[], int *num_commands);

char* trim_str(const char *str);

void execute_cmd(char *command,Details* details,Queue* queue,char** last_command,int* elapsed,char* home);

void execute_pipe(char* input,Details* details,Queue* queue,char** last_command,int* elapsed_time,char* home);

#endif