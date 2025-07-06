#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include "activities.h"

#define RED "\033[1;31m"
#define RESET "\x1b[0m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"

int get_process_information(int pid, Process *proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, RED "Error opening file for PID %d\n" RESET, pid);
        return 0;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name: %s", proc->command);
        }
        if (strncmp(line, "State:", 6) == 0) {
            char state_char;
            sscanf(line, "State: %c", &state_char);
            switch (state_char) {
                case 'R':
                case 'S':
                case 'D':
                case 'Z':
                    strcpy(proc->state, "Running");
                    break;
                case 'T':
                    strcpy(proc->state, "Stopped");
                    break;
                default:
                    strcpy(proc->state, "Unknown");
                    break;
            }
        }
    }
    fclose(file);
    return 1;
}

void print_processes(Process *procs, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(procs[i].command, procs[j].command) > 0) {
                Process temp = procs[i];
                procs[i] = procs[j];
                procs[j] = temp;
            }
        }
    }
    for (int i = 0; i < count; i++) {
        printf("%d : %s - %s\n", procs[i].pid, procs[i].command, procs[i].state);
    }
    if(!count)
    {
        fprintf(stderr,RED "No child processes\n" RESET);
        return;
    }
}

void print_children(int parent_pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/task/%d/children", parent_pid, parent_pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, RED "Error opening file for parent PID %d\n" RESET, parent_pid);
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
    Process processes[256];
    int proc_count = 0;
    for (int i = 0; i < num_children; i++) {
        Process proc;
        proc.pid = child_pids[i];
        int a=get_process_information(proc.pid, &proc);
        if(a)
        {
            processes[proc_count++] = proc;
        }
    }
    print_processes(processes, proc_count);
}
