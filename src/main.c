#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "exec.h"
#include "utils.h"
#include "queue.h"


void sigHandler(int sig) {
    shell_exit(0);
}

int main(int argc, char** argv) {

    int log_out, log_err; // File di log
    int cmd_id = 1, subcmd_id = 0; // ID incrementale del comando, per il file di log

    read_options(argc, argv);

    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0644);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0644);

    // Buffer per l'input dell'utente
    char *comando, prompt[BUF_SIZE];

    //tab autocomplete
    rl_bind_key('\t', rl_complete);

    // Catch Ctrl-C
    signal(SIGINT, sigHandler);

    while(1) {
        get_prompt(prompt);
        size_t DIM_BUFFER_COMANDO = 1000;
        comando = (char *) malloc(DIM_BUFFER_COMANDO*sizeof(char));
        comando = readline(prompt);

        if (comando == NULL) break; // Era una linea vuota

        // Gestisco history
        add_history(comando);

        struct PROCESS p;
        subcmd_id = 0;
        p = exec_line(comando, cmd_id, &subcmd_id);
        printf("Exit code: %d\n", p.status);

        free(comando);
        cmd_id++;
    }


    // Pulisci tutto ed esci
    shell_exit(0);
    close(log_out);
    close(log_err);
    return 0;
}
