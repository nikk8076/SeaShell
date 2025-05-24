#include<unistd.h>
#include "details.h"
#include<string.h>
#include<stdlib.h>

Details* getDetails(Details* details)
{
    details=(Details*)malloc(sizeof(Details));
    details->username=(char*)malloc(sizeof(char)*MAX_USERNAME_LEN);
    details->hostname=(char*)malloc(sizeof(char)*MAX_HOSTNAME_LEN);
    details->currentDirectory=(char*)malloc(sizeof(char)*MAX_CWD_LEN);
    details->currentDirectory="~";
    details->previousDirectory=(char*)malloc(sizeof(char)*MAX_CWD_LEN);
    details->previousDirectory=details->currentDirectory;
    details->LOG_FILE=(char*)malloc(sizeof(char)*MAX_INPUT_LEN);
    char cwd2[1024];
    getcwd(cwd2,sizeof(cwd2));
    strcat(cwd2,"/history.txt");
    details->LOG_FILE=strdup(cwd2);
    details->username=getlogin();
    gethostname(details->hostname,256);
    return details;
}

Details* updateDir(Details* details,char* path)
{

}