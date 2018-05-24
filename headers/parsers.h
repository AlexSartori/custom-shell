#ifndef PARSERS_H
#define PARSERS_H


void vectors_initializer();


// Gestione alias
char* parse_alias(char* comando);
int list_alias();
int make_alias(char *copy_line);
int search_alias(elemento* el);


// Gestione variabili d'ambiente
int list_vars();
// int search_var_elemento(elemento* el);
char* search_var_name(char *name);
char* parse_vars(char *comando);
int make_var(char *copy_line);
void inc_var(char *name);
void azz_var(char *name);

#endif
