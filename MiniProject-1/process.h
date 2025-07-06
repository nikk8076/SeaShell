#ifndef PROCESS_H
#define PROCESS_H
#define MAX_BG_PROCESSES 100
#define MAX_ARGS 64

void sigchld_handler(int signo);

void setup_signal_handler();

void parse_command(char* command, char* args[]);

void execute_command(char* command, int bg_no, int* elapsed_time, char** last_command);

int count_ampersands(const char* input);

#endif