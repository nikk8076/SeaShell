#ifndef SEEK_H
#define SEEK_H
#include "details.h"
#define RED_COLOR "\033[1;31m"
#define GREEN_COLOR "\033[1;32m"
#define BLUE_COLOR "\033[1;34m"
#define RESET_COLOR "\033[0m"

void display_txt_file(const char* path);

void execute_file(const char* path);

void handle_found_file(const char* path);

void seek_recursive(const char* base_path, const char* target, const char* target_without_ext, int d_flag, int f_flag, int e_flag, int* found_count, char* result_path, const char* original_dir, Details* details, const char* start_dir);

void seek(const char* target, const char* directory, int d_flag, int f_flag, int e_flag, Details* details);

void handle_seek(char* input,Details* details);

void print_colored(const char* name, int is_dir);

int check_permissions(const char* path, int is_dir);

#endif