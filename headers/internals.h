#ifndef INTERNALS_H
#define INTERNALS_H

int print_history(char *hist_arg);
char *expand_wildcar(char *s);


// Parsing della linea di comando
int gest_pv (char **comandi, char *comando);
int gest_and(char* c, int* cmd_id, int subcmd_id, int log_out, int log_err);
int redirect(char* c,int* cmd_id, int subcmd_id, int log_out, int log_err);

#endif
