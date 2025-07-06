#include "nano.h"
#include "redirect.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void take_inputs(char* home_dir)
{
    char myshrc_file[4096];
    snprintf(myshrc_file,sizeof(myshrc_file),"%s/.myshrc",home_dir);
    FILE* file=fopen(myshrc_file,"a");
    while(1)
    {
        printf("Do you want to add an alias?(yes/no)");
        char* answer=(char*)malloc(sizeof(char)*10);
        scanf("%s",answer);
        if(strcmp(answer,"no")==0)
        {
            break;
        }
        printf("Enter an alias:");
        scanf("%*c");
        char* alias=(char*)malloc(sizeof(char)*4096);
        scanf("%[^\n]s",alias);
        fprintf(file,"%s\n",alias);
        scanf("%*c");
        free(answer);
        free(alias);
    }
    fclose(file);
}

char* check_for_alias(char* home_dir,char* input)
{
    char myshrc_file[4096];
    snprintf(myshrc_file,sizeof(myshrc_file),"%s/.myshrc",home_dir);
    FILE* file=fopen(myshrc_file,"r"); //reveala = reveal -a
    if(file)
    {
        char line[4096];
        while(fgets(line,sizeof(line),file))
        {
            char* str=strdup(line);
            str[strcspn(str,"\n")]=0;
            char* token=strtok(str,"=");
            char* args[2];
            int a=0;
            while(token)
            {
                char* s1=strdup(token);
                s1=remove_extra_space(s1);
                args[a]=(char*)malloc(sizeof(char)*strlen(token));
                strcpy(args[a],s1);
                token=strtok(NULL,"=");
                ++a;
            }
            if(strcmp(args[0],input)==0)
            {
                fclose(file);
                return args[1];
            }
        }
    }
    fclose(file);
    return NULL;
}