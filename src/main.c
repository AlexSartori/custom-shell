#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#include "exec.h"
#include "utils.h"
#include "vector.h"


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

// Legge il comando e splitta in corrispondenza di ';' eliminando gli spazi 
int gest_pv (char **comandi){
    char* c = strtok(comando,";");
    int cont_comandi = 0;
    while (c){ 
        int inizio = 0;
        int fine = strlen(c);
        if (fine == 0) continue; // Comando vuoto
        while(c[inizio] == ' ' && inizio != fine) inizio++;
        c = c + inizio;
        if( inizio == fine){     // Solo spazi
            c = strtok(NULL,";");
            continue;
        }
        inizio = 0;
        fine = strlen(c) - 1;
        while( c[fine - 1] == ' ' && fine != inizio) fine --;
        if( fine != inizio) c[fine + 1] = '\0';
        
        comandi[cont_comandi] = c;
        c = strtok(NULL,";");
        cont_comandi++;
    }
return cont_comandi;
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
    vector_alias_initializer();

    while(1) {
        get_prompt(prompt);
        comando = readline(prompt);
        if (comando == NULL) break; // Era una linea vuota
        if (strlen(comando) == 0) continue; // Invio
        

        // Gestisco history
        add_history(comando);

        // Controlla se ci sono punto e virgola
        int pv = 0;
        int cont = 0;
        for(int i = 0; comando[i] != '\0'; i++) if(comando[i] == ';') pv++;
        char* comandi[pv + 1];
        if (pv!=0) {
            cont = gest_pv(comandi);
        } else {
            cont = 1;
            comandi[cont - 1] = comando;
        }

        for (int j=0; j<cont; j++){
            cmd_id++;
            subcmd_id = 0;
            
            if(comandi[j] == NULL || strlen(comandi[j]) == 0 ) continue;
                        
            // Controlla se il comando è fatto di soli spazi
            int spazi = 0;
            for(int k=0; k<strlen(comandi[j]); k++) if( comandi[j][k]== ' ') spazi++;
            if( strlen(comandi[j]) == spazi) continue;

            struct PROCESS p = exec_line(comandi[j], cmd_id, &subcmd_id, log_out, log_err);
            int status;
            wait(&status);
            printf("Exit code: %d\n", status); // Boh non capiscose è giusto
            write_to(p.stdout, log_out, 1);
            write_to(p.stderr, log_err, 2);
            // printf("Exit code: %d\n", p.status);
        }
    }

    // Pulisci tutto ed esci
    shell_exit(0);
    return 0;
}
