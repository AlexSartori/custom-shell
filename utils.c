#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include "utils.h"

/*
    Stampa l'aiuto
*/
void printhelp() {
    char *help = "QUI CI METTIAMO UN BELL'HELP!\n";
    printf("%s", help);
}

/*
    Ritorna il nome dell'utente attivo (altrimenti la stringa "user")
*/
char* getuser() {
  register struct passwd *pw;
  register uid_t uid;
  int c;

  uid = geteuid ();
  pw = getpwuid (uid);
  return pw ? pw->pw_name : "user";
}


/*
    Stampa a video s con il colore indicato
    ESEMPIO: printcolor("Ciao\n", KRED);
*/
void printcolor(char *s, char *color) {
    printf("%s%s%s", color, s, KNRM);
}


/*
    Stampa il prompt della shell come "[utente@percorso]$"
*/
void print_prompt() {
    char* user = getuser();
    char path[BUF_SIZE];

    printf("[");
    printcolor(user, KNRM);
    printf(" @ ");
    getcwd(path, BUF_SIZE);
    printcolor(path, KYEL);
    printf("] -> ");
}


/*
    Esegui il comando passato come figlio e registra stdout e stderr.
    Parametro: array degli argomenti, il primo elemento sarà il comando
*/
int exec_cmd(char** args, int log_out, int log_err, int *child_out, int *child_err) {

    // Codice di ritorno del processo
    int ret_code = -1;
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) {
        // Child:
        //   - Chiudi pipe-end che non servono
        //   - Redirigi out/err ed esegui il comando
        //   - Chiudi i pipe

        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);
        ret_code = execvp(args[0], args);
        if (ret_code < 0) perror("Cannot execute command");
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
    } else {
        // Parent: chiudi i pipe che non servono e aspetta il figlio
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        wait(NULL);
    }

    // Leggi stdout del figlio e scrivilo su stdout e logfile
    char* buf_out = (char*)malloc(sizeof(char)*BUF_SIZE);
    if (read(child_out[PIPE_READ], buf_out, BUF_SIZE) > 0) { 
        fprintf(stdout, "%s", buf_out);                         
        write(log_out, buf_out, strlen(buf_out));                         
        free(buf_out);
    } 
    close(child_out[PIPE_READ]);
    

    // Leggi stderr del figlio e scrivilo su stderr e logfile
    char* buf_err = (char*)malloc(sizeof(char)*BUF_SIZE);
    if (read(child_err[PIPE_READ], buf_err, BUF_SIZE) > 0) {
        fprintf(stderr, "%s", buf_err);
        write(log_err, buf_err, strlen(buf_err)); 
        free(buf_err); 
    }
    close(child_err[PIPE_READ]);
    
    return ret_code;
}
