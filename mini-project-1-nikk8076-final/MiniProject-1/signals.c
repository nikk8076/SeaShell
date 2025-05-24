#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "signals.h"
#define RED "\033[1;31m"
#define RESET "\x1b[0m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"

void ping(int pid,int signal)
{
    char path[256];
    snprintf(path,sizeof(path),"/proc/%d/status",pid);
    FILE *fp = fopen(path,"r");
    if(!fp)
    {
        fprintf(stderr,RED "No such process\n" RESET);
        return;
    }
    fclose(fp);
    kill(pid,signal);
}

void kill_all_children(int pid)
{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/task/%d/children", pid, pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, RED "Error opening file for parent PID %d\n" RESET,pid);
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file);
    fclose(file);
    int child_pids[256];
    int num_children = 0;
    char *token = strtok(line, " ");
    while (token != NULL) {
        child_pids[num_children++] = atoi(token);
        token = strtok(NULL, " ");
    }
    for(int i=0;i<num_children;i++)
        kill(child_pids[i],SIGKILL);
}