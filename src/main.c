#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "exec.h"
#include "utils.h"
#include "queue.h"


int main(int argc, char** argv) {

    int log_out, log_err; // File di log

    read_options(argc, argv);

    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    // Buffer per l'input dell'utente
    char *comando, prompt[BUF_SIZE];

    //tab autocomplete
    rl_bind_key('\t', rl_complete);

    while(1) {
        get_prompt(prompt);
        size_t DIM_BUFFER_COMANDO = 1000;
        comando = (char *) malloc(DIM_BUFFER_COMANDO*sizeof(char));
        comando = readline(prompt);

        if (comando == NULL) break; // Era una linea vuota

        // Gestisco history
        add_history(comando);

        struct PROCESS p;
        p = exec_line(comando);

        // Leggi stdout del figlio e scrivilo su stdout e logfile
        char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
        int r = 0;
        while ((r = read(p.stdout, buf, BUF_SIZE)) > 0) {
            buf[r] = '\0';
            fprintf(stdout, "%s", buf);
            write(log_out, buf, r);
        }
        close(p.stdout);


        // Leggi stderr del figlio e scrivilo su stderr e logfile
        while ((r = read(p.stderr, buf, BUF_SIZE)) > 0) {
            buf[r] = '\0';
            fprintf(stderr, "%s", buf);
            write(log_err, buf, r);
        }
        close(p.stderr);


        free(buf);
        free(comando);
    }


    // Pulisci tutto ed esci
    shell_exit(0);
    close(log_out);
    close(log_err);
    return 0;
}
