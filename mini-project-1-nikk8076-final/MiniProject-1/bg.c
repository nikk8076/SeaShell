#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define RED "\033[1;31m"
#define RESET "\x1b[0m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"

void run_bg(int pid)
{
    char path[256];
    snprintf(path,sizeof(path),"/proc/%d/status",pid);
    FILE *fp = fopen(path,"r");
    if(!fp)
    {
        fprintf(stderr,RED "No process found\n" RESET);                                                                         
        return;
    }
    fclose(fp);
    if(kill(pid,SIGCONT)==-1)
    {
        perror("failed to run\n");
        return;
    }
}