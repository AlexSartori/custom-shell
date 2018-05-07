#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include "utils.h"

int main() {

    int log_out, log_err; // File di log
    int child_out[2], child_err[2]; // Pipe per leggere out ed err dei comandi da eseguire

    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    // Buffer per l'input dell'utente
    char *comando;

    while(1) {
        print_prompt();

        size_t DIM_BUFFER_COMANDO = 1000;
        comando = (char *) malloc(DIM_BUFFER_COMANDO*sizeof(char));
        getline(&comando, &DIM_BUFFER_COMANDO, stdin);

        //rimuovo caratteri che potrebbero farmi fallire strcmp
        comando[strcspn(comando, "\r\n")] = 0;
        comando = strtok (comando," ");

        if (strcmp(comando, "clear") == 0) clear();
        else if(strcmp(comando, "exit") == 0) break;
        else if(strcmp(comando, "help") == 0) printhelp();
        else if(strcmp(comando, "cd") == 0) {
            int status = chdir(strtok (NULL, " "));
            if(status == -1) {
                printcolor("! Errore: cartella inesistente\n", KRED);
            }
        }
        else {
            // TODO fare un parse_line, ad exec_cmd va passato un array di argomenti non la stringa...

            /*
                Francesco:
                va bene così? una roba tipo "comando" spazio "argomenti"
                possiamo anche leggere più argomenti tipo "comando -a1 -a2 -a3" ma va estesa questa parte
                stavo pensando di leggere tutti i tokens in una volta e metterli in una coda,
                durante la lettura voglio catturare le variabili d'ambiente, vedere se matchano con quelle dichiarate
                e mettere direttamente nella coda il contenuto della variabile
                poi la coda dovrebbe diventare l'array per i parametri sotto
            */
            exec_cmd((char* []){comando, strtok (NULL, " "), NULL}, log_out, log_err, child_out, child_err);
        }


        /*
        while (comando_split != NULL) {
            printf ("%s\n",comando_split);

            //next token
            comando_split = strtok (NULL, " ");
        }
        */

        /*
        // Controlla se è un comando interno ed interpretalo,
        // altrimenti eseguilo come figlio
        if (strcmp(comando, "clear") == 0) clear();
        else if(strcmp(comando, "exit") == 0) break;
        else if(strcmp(comando, "help") == 0) printhelp();
        else {
            // TODO fare un parse_line, ad exec_cmd va passato un array di argomenti non la stringa...
            exec_cmd((char* []){ comando, NULL });
        }
        */

        free(comando);
    }


    // Pulisci tutto ed esci
    close(log_out);
    close(log_err);
    return 0;
}
