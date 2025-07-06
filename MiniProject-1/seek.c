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
#include "seek.h"
#include "details.h"
#include "hop.h"
#include <math.h>

void print_colored(const char* name, int is_dir) {
    if (is_dir) 
        printf("\033[1;34m%s\033[0m\n", name); 
    else 
        printf("\033[1;32m%s\033[0m\n", name);
}

int check_permissions(const char* path, int is_dir) {
    if (is_dir) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, RED_COLOR "Missing permissions for directory %s!\n" RESET_COLOR, path);
            return 0;
        }
    } else {
        if (access(path, R_OK) != 0) {
            fprintf(stderr, RED_COLOR "Missing permissions for file %s!\n" RESET_COLOR, path);
            return 0;
        }
    }
    return 1;
}

void display_txt_file(const char* path) {
    printf("Displaying contents of: %s\n", path);
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, RED_COLOR "fopen() error: %s\n" RESET_COLOR, strerror(errno));
        return;
    }
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }
    fclose(file);
}

void execute_file(const char* path) {
    printf("Executing file: %s\n", path);
    int status = system(path);
    if (status == -1) {
        fprintf(stderr, RED_COLOR "Execution failed: %s\n" RESET_COLOR, strerror(errno));
    } else {
        printf("Execution of %s finished with status %d\n", path, WEXITSTATUS(status));
    }
}

void handle_found_file(const char* path) {
    char* ext = strrchr(path, '.');
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        fprintf(stderr, RED_COLOR "stat() error: %s\n" RESET_COLOR, strerror(errno));
        return;
    }
    if (ext) {
        if (strcmp(ext, ".txt") == 0) {
            display_txt_file(path);
        } else if (statbuf.st_mode & S_IXUSR) {
            execute_file(path);
        } else {
            display_txt_file(path);
        }
    } else {
        display_txt_file(path);
    }
}

void seek_recursive(const char* base_path, const char* target, const char* target_without_ext, int d_flag, int f_flag, int e_flag, int* found_count, char* result_path, const char* original_dir, Details* details, const char* start_dir) {
    struct dirent *dp;
    DIR *dir = opendir(base_path);
    if (!dir) return;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);
        struct stat statbuf;
        if (stat(path, &statbuf) == -1) {
            perror("stat error");
            continue;
        }
        int is_dir = S_ISDIR(statbuf.st_mode);
        if ((d_flag && !is_dir) || (f_flag && is_dir))
            continue;
        int matches = 0;
        char* ext = strrchr(dp->d_name, '.');
        size_t name_len = ext ? (size_t)(ext - dp->d_name) : strlen(dp->d_name);
        if (strncmp(dp->d_name, target, strlen(target)) == 0) {
            matches = 1;
        }
        if (matches) {
            (*found_count)++;
            char relative_path[MAX_PATH];
            if (strncmp(path, start_dir, strlen(start_dir)) == 0) {
                snprintf(relative_path, sizeof(relative_path), ".%s", path + strlen(start_dir));
            } else {
                snprintf(relative_path, sizeof(relative_path), "%s", path);
            }
            if (e_flag && *found_count == 1) {
                strncpy(result_path, path, MAX_PATH - 1);
                result_path[MAX_PATH - 1] = '\0';
            } else {
                print_colored(relative_path, is_dir);
            }
        }
        if (is_dir) {
            seek_recursive(path, target, target_without_ext, d_flag, f_flag, e_flag, found_count, result_path, original_dir, details, start_dir);
        }
    }
    closedir(dir);
}

void seek(const char* target, const char* directory, int d_flag, int f_flag, int e_flag, Details* details) {
    int found_count = 0;
    char result_path[MAX_PATH] = {0};
    if (directory == NULL) {
        directory = ".";
    }
    char* ext = strrchr(target, '.');
    char target_without_ext[MAX_PATH] = {0};
    if (ext) {
        strncpy(target_without_ext, target, ext - target);
        target_without_ext[ext - target] = '\0';
    } else {
        strncpy(target_without_ext, target, MAX_PATH - 1);
    }
    char original_dir[MAX_PATH];
    if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
        fprintf(stderr, RED_COLOR "Error: Unable to get current working directory.\n" RESET_COLOR);
        return;
    } 
    char start_dir[MAX_PATH];
    if (getcwd(start_dir, sizeof(start_dir)) == NULL) {
        fprintf(stderr, RED_COLOR "Error: Unable to get starting directory.\n" RESET_COLOR);
        return;
    } 
    seek_recursive(directory, target, target_without_ext, d_flag, f_flag, e_flag, &found_count, result_path, original_dir, details, start_dir);
    if (e_flag && found_count == 1) {
        struct stat statbuf;
        if (stat(result_path, &statbuf) == -1) {
            fprintf(stderr, RED_COLOR "Error: Unable to stat file %s.\n" RESET_COLOR, result_path);
            return;
        }
        if (!check_permissions(result_path, S_ISDIR(statbuf.st_mode))) {
            fprintf(stderr, RED_COLOR "Error: Missing permissions for %s.\n" RESET_COLOR, result_path);
            return;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            printf("Changing the directory to %s\n", target);
            hop(target, details);
        } else {
            handle_found_file(result_path);
        }
    } else if (e_flag && found_count > 1) {
        fprintf(stderr, RED_COLOR "Error: Multiple matches found. -e flag has no effect.\n" RESET_COLOR);
    } else if (found_count == 0) {
        fprintf(stderr, RED_COLOR "Error: No matches found.\n" RESET_COLOR);
    }
}

void handle_seek(char* input,Details* details) {
    char* copy = strdup(input);
    char* token;
    char* delim = " ";
    char* path = NULL;
    int D_FLAG = 0, F_FLAG = 0, E_FLAG = 0;
    token = strtok(copy, delim);
    while (token) {
        if (strcmp(token, "-d") == 0) D_FLAG = 1;
        else if (strcmp(token, "-f") == 0) F_FLAG = 1;
        else if (strcmp(token, "-e") == 0) E_FLAG = 1;
        else if (strcmp(token, "seek") != 0) path = strdup(token);
        token = strtok(NULL, delim);
    }
    if (!path) path = strdup(".");
    char* cwd = (char*)malloc(sizeof(char) * MAX_CWD_LEN);
    cwd = getcwd(cwd, sizeof(cwd));
    if(D_FLAG && F_FLAG)
    {
        fprintf(stderr, RED_COLOR "Invalid combination of flags.\n" RESET_COLOR);
        return;
    }
    seek(path, cwd, D_FLAG, F_FLAG, E_FLAG,details);
    free(cwd);
    free(path);
    free(copy);
}