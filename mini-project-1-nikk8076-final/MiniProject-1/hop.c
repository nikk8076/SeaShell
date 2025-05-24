#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include "hop.h"
#include "details.h"
#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"
#define MAX_PATH 256
#define MAX_INPUT_LEN 4096
static char initial_dir[MAX_PATH];
static char *current_dir_ptr = NULL;
static char *prev_dir_ptr = NULL;

void upd() {
    if (current_dir_ptr) free(current_dir_ptr);
    if (prev_dir_ptr) free(prev_dir_ptr);
    current_dir_ptr = (char *)malloc(MAX_PATH);
    prev_dir_ptr = (char *)malloc(MAX_PATH);
    if (!current_dir_ptr || !prev_dir_ptr) {
        perror("malloc() error");
        exit(EXIT_FAILURE);
    }
    if (getcwd(current_dir_ptr, MAX_PATH) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }
    strncpy(prev_dir_ptr, current_dir_ptr, MAX_PATH);
    if (strlen(initial_dir) == 0) {
        strncpy(initial_dir, current_dir_ptr, MAX_PATH);
    }
}

void handle_hop(char* command, Details* details) {
    size_t len = strlen(command);
    while (len > 0 && isspace((unsigned char)command[len - 1])) {
        len--;
    }
    command[len] = '\0';
    if (strcmp(command, "hop") == 0) {    
        hop("~",details);
        return;
    }
    char* directories_str = strdup(command);
    if (!directories_str) {
        perror("strdup() error");
        exit(EXIT_FAILURE);
    }
    char* token;
    char* delim = " ";
    int dir_count = 0;
    token = strtok(directories_str, delim);
    while (token != NULL) {
        dir_count++;
        token = strtok(NULL, delim);
    }
    char** directories = (char**)malloc(sizeof(char*) * dir_count);
    if (!directories) {
        perror("malloc() error");
        free(directories_str);
        exit(EXIT_FAILURE);
    }
    directories_str = strdup(command); 
    token = strtok(directories_str, delim);
    for (int i = 0; i < dir_count; i++) {
        if (!token) break;
        directories[i] = strdup(token);
        if (!directories[i]) {
            perror("strdup() error");
            for (int j = 0; j < i; j++) free(directories[j]);
            free(directories);
            free(directories_str);
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, delim);
    }
    for (int i = 1; i < dir_count; i++) {
        hop(directories[i], details);
        free(directories[i]);
    }
    free(directories);
    free(directories_str);
}

void hop(const char *path, Details* details) {
    char* temp = strdup(prev_dir_ptr);
    if (!temp) {
        perror("strdup() error");
        exit(EXIT_FAILURE);
    }
    char new_dir[MAX_PATH];
    struct stat statbuf;
    if (strncmp(path, "~/", 2) == 0) 
        snprintf(new_dir, sizeof(new_dir), "%s%s", initial_dir, path + 1);
    else if (strcmp(path, "~") == 0) 
        strncpy(new_dir, initial_dir, MAX_PATH);
    else if (strcmp(path, "-") == 0) {
        if (prev_dir_ptr) {
            strncpy(new_dir, temp, MAX_PATH);
        } else {
            fprintf(stderr, RED "Error: previous directory not available\n" RESET);
            free(temp);
            return;
        }
    } else if (strcmp(path, "..") == 0) {
        if (realpath(current_dir_ptr, new_dir) == NULL) {
            perror("realpath() error");
            free(temp);
            return;
        }
        char *last_slash = strrchr(new_dir, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        }
    } else {
        if (path[0] == '/') {
            snprintf(new_dir, sizeof(new_dir), "%s", path);
        } else {
            snprintf(new_dir, sizeof(new_dir), "%s/%s", current_dir_ptr, path);
        }
    }
    if (chdir(new_dir) != 0) {
        fprintf(stderr, RED "Error:%s is not a directory\n" RESET, new_dir);
        free(temp);
        return;
    }
    if (getcwd(new_dir, sizeof(new_dir)) == NULL) {
        perror("getcwd() error");
        free(temp);
        return;
    }
    // printf("%s\n",new_dir);
    fprintf(stdout, BLUE "%s\n" RESET, new_dir);
    if (lstat(new_dir, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        if(prev_dir_ptr)
            free(prev_dir_ptr);
        prev_dir_ptr = strdup(current_dir_ptr);
        details->previousDirectory=strdup(prev_dir_ptr);
        if (!prev_dir_ptr) {
            perror("strdup() error");
            exit(EXIT_FAILURE);
        }
        if (current_dir_ptr) free(current_dir_ptr);
        current_dir_ptr = strdup(new_dir);
        if (!current_dir_ptr) {
            perror("strdup() error");
            free(temp);
            exit(EXIT_FAILURE);
        }
        char rel_path[MAX_PATH];
        if (strncmp(new_dir, initial_dir, strlen(initial_dir)) == 0) {
            snprintf(rel_path, sizeof(rel_path), "~%s", new_dir + strlen(initial_dir));
        } else {
            realpath(new_dir, rel_path);
        }
        details->currentDirectory = strdup(rel_path);
        if (!details->currentDirectory) {
            perror("strdup() error");
            free(temp);
            exit(EXIT_FAILURE);
        }
        // printf("%s\n", details->currentDirectory); //relative path
    } else {
        fprintf(stderr, RED "Error:%s is not a directory\n" RESET, new_dir);
    }
    free(temp);
}