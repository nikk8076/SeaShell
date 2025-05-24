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
#include "hop.h"
#include "log.h"
#include "reveal.h"

#define MAX_BG_PROCESSES 100
#define MAX_ARGS 64
#define RED_COLOR "\033[1;31m"
#define GREEN_COLOR "\033[1;32m"
#define BLUE_COLOR "\033[1;34m"
#define RESET_COLOR "\033[0m"

BgProcess bg_processes[MAX_BG_PROCESSES];
int bg_count = 0;
int next_bg_index = 1;

void tokenize_and_trim(const char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* home);

char* trim(const char* str);

void execute_command(char* command, int bg_no, int* elapsed_time, char** last_command);

int count_ampersands(const char* input);

void handle_command(char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* home);

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

void handle_reveal(char *input, Details *details,char* homedir) {
    // printf("%s %s\n",details->currentDirectory,details->previousDirectory);
    char *CWD = (char *)malloc(sizeof(char) * MAX_PATH);
    if (CWD == NULL) {
        perror("malloc() error");
        return;
    }
    if (getcwd(CWD, MAX_PATH) == NULL) {
        perror("getcwd() error");
        free(CWD);
        return;
    }
    int L_FLAG = 0, A_FLAG = 0;
    char *pathString = NULL;
    char *str = strdup(input);
    if (str == NULL) {
        perror("strdup() error");
        free(CWD);
        return;
    }
    char *token;
    char *delim = " ";
    token = strtok(str, delim);
    while (token) {
        if (token[0] == '-') {
            for (int i = 0; i < strlen(token); i++) {
                if (token[i] == 'a') A_FLAG = 1;
                if (token[i] == 'l') L_FLAG = 1;
            }
        } else if (strcmp(token, "reveal") != 0) {
            if (pathString == NULL) {
                pathString = strdup(token);
                if (pathString == NULL) {
                    perror("strdup() error");
                    free(CWD);
                    free(str);
                    return;
                }
            }
        }
        token = strtok(NULL, delim);
    }
    if (pathString == NULL) {
        int a=0;
        for(int i=0;i<strlen(input);i++)
        {
            if(input[i]=='-')
            {
                if(i+1<strlen(input) &&  input[i+1]=='\0') a=1;
                if(i+1==strlen(input)) a=1;
            }
        }
        if(a) pathString=strdup("-");
        else pathString = strdup(".");
        if (pathString == NULL) {
            perror("strdup() error");
            free(CWD);
            free(str);
            return;
        }
    }
    if (details->currentDirectory == NULL) {
        details->currentDirectory = strdup(CWD);
        if (details->currentDirectory == NULL) {
            perror("strdup() error");
            free(CWD);
            free(str);
            free(pathString);
            return;
        }
    }
    // printf("%s\n",pathString);
    reveal(L_FLAG, A_FLAG,pathString, details, homedir);
    free(pathString);
    free(CWD);
    free(str);
}

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

