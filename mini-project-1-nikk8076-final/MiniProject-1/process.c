#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>
#include <pwd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include "details.h"
#include "process.h"

BgProcess bg_processes[MAX_BG_PROCESSES];
int bg_count = 0;
int next_bg_index = 1;

void sigchld_handler(int signo) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < bg_count; i++) {
            if (bg_processes[i].pid == pid) {
                if (WIFEXITED(status)) {
                    printf("\n[%d] Sleep exited normally (%d)\n", bg_processes[i].index, pid);
                } else if (WIFSIGNALED(status)) {
                    printf("\n[%d] Sleep terminated by signal (%d)\n", bg_processes[i].index, pid);
                }
                for (int j = i; j < bg_count - 1; j++) {
                    bg_processes[j] = bg_processes[j + 1];
                }
                bg_count--;
                break;
            }
        }
    }
}

void parse_command(char* command, char* args[]) {
    int i = 0;
    char* token = strtok(command, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

void execute_command(char* command, int bg_no, int* elapsed_time, char** last_command) {
    pid_t pid = fork();
    time_t start, end;
    double elapsed;
    if (pid == 0) {
        char* args[MAX_ARGS];
        parse_command(command, args);
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        time(&start);
        if (bg_no) {
            if (bg_count < MAX_BG_PROCESSES) {
                bg_processes[bg_count].index = next_bg_index;
                bg_processes[bg_count].pid = pid;
                printf("[%d] %d\n", next_bg_index, pid);
                bg_count++;
                next_bg_index++;
            } else {
                fprintf(stderr, "Maximum number of background processes reached\n");
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
            time(&end);
            elapsed = difftime(end, start);
            if (elapsed > 2) {
                *elapsed_time = (int)elapsed;
                *last_command = strdup(command);
            } else {
                *elapsed_time = 0;
            }
        }
    } else {
        perror("fork failed");
    }
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

int count_ampersands(const char* input) {
    int count = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '&') {
            count++;
        }
    }
    return count;
}