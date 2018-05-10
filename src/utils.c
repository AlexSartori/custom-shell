#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include "utils.h"


/*
    Copia source nel file di log e nella destinazione (stdout, stdin di un altro processo, ecc.).
*/
void write_to(int source, int log_file, int destination) {
    char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
    int r = 0;
    while ((r = read(source, buf, BUF_SIZE)) > 0) {
        write(log_file, buf, r);
        write(destination, buf, r);
    }
    free(buf);
}


/*
    Legge le opzioni con cui la shell è stata chiamata e setta le variabili]
    Opzioni attese:
    -o <file>, --outfile=<file>
    -e <file>, --errfile=<file>
    -m <int>, --maxsize=<int>
    -r, --retcode

    Params:
        argc, argv: quelli del main da leggere

    See:
        https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html#Getopt-Long-Options
*/
struct OPTIONS read_options(int argc, char** argv) {
    /*
        Array di struct OPTION che descrivono le opzioni aspettate

        Params:
            name:    nome dell'opzione
            has_arg: se l'opzione richiede un argomento obbligatorio
            flag:    quando questa opzione verrà trovata, val verrà messo dove punta flag
            val:     vedi sopra, (se flag == NULL, viene ritornato val)
    */
    struct option long_opts[] = {
        { "outfile", required_argument, 0, 'o' },
        { "errfile", required_argument, 0, 'e' },
        { "maxsize", required_argument, 0, 'm' },
        { "retcode", no_argument,       0, 'r' },
        { "help",    no_argument,       0, 'h' },
        { 0, 0, 0, 0 } // Serve come delimitatore finale dell'array
    };

    struct OPTIONS ret; // Contenitore delle opzioni da ritornare, lo riempio coi deafult
    strcpy(ret.log_out_path, "log_stdout.txt");
    strcpy(ret.log_err_path, "log_stderr.txt");

    char c;
    int indexptr;
    opterr = 0; // Non stampare messaggi

    while ((c = getopt_long(argc, argv, ":o:e:m:rh", long_opts, &indexptr)) != -1) {
        switch (c) {
            case 'h':
                print_help();
                break;
            case 'o':
                printf("  Outfile:\t%s\n", optarg);
                strcpy(ret.log_out_path, optarg);
                break;
            case 'e':
                printf("  Errfile:\t%s\n", optarg);
                strcpy(ret.log_err_path, optarg);
                break;
            case 'm':
                printf("  Max Size:\t%s [bytes]\n", optarg);
                break;
            case 'r':
                printf("  Record process return code: yes\n");
                break;

            case ':':
                fprintf(stderr, "Argument required for -%c\n", optopt);
                break;
            case '?':
                fprintf(stderr, "Unknown option: %s\n", argv[optind-1]);
                break;
            default:
                fprintf(stderr, "Errore nella lettura delle opzioni (codice opzione: %c)\n", c);
                break;
        }
    }

    // Se sono rimasti argomenti non riconosciuti avverti l'utente
    for (int i = optind; i < argc; i++)
        fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);

    return ret;
}


/*
    Stampa l'aiuto
*/
void print_help() {
    printf("\n\n\
                     C u s t o m   S h e l l\n\
                   Progetto Sistemi Operativi 1\n\n\n\
  Usage:\n\
      ./shell <options>\n\n\
  Options:\n\
      -h, --help           Mostra questo messaggio\n\
      -o, --outfile        File di log dello stdout dei comandi\n\
      -e, --errfile        File di log dello stderr dei comandi\n\
      -m, --maxsize        Dimensione massima in bytes dei file di log\n\
      -r, --retcode        Registra anche i codici di ritorno dei comandi\n\n\n");
}


/*
    Ritorna il nome dell'utente attivo (altrimenti la stringa "user")
*/
char* getuser() {
  register struct passwd *pw;
  register uid_t uid;

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
char* get_prompt(char* prompt) {
    char path[BUF_SIZE];

    if (getcwd(path, BUF_SIZE) == NULL) perror("Path cannot fit in the buffer");
    snprintf(prompt, BUF_SIZE, "[%s%s%s @ %s%s%s] -> ", KCYN, getuser(), KNRM, KYEL, path, KNRM);

    return prompt;
}
