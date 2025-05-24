#ifndef DETAILS_H
#define DETAILS_H
#define MAX_USERNAME_LEN 256
#define MAX_HOSTNAME_LEN 256
#define MAX_INPUT_LEN 4096
#define MAX_CWD_LEN 256
#include <stdlib.h>
typedef struct BgProcess{
    int index;
    pid_t pid;
} BgProcess;

typedef struct Details{
    char* username;
    char* hostname;
    char* currentDirectory;
    char* previousDirectory;
    char* LOG_FILE;
}Details;

Details* getDetails(Details* details);

#endif