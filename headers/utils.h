#ifndef UTILS_H
#define UTILS_H

#define BUF_SIZE 2048       // Dimensione dei buffer

// Codici ASCII dei colori
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define min(a, b) (a > b ? b : a)
#define max(a, b) (a > b ? a : b)

#include "../headers/exec.h"

struct OPTIONS {
    char log_out_path[BUF_SIZE], log_err_path[BUF_SIZE];
    int max_size, hist_size, save_ret_code, timeout;
};

int save_ret_code;

int clear();
char* getuser();
void string_tolower(char s[]);
void printcolor(char *s, char *color);
int print_help();
void write_to(int source, int log_file, int destination);
void log_process(struct PROCESS p, char* cmd, int cmd_id, int subcmd_id, int* streams);
char* get_prompt(char* prompt);
struct OPTIONS read_options(int argc, char** argv);
void sigHandler(int sig);

#endif
