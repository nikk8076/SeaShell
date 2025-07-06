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
#include "proclore.h"
#include "commands.h"

void handle_reveal(char *input, Details *details,char* homedir) {
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
    reveal(L_FLAG, A_FLAG,pathString, details, homedir);
    free(pathString);
    free(CWD);
    free(str);
}

void handle_command(char* input, Details* details, Queue* queue, char** last_command, int* elapsed_time,char* homedir) {
    char* key=check_for_alias(homedir,input);
    if(key)
    {
        printf("%s\n",key); 
        tokenize_and_trim(key,details,queue,last_command,elapsed_time,homedir);
        return;
    }
    if(strstr(input,"|"))
    {
        execute_pipe(input,details,queue,last_command,elapsed_time,homedir);
        return;
    }
    else if(strstr(input,">") || strstr(input,">>") || strstr(input,"<"))
    {
        redirect(input,details,queue,last_command,elapsed_time,homedir);
        return;
    }
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
        }
        else if(strcmp(input,"nano .myshrc")==0)
        {
            take_inputs(homedir);
        } 
        else if(strcmp(input,"activities")==0)
        {
            int pid=getpid();
            print_children(pid);   
        }
        else if(strncmp(input,"ping",4)==0)
        {
            int pid=-1,signal=-1;
            char* str=strdup(input);
            char* token=strtok(str," ");
            int a=0;
            while(token)
            {
                ++a;
                if(a==2)
                    pid=atoi(token);
                if(a==3)
                    signal=atoi(token);
                token=strtok(NULL," ");
            }
            ping(pid,signal);
        }
        else if(strncmp(input,"fg",2)==0)
        {
            char* str=strdup(input);
            char* token=strtok(str," ");
            int a=0;
            int pid;
            while(token)
            {
                ++a;
                if(a==2)
                {
                    pid=atoi(token);
                }
                token=strtok(NULL," ");
            }
            run_fg(pid);
        }
        else if(strncmp(input,"bg",2)==0)
        {
            char* str=strdup(input);
            char* token=strtok(str," ");
            int a=0;
            int pid;
            while(token)
            {
                ++a;
                if(a==2)
                {
                    pid=atoi(token);
                }
                token=strtok(NULL," ");
            }
            run_bg(pid);
        }
        else if(strncmp(input,"neonate -n",10)==0)
        {
            char* dup_string=strdup(input);
            char* token=strtok(dup_string," ");
            int a=0;
            int time;
            while(token)
            {
                ++a;
                if(a==3)
                {
                    time=atoi(token);
                }
                token=strtok(NULL," ");
            }
            pid_t pid = fork();
            if (pid == 0) {
                int a=run_neonate(time);
                exit(0);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork");
            }
        }
        else if(strncmp(input,"iMan",4)==0)
        {
            if(strlen(input)==4)
            {
                fprintf(stderr,RED_COLOR "Argument missing\n" RESET_COLOR);
                return;
            }
            char* dup_string=strdup(input);
            char* token=strtok(dup_string," ");
            int a=0;
            while(token)
            {
                ++a;
                if(a==2)
                {
                    iMan(token);
                }
                token=strtok(NULL," ");
            }
        }
        else {
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