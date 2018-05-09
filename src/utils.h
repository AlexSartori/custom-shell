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
#define clear() printf("\033[H\033[J")
#define min(a, b) (a > b ? b : a)
#define max(a, b) (a > b ? a : b)


//PROTOTIPI FUNZIONI E PROCEDURE
char* getuser();
void printcolor(char *s, char *color);
void print_help();
void shell_exit(int status);
char* get_prompt(char* prompt);
void read_options(int argc, char** argv);

#endif
