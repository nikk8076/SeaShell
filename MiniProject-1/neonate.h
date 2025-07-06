#ifndef NEONATE_H
#define NEONATE_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
    int pid;
    char cmdline[256];
    char state;
} ProcessInfo;

int run_neonate(int time_arg);

#endif