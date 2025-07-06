#ifndef PROCLORE_H
#define PROCLORE_H
#include <stdlib.h>
int is_process_exists(int pid);

char get_process_status(int pid, int* is_foreground);

unsigned long get_virtual_memory_size(int pid);

void get_executable_path(int pid, char* exe_path, size_t size);

void proclore(int pid);

#endif