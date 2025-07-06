#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "reveal.h"

#define MAX_PATH 4096
#define COLOR_RESET "\x1b[0m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_GREEN "\x1b[32m"

int compare(const void *a, const void *b) {
    struct dirent *entryA = *(struct dirent **)a;
    struct dirent *entryB = *(struct dirent **)b;
    return strcasecmp(entryA->d_name, entryB->d_name);
}

void reveal(int L_FLAG, int A_FLAG, char *path, Details *details, const char *initial_dir) {
    char full_path[MAX_PATH] = {0};
    char resolved_current[MAX_PATH] = {0};
    char resolved_previous[MAX_PATH] = {0};
    struct dirent **namelist = NULL; 
    int n = 0;
    struct stat file_stat;
    char time_str[80];
    struct passwd *pw;
    struct group *gr;
    blkcnt_t total_blocks = 0;
    if (details->currentDirectory[0] == '~') {
        strncat(resolved_current, initial_dir, sizeof(resolved_current) - strlen(resolved_current) - 1);
        strncat(resolved_current, details->currentDirectory + 1, sizeof(resolved_current) - strlen(resolved_current) - 1);
    } else {
        realpath(details->currentDirectory, resolved_current);
    }
    if (details->previousDirectory) {
        if (details->previousDirectory[0] == '~') {
            strncat(resolved_previous, initial_dir, sizeof(resolved_previous) - strlen(resolved_previous) - 1);
            strncat(resolved_previous, details->previousDirectory + 1, sizeof(resolved_previous) - strlen(resolved_previous) - 1);
        } else {
            realpath(details->previousDirectory, resolved_previous);
        }
    }
    if (strcmp(path, "~") == 0) {
        strncat(full_path, initial_dir, sizeof(full_path) - 1);
    } else if (strcmp(path, "-") == 0) {
        if (details->previousDirectory) {
            strncat(full_path, resolved_previous, sizeof(full_path) - 1);
        } else {
            printf("Error: previous directory not available\n");
            return;
        }
    } else if (path[0] == '/') {
        strncat(full_path, path, sizeof(full_path) - 1);
    } else {
        strncat(full_path, resolved_current, sizeof(full_path) - strlen(full_path) - 1);
        strncat(full_path, "/", sizeof(full_path) - strlen(full_path) - 1);
        strncat(full_path, path, sizeof(full_path) - strlen(full_path) - 1);
    }
    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!A_FLAG && entry->d_name[0] == '.') {
            continue; 
        }
        char file_path[MAX_PATH] = {0};
        strncat(file_path, full_path, sizeof(file_path) - strlen(file_path) - 1);
        strncat(file_path, "/", sizeof(file_path) - strlen(file_path) - 1);
        strncat(file_path, entry->d_name, sizeof(file_path) - strlen(file_path) - 1);
        if (stat(file_path, &file_stat) < 0) {
            perror("Error getting file stats");
            continue;
        }
        total_blocks += file_stat.st_blocks;
        n++;
    }
    printf("Total: %ld\n", total_blocks / 2);  
    namelist = malloc(n * sizeof(struct dirent *));
    if (namelist == NULL) {
        perror("malloc() error");
        closedir(dir);
        return;
    }
    rewinddir(dir);
    int index = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (!A_FLAG && entry->d_name[0] == '.') {
            continue; 
        }
        namelist[index] = malloc(sizeof(struct dirent));
        if (namelist[index] == NULL) {
            perror("malloc() error");
            closedir(dir);
            free(namelist);
            return;
        }
        memcpy(namelist[index], entry, sizeof(struct dirent));
        index++;
    }
    closedir(dir);
    qsort(namelist, n, sizeof(struct dirent *), compare);
    for (int i = 0; i < n; i++) {
        char file_path[MAX_PATH] = {0};
        strncat(file_path, full_path, sizeof(file_path) - strlen(file_path) - 1);
        strncat(file_path, "/", sizeof(file_path) - strlen(file_path) - 1);
        strncat(file_path, namelist[i]->d_name, sizeof(file_path) - strlen(file_path) - 1);
        if (stat(file_path, &file_stat) < 0) {
            perror("Error getting file stats");
            free(namelist[i]);
            continue;
        }
        if (L_FLAG) {
            printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
            printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
            printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
            printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
            printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
            printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
            printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
            printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
            printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
            printf(" %lu", file_stat.st_nlink);
            pw = getpwuid(file_stat.st_uid);
            gr = getgrgid(file_stat.st_gid);
            if (pw && gr) {
                printf(" %s %s", pw->pw_name, gr->gr_name);
            } else {
                printf(" ??? ???");
            }
            printf(" %8ld", file_stat.st_size);
            strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&file_stat.st_mtime));
            printf(" %s", time_str);
        }
        if (S_ISDIR(file_stat.st_mode)) {
            printf(COLOR_BLUE " %s" COLOR_RESET, namelist[i]->d_name);
        } else if (file_stat.st_mode & S_IXUSR) {
            printf(COLOR_GREEN " %s" COLOR_RESET, namelist[i]->d_name);
        } else {
            printf(" %s", namelist[i]->d_name);
        }
        printf("\n");
        free(namelist[i]);
    }
    free(namelist);
}
