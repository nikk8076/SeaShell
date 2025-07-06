#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include<stdio.h>
#include<stdlib.h>
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

int is_process_exists(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    return access(path, F_OK) != -1;
}

char get_process_status(int pid, int* is_foreground) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* stat_file = fopen(path, "r");
    if (!stat_file) {
        perror("fopen() error");
        return '?';
    }
    char status = '?';
    char comm[256];
    int pid_num, ppid, pgrp, session, tty_nr, tpgid;
    if (fscanf(stat_file, "%d %s %c %d %d %d %d %d", 
               &pid_num, comm, &status, &ppid, &pgrp, &session, &tty_nr, &tpgid) != 8) {
        fclose(stat_file);
        return '?';
    }
    fclose(stat_file);
    if (tty_nr == 0) {
        *is_foreground = 0;
        return status;
    }
    int tty_fd = open("/dev/tty", O_RDONLY);
    if (tty_fd == -1) {
        *is_foreground = 0;
        return status;
    }
    pid_t fg_pgrp;
    if (ioctl(tty_fd, TIOCGPGRP, &fg_pgrp) == -1) {
        close(tty_fd);
        *is_foreground = 0;
        return status;
    }
    close(tty_fd);
    *is_foreground = (pgrp == fg_pgrp);
    return status;
}

unsigned long get_virtual_memory_size(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/statm", pid);
    FILE* statm_file = fopen(path, "r");
    if (!statm_file) {
        perror("fopen() error");
        return 0;
    }
    unsigned long vm_size;
    if (fscanf(statm_file, "%lu", &vm_size) != 1) {
        fclose(statm_file);
        return 0;
    }
    fclose(statm_file);
    return vm_size * sysconf(_SC_PAGESIZE);
}

void get_executable_path(int pid, char* exe_path, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    ssize_t len = readlink(path, exe_path, size - 1);
    if (len != -1)
        exe_path[len] = '\0';
    else 
        snprintf(exe_path, size, "Unknown");
}

void proclore(int pid) {
    if (pid <= 0) 
        pid = getpid();
    if (!is_process_exists(pid)) {
        fprintf(stderr, "Error: Process with PID %d not found\n", pid);
        return;
    }
    printf("pid : %d\n", pid);
    int process_group = getpgid(pid);
    if (process_group == -1) 
        fprintf(stderr, "Error getting process group: %s\n", strerror(errno));
    else
        printf("process group : %d\n", process_group);
    int is_foreground = 0;
    char process_status = get_process_status(pid, &is_foreground);
    printf("process status : %c%s\n", process_status, is_foreground ? "+" : "");
    unsigned long vm_size = get_virtual_memory_size(pid);
    printf("virtual memory : %lu bytes\n", vm_size);
    char exe_path[256];
    get_executable_path(pid, exe_path, sizeof(exe_path));
    printf("executable path : %s\n", exe_path);
}