#ifndef LOG_H
#define LOG_H
#define MAX_HISTORY_LEN 15
#include "details.h"

typedef struct histNode{
    char* command;
    struct histNode* next;
    struct histNode* prev;
}histNode;

typedef struct Queue{
    struct histNode* front;
    struct histNode* rear;
    int Qsize;
}Queue;

Queue* init_queue();

Queue* push(char* command,Queue* queue);

Queue* pop(Queue* queue);

histNode* init_node(char* command);

void add_command(Queue* queue,Details* details,char* command);

void load_history(Queue* queue,Details* details); 

void write_to_file(Queue* queue,Details* details);

void print_history(Queue* queue);

void purge_history(Queue* queue,Details* details);

void free_queue(Queue* queue);

#endif