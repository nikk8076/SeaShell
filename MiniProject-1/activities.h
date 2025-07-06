#ifndef ACTIVITIES_H
#define ACTIVITIES_H

typedef struct Process{
    char command[256];
    int pid;
    char state[16];
}Process;

void print_children(int pid);

void print_processes(Process *procs, int count);

int get_process_information(int pid, Process *proc);

#endif