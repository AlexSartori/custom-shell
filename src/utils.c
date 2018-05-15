#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/wait.h>

#include "utils.h"

/*
    Converte una stringa in lowercase
*/
void string_tolower(char s[]) {
    for(int i = 0; s[i]; i++){
        s[i] = tolower(s[i]);
    }
}

/* Legge il comando e splitta
   in corrispondenza
   di ';' eliminando gli spazi */
int gest_pv (char **comandi, char *comando){
    char* c = strtok(comando,";");
    int cont_comandi = 0;
    while (c){
        int inizio = 0;
        int fine = strlen(c);
        if (fine == 0) continue; // Comando vuoto
        while(c[inizio] == ' ' && inizio != fine) inizio++;
        c = c + inizio;
        if( inizio == fine) return cont_comandi; // Solo spazi
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
    Logga e stampa le info sul processo

    Streams:
        0 = log_out
        1 = log_err
        2 = stdout
        3 = stderr
*/
void log_process(struct PROCESS p, char* cmd, int cmd_id, int subcmd_id, int* streams) {
    char* buf = malloc(sizeof(char)*BUF_SIZE);
    time_t curtime; time(&curtime);

    //////////////////////   STDOUT   //////////////////////
    // Scrivi intestazioni
    write(streams[0], "\n\n=============================================\n", 48);
    snprintf(buf, BUF_SIZE, "\
        ID:        #%d.%d\n\
        COMMAND:   %s\n\
        DATE:      %s\
        RET CODE:  %d\n\
        OUTPUT:\n\n", cmd_id, subcmd_id, cmd, ctime(&curtime), p.status);
    write(streams[0], buf, strlen(buf));

    // Scrivi lo stdout del proc sugli output richiesti (stdout o stdin di un altro proc)
    write_to(p.stdout, streams[0], streams[2]);


    //////////////////////   STDERR   //////////////////////
    // Scrivi intestazioni
    write(streams[1], "\n\n=============================================\n", 48);
    snprintf(buf, BUF_SIZE, "\
        ID:        #%d.%d\n\
        COMMAND:   %s\n\
        DATE:      %s\
        RET CODE:  %d\n\
        STDERR:\n\n", cmd_id, subcmd_id, cmd, ctime(&curtime), p.status);
    write(streams[1], buf, strlen(buf));

    // Scrivi lo stdout del proc sugli output richiesti (stdout o stdin di un altro proc)
    write_to(p.stderr, streams[1], streams[3]);
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

    struct OPTIONS ret; // Contenitore delle opzioni da ritornare.
    // Lo riempio con i deafult
    strcpy(ret.log_out_path, "log_stdout.txt");
    strcpy(ret.log_err_path, "log_stderr.txt");
    ret.max_size = 5 * 1024 * 1024; // 5MB

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
                ret.max_size = atoi(optarg);
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
                fprintf(stderr, "Error reading options (option code: %c)\n", c);
                break;
        }
    }

    // Se sono rimasti argomenti non riconosciuti avverti l'utente
    for (int i = optind; i < argc; i++)
        fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);

    return ret;
}


/*
    Se cmd è NULL, stampa l'aiuto sull'uso della shell, altrimenti un aiuto su cmd.
    Se cmd è un comando interno stampa l'aiuto scritto, altrimenti chiama "man cmd".
*/
int print_help(char* cmd) {
    if (cmd == NULL)
        printf("\n\n\
                     C u s t o m   S h e l l\n\
                      Operating Systems Lab\n\n\n\
  Usage:\n\
      ./shell <options>\n\n\
  Options:\n\
      -h, --help           Print this message\n\
      -o, --outfile        Set path of the stdout log file\n\
      -e, --errfile        Set path of the err log file\n\
      -m, --maxsize        Set the max size of the log files\n\
      -r, --retcode        Save the exit code of the commands\n\n\n");
    else {
        if (strcmp(cmd, "clear") == 0)
            printf("clear: Clear the screen.\n");
        else if (strcmp(cmd, "exit") == 0)
            printf("exit: Terminate the session and exit the shell.\n");
        else if (strcmp(cmd, "alias") == 0)
            printf("alias <'a'='b'>: List aliases or register a=b.\n");
        else if (strcmp(cmd, "cd") == 0)
            printf("cd <dir>: Change the working directory.\n");
        else if (strcmp(cmd, "history") == 0)
            printf("history <n>: Show the full command history or the last N entries.\n");
        else {
            int pid = fork(), status;
            if (pid < 0)  perror("Cannot call MAN");
            if (pid == 0) exit(execvp("man", (char*[]) {"man", cmd, NULL}));
            else wait(&status);
            return status;
        }
    }
    return 0;
}


/*
    Clear the console.
*/
int clear() {
    printf("\033[H\033[J");
    return 0;
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
