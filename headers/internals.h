/*
    Comandi interni
*/

#ifndef INTERNALS_H
#define INTERNALS_H

#define MAXSIZE 1024

#include "../headers/exec.h"

int print_history(char *hist_arg);
char *expand_wildcard(char *s);


// Parsing della linea di comando
char** split_pv (char *comando);
int gest_and(char* c);
int redirect(char* c, struct PROCESS *ret_p);
int do_for(char** args);

#endif