void handle_command(char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* homedir) {
    if (strncmp(input, "log", 3) == 0) {
        if(input[3]!=' ' && strlen(input)>3)
        {
            fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
            return;
        }
        if (strcmp("log", input) == 0) {
            print_history(queue);
        } else if (strcmp(input, "log purge") == 0) {
            purge_history(queue, details);
        } else if (strncmp(input, "log execute", 11) == 0) {
            int a = 0;
            for (int i = strlen(input) - 1; i >= 12; i--) {
                if (input[i] >= '0' && input[i] <= '9') {
                    a += (input[i] - '0') * (pow(10, strlen(input) - 1 - i));
                }
            }
            histNode* temp = queue->rear;
            --a;
            while (a && temp) {
                temp = temp->prev;
                --a;
            }
            if(!temp)
            {
                fprintf(stderr, RED_COLOR "%d  is out of range\n" RESET_COLOR, a+1);
                return;
            }
            tokenize_and_trim(temp->command, details, queue, last_command, elapsed_time,homedir);
        }
        else
            fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
    } else {
        if (strncmp(input, "hop", 3) == 0) {
            if(input[3]!=' ' && strlen(input)>3)
            {
                fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
                return;
            }
            handle_hop(input, details);
        } else if (strncmp(input, "reveal", 6) == 0) {
            if(input[6]!=' ' && strlen(input)>6)
            {
                fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
                return;
            }
            handle_reveal(input,details,homedir);
        } else if (strncmp(input, "proclore", 8) == 0) {
            if(input[8]!=' ' && strlen(input)>8)
            {
                fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
                return;
            }
            pid_t pid;
            if (strcmp(input, "proclore") == 0) {
                pid = getpid();
                proclore(pid);
                return;
            }
            int num = 0;
            for (int i = strlen(input) - 1; ~i; --i) {
                if (input[i] == ' ') break;
                else num += pow(10, strlen(input) - 1 - i) * (input[i] - '0');
            }
            pid = num;
            printf("%d\n", pid);
            proclore(pid);
        } else if (strncmp(input, "seek", 4) == 0) { 
            if(input[4]!=' ')
            {
                fprintf(stderr, RED_COLOR "Invalid command\n" RESET_COLOR);
                return;
            }
            handle_seek(input, details);
        } else {
            int ampersand_count = count_ampersands(input);
            int command_count = 0;
            char* token = strtok(input, "&");
            while (token != NULL) {
                command_count++;
                int bg = (command_count <= ampersand_count) ? command_count : 0;
                execute_command(token, bg, elapsed_time, last_command);
                token = strtok(NULL, "&");
            }
        }
    }
}

char* trim(const char* str) {
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

void tokenize_and_trim(const char* input,Details* details,Queue* queue, char** last_command, int* elapsed_time,char* initial) {
    const char* start = input;
    const char* end = input;
    while(*end) {
        if(*end == ';') {
            size_t token_len = end - start;
            char* token = malloc(token_len + 1);
            if(token) {
                strncpy(token, start, token_len);
                token[token_len] = '\0';
                char* trimmed = trim(token);
                handle_command(trimmed,details,queue,last_command,elapsed_time,initial);
                free(trimmed);
                free(token);
            }
            start=end+1; 
        }
        end++;
    }
    if(start != end) {
        char* token = strdup(start);
        if(token) {
            char* trimmed = trim(token);
            if (trimmed)
                handle_command(trimmed,details,queue,last_command,elapsed_time,initial);
            free(token);
        }
    }
}

int main()
{
    setup_signal_handler();
    Details* details=(Details*)malloc(sizeof(Details));
    details=getDetails(details);
    char* input=(char*)malloc(sizeof(char)*MAX_INPUT_LEN);
    upd();
    char* last_command=NULL;
    Queue* queue=NULL;
    int elapsed_time=0;
    char INITIAL[4096];
    getcwd(INITIAL,sizeof(INITIAL));
    while (1) {
        if(strcmp(input,"exit")==0) break;
        queue = init_queue();
        load_history(queue, details);
        if (elapsed_time > 2) {
            int first_command_start=-1,first_command_end=-1;
            for(int i=0;i<strlen(last_command);i++)
            {
                if(last_command[i]!=' ')
                {
                    first_command_start=i;
                    break;
                }
            }
            for(int i=first_command_start;i<strlen(last_command);i++)
            {
                if(last_command[i]==' ')
                {
                    first_command_end=i-1;
                    break;
                }
            }
            char* base_command=(char*)malloc(sizeof(char)*(first_command_end-first_command_start+1));
            for(int i=first_command_start;i<=first_command_end;i++)
                base_command[i-first_command_start]=last_command[i];
            printf(GREEN_COLOR "<%s@%s" RESET_COLOR BLUE_COLOR ":%s " RESET_COLOR "%s > %ds> ", details->username, details->hostname, details->currentDirectory,base_command,elapsed_time);
            free(last_command); 
            elapsed_time=0;
            last_command = NULL;
            free(base_command);
        } else {
            printf(GREEN_COLOR "<%s@%s" RESET_COLOR BLUE_COLOR ":%s" RESET_COLOR "> ", details->username, details->hostname, details->currentDirectory);
        }
        scanf("%[^\n]s", input);
        char* input_copy2 = strdup(input);
        if (strstr(input_copy2, "log") == NULL)
            add_command(queue, details, input_copy2);
        tokenize_and_trim(input_copy2, details, queue, &last_command, &elapsed_time,INITIAL);
        scanf("%*c");  
    }
}