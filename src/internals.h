#ifndef INTERNALS_H
#define INTERNALS_H

typedef struct elemento {
    char* name;
    char* data;
} elemento;


void vector_alias_initializer();
char* parse_alias(char* comando);
int list_alias();
int make_alias(char *copy_line);
int print_history(char *hist_arg);
int search_alias(elemento* el);

#endif
