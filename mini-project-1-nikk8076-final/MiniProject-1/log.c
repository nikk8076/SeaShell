#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "details.h"

#define RED "\033[31m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

Queue* init_queue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        printf("Failed to allocate memory to queue\n");
        return NULL;
    }
    queue->Qsize = 0;
    queue->front = NULL;
    queue->rear = NULL;
    return queue;
}

Queue* push(char* command, Queue* queue) {
    if (!queue || !command) {
        return queue;
    }
    histNode* temp = init_node(command);
    if (!temp) {
        printf("Failed to create queue node\n");
        return queue;
    }
    while (queue->Qsize >= MAX_HISTORY_LEN)
        queue = pop(queue);
    if (queue->rear == NULL)
        queue->front = queue->rear = temp;
    else {
        queue->rear->next = temp;
        temp->prev = queue->rear;
        queue->rear = temp;
    }
    queue->Qsize++;
    return queue;
}

Queue* pop(Queue* queue) {
    if(!queue)
    {
        fprintf(stderr, RED "Queue empty\n" RESET);
        return NULL;
    }
    if (queue->front) {
        return queue;
    }
    histNode* temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL)
        queue->rear = NULL;
    else
        queue->front->prev = NULL;
    free(temp->command);
    free(temp);
    queue->Qsize--;
    return queue;
}

histNode* init_node(char* command) {
    if (!command) return NULL;
    histNode* temp = (histNode*)malloc(sizeof(histNode));
    if (!temp) {
        fprintf(stderr, RED "Failed to create queue node\n" RESET);
        return NULL;
    }
    temp->command = strdup(command);
    if (!temp->command) {
        fprintf(stderr, RED "Failed to allocate memory for command\n" RESET);
        free(temp);
        return NULL;
    }
    temp->next = NULL;
    temp->prev = NULL;
    return temp;
}

void load_history(Queue* queue, Details* details) {
    if (!queue) {
        fprintf(stderr, RED "Allocate memory to queue before reading from file.\n" RESET);
        return;
    }
    FILE* file = fopen(details->LOG_FILE, "r");
    if (!file) {
        fprintf(stderr, RED "No previous history found.\n" RESET);
        return;
    }
    char line[MAX_INPUT_LEN];
    while (fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        if (strlen(line) > 0)
            queue = push(line, queue);
    }
    fclose(file);
}

void add_command(Queue* queue, Details* details, char* command) {
    if (!queue || !command || strlen(command) == 0)
        return;
    if (queue->rear && strcmp(queue->rear->command, command) == 0)
        return;
    queue = push(command, queue);
    write_to_log_file(queue, details);
}

void write_to_log_file(Queue* queue, Details* details) {
    if (!queue) {
        fprintf(stderr,RED "Allocate memory to queue before writing\n", RESET);
        return;
    }
    FILE* file = fopen(details->LOG_FILE, "w");
    if (!file) {
        fprintf(stderr,RED "Error opening file for writing\n" RESET);
        return;
    }
    histNode* temp = queue->front;
    while (temp != NULL) {
        fprintf(file, "%s\n", temp->command);
        temp = temp->next;
    }
    fclose(file);
}

void print_history(Queue* queue) {
    if (!queue || queue->Qsize == 0) {
        fprintf(stdout, RED "No recent commands\n" RESET);
        return;
    }
    histNode* temp = queue->front;
    int count = 1;
    while (temp != NULL) {
        printf("%d %s\n", count++, temp->command);
        temp = temp->next;
    }
}

void purge_history(Queue* queue, Details* details) {
    histNode* temp=queue->front;
    while(temp)
    {
        histNode* temp2=temp;
        temp=temp->next;
        free(temp2);
    }
    queue->front=queue->rear=NULL;
    FILE* file = fopen(details->LOG_FILE, "w");
    if (!file) {
        fprintf(stderr, RED "Error opening file for purging\n" RESET);
        return;
    }
    fclose(file);
    printf("Command history has been purged.\n");
}

void free_queue(Queue* queue) {
    if (!queue) return;
    while (queue->front != NULL)
        queue = pop(queue);
    free(queue);
}
