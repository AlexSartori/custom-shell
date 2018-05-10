#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#include "exec.h"
#include "utils.h"
#include "queue.h"


// Buffer per l'input dell'utente e il prompt
char *comando, prompt[BUF_SIZE];
int log_out, log_err; // File di log
int cmd_id = 0, subcmd_id = 0; // ID incrementale del comando, per il file di log

void init_shell(struct OPTIONS opt) {
    comando = (char *)malloc(BUF_SIZE*sizeof(char)),
    //tab autocomplete
    rl_bind_key('\t', rl_complete);
    // Catch Ctrl-C
    signal(SIGINT, sigHandler);
    // Apri i file di log
    log_out = open(opt.log_out_path, O_RDWR | O_CREAT, 0644);
    log_err = open(opt.log_err_path, O_RDWR | O_CREAT, 0644);
}


void shell_exit(int status) {
    printf("\nExiting normally...\n");
    // Libera i buffer
    free(comando);
    // Chiudi i file
    close(log_out);
    close(log_err);
    // Esci
    exit(status);
}

void sigHandler(int sig) {
    shell_exit(0);
}

int main(int argc, char** argv) {
    struct OPTIONS opt = read_options(argc, argv);
    init_shell(opt);

    while(1) {
        get_prompt(prompt);
        comando = readline(prompt);
        if (comando == NULL) break; // Era una linea vuota

        // Gestisco history
        add_history(comando);

        cmd_id++;
        subcmd_id = 0;
        struct PROCESS p = exec_line(comando, cmd_id, &subcmd_id, log_out, log_err);
        int status;
        wait(&status);
        printf("Exit code: %d\n", status); // Boh non capiscose è giusto
        write_to(p.stdout, log_out, 1);
        write_to(p.stderr, log_err, 2);
        // printf("Exit code: %d\n", p.status);
    }

    // Pulisci tutto ed esci
    shell_exit(0);
    return 0;
}
