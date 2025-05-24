#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "redirect.h"
char* remove_extra_space(char* str)
{
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

void redirect(char* s,Details* details, Queue* queue, char** last_command, int* elapsed_time,char* homedir)
{
    char* input = strdup(s);
    input = remove_extra_space(input);
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);
    int input_fd = -1;
    int output_fd = -1;
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { 
        if (strstr(input, ">>")) 
        { 
            char* ptr = strstr(input, ">>");
            char* filename = strdup(ptr + 3);
            *(ptr - 1) = '\0';
            input = remove_extra_space(input);
            output_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd < 0) {
                perror("Error opening file for append");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
        } 
        else if (strstr(input, ">")) 
        { 
            char* ptr = strstr(input, ">");
            char* filename = strdup(ptr + 2);
            *(ptr - 1) = '\0';
            input = remove_extra_space(input);
            output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                perror("Error opening file for overwrite");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
        }
        if (strstr(input, "<")) 
        {
            char* ptr = strstr(input, "<");
            char* filename = strdup(ptr + 2);
            *(ptr - 1) = '\0';
            input = remove_extra_space(input);
            input_fd = open(filename, O_RDONLY);
            if (input_fd < 0) {
                perror("No such input file found!");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
        }
        tokenize_and_trim(input,details,queue,last_command,elapsed_time,homedir);
        exit(0);
    } 
    else 
    {
        wait(NULL);
        dup2(orig_stdin, STDIN_FILENO);
        dup2(orig_stdout, STDOUT_FILENO);
        if (input_fd >= 0) close(input_fd);
        if (output_fd >= 0) close(output_fd);
        close(orig_stdin);
        close(orig_stdout);
    }
}
