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
#include "redirect.h"
#include "proclore.h"
#include "seek.h"
#include "commands.h"
#include "process.h"

int main()
{
    setup_signal_handler();
    // initialize_shell();
    Details* details=(Details*)malloc(sizeof(Details));
    details=getDetails(details);
    char* input=(char*)malloc(sizeof(char)*MAX_INPUT_LEN);
    upd();
    char* last_command=NULL;
    Queue* queue=NULL;
    int elapsed_time=0;
    char INITIAL[4096];
    getcwd(INITIAL,sizeof(INITIAL));
    char buffer[256];
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
    return 0;
}