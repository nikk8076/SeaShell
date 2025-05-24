#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<grp.h>
#define RED "\033[1;31m"
#define RESET "\x1b[0m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"

void run_fg(int pid)
{
    char path[256];
    snprintf(path,sizeof(path),"/proc/%d/status",pid);
    FILE *fp = fopen(path,"r");
    if(!fp)
    {
        fprintf(stderr, RED "No such process\n" RESET);
        return;
    }
    printf("Running process with pid %d in foreground\n",pid);
    kill(pid,SIGCONT);
    int status;
    waitpid(pid,&status,WUNTRACED);
    printf("Fg process terminated\n");
}