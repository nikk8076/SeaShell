#ifndef REVEAL_H
#define REVEAL_H
#define MAX_INPUT_LEN 4096
#include "details.h"

int compare(const void *a, const void *b);

void reveal(int L_FLAG, int A_FLAG,char* path, Details* details, const char *initial_dir);

#endif