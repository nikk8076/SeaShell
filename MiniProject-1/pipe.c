#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include "pipe.h"
#define MAX_COMMANDS 10
#define MAX_COMMAND_LENGTH 100

void tokenize_commands(char *input, char *commands[], int *num_commands) {
    char *token = strtok(input, "|");
    *num_commands = 0;
    while (token != NULL && *num_commands < MAX_COMMANDS) {
        commands[*num_commands] = token;
        (*num_commands)++;
        token = strtok(NULL, "|");
    }
}

char* trim_str(const char* str) {
    if(!str) return NULL;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) 
        return strdup("");
    const char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end++;
    size_t len = end - str;
    char* result = malloc(len + 1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }
    return result;
}

void execute_cmd(char *command, Details* details, Queue* queue, char** last_command, int* elapsed_time, char* homedir) {
    tokenize_and_trim(command, details, queue, last_command, elapsed_time, homedir);
    exit(0);
}

void execute_pipe(char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time, char* homedir) {
    char *commands[MAX_COMMANDS];
    int num_commands;
    int pipes[MAX_COMMANDS - 1][2];
    char *input_copy = strdup(input);
    tokenize_commands(input_copy, commands, &num_commands);
    for (int i = 0; i < num_commands; i++) {
        commands[i] = trim_str(commands[i]);
        if (strlen(commands[i]) == 0) {
            fprintf(stderr, "Error: Invalid use of pipe\n");
            free(input_copy);
            return;
        }
    }
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            free(input_copy);
            return;
        }
    }
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == 0) { 
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            }
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            }
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            execute_cmd(commands[i], details, queue, last_command, elapsed_time, homedir);
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            free(input_copy);
            return;
        }
    }
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
    free(input_copy);
}
