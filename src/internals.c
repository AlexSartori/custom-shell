#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "utils.h"
#include "vector.h"
#include "parsers.h"
#include "internals.h"


/*
    Implementazione base di gestione cicli for basati su contatore
    Es: for i in 5 do echo i = $i done
*/
int do_for(char** args) {
	int a = 0, b = 0, c = 0, lim = 0;
	char *var = args[1];
	char *cmd = (char*) malloc(sizeof(char) * BUF_SIZE);
	char *cmd_parsed = (char*) malloc(sizeof(char) * BUF_SIZE);

	// Viene trovata una variabile
	if(var != NULL) a = 1;

	// Nel caso esista viene azzerata, altrimenti ne creo una
	if(search_var_name(var) != NULL) clear_var(var);
	else {
		char *make = (char*) malloc(sizeof(char) * BUF_SIZE);
		snprintf(make, BUF_SIZE, "var \'%s\' = \'%d'\"", var, 0);
		make_var(make);
	}

	if(args[2] != NULL && strcmp(args[2], "in") == 0) b = 1;

	// Catturo il limite del ciclo for, nel caso sia un intero posso fare casting
	// Nel caso sia una variabile posso sostituirla con il suo valore
	lim = atoi(parse_vars(args[3]));
	if(args[4] != NULL && strcmp(args[4], "do") == 0) c = 1;

	//Se non ho errori di sintassi, catturo il comando da ripetere
	if(a == 1 && b == 1 && c == 1) {
		int argc = 5;
		while(args[argc] != NULL && strcmp(args[argc], "done") != 0) {
			strcat(cmd, args[argc]);
			strcat(cmd, " ");
			argc++;
		}
		strcat(cmd, "\0");

		// Eseguo lim volte il parsing del comando per catturare variabili e l'esecuzione
		// Incremento la variabile della shell associata al contatore
		int start, ret = -1;
		for(start = 0; start < lim; start++) {
			// Gestisco variabili
			cmd_parsed = parse_vars(cmd);
            ret = exec_line(cmd_parsed).status;
            wait(&ret);
            inc_var(var);
		}
		return ret;
	} else {
		printcolor("! Error: invalid syntax.\n", KRED);
		return -1;
	}
}


/*
    Stampa la storia dei comandi eseguiti
*/
int print_history(char *hist_arg) {
	HIST_ENTRY** hist = history_list();
    int n; // Quanti elementi della cronologia mostrare

    // Se non è specificato li mostro tutti
    if (hist_arg == NULL) n = history_length;
    else n = min(atoi(hist_arg), history_length);

    int i;
    for (i = history_length - n; i < history_length; i++)
        printf("  %d\t%s\n", i + history_base, hist[i]->line);

    return 0;
}


/*
    Implementa la gestione del carattere * (funziona bene solo con ls per ora)

    Vengono gestiti casi come prefix*, *suffix, pre*suf

    L'idea è di creare al volo un comando che preveda l'uso di grep e alcune espressioni
    regolari di base
*/
char *expand_wildcard(char *s) {
    char *ns = (char* ) malloc(sizeof(char) * MAXSIZE);
    char *ns1 = (char* ) malloc(sizeof(char) * MAXSIZE);
    char *search = (char* ) malloc(sizeof(char) * MAXSIZE);
    int tmp = 0, found = 0, letters_before = 0, letters_after = 0, i = 0, pos = 0;

    for(i=0;i<MAXSIZE;i++) ns[i] = '\0';

    for(i=0; s[i]; i++) {
        if(s[i] == ' ') tmp = 0;
        else tmp++;

        if(found > 0) letters_after++;

        if(s[i] == '*' && found == 0) {
            letters_before = tmp;
            found++;
        } else if(s[i] == '*' && found > 0) {
            found++;
        }
    }

    //Casi non supportati
    if(found > 2) {
        // ns[0] = '\0';
        printcolor("! Error: not supported\n", KRED);
        return s;
    }

    //Se non trovo il carattere *, restituisco il comando originale
    if(found == 0) {
        free(ns);
        return s;
    }

    //Caso *suffix
    if(found == 1 && letters_before == 1) {
        for(i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                i++;
                pos = 0;
                while(s[i] != ' ' && s[i] != '\0') {
                    search[pos] = s[i];
                    i++;
                    pos++;
                }
                search[pos] = '\0';
                break;
            }
        }
        snprintf(ns1, MAXSIZE, "%s | grep %s$", ns, search);
    }

    //Caso prefix*
    if(found == 1 && letters_before != 1 && letters_after == 0) {
        for (i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                int tmp = i;
                while(ns[tmp] != ' ') tmp--;
                ns[tmp] = '\0';
                i--;
                int pos = letters_before - 2;
                while(s[i] != ' ') {
                    search[pos] = s[i];
                    i--;
                    pos--;
                }
                search[letters_before - 1] = '\0';
                break;
            }
        }
        snprintf(ns1, MAXSIZE, "%s | grep ^%s", ns, search);
    }


    //Caso pre*suf
    if(found == 1 && letters_before != 1 && letters_after != 0) {
        char *search1 = (char* ) malloc(sizeof(char) * MAXSIZE);
        for(i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                int tmp = i;
                int start = i+1;
                while(ns[tmp] != ' ') tmp--;
                ns[tmp] = '\0';
                i--;
                int pos = letters_before - 2;
                while(s[i] != ' ') {
                    search[pos] = s[i];
                    i--;
                    pos--;
                }
                i = start;
                int pos2 = 0;
                while(s[i] != ' ' && s[i] != '\0') {
                    search1[pos2] = s[i];
                    i++;
                    pos2++;
                }
                search1[pos2] = '\0';
                search[letters_before - 1] = '\0';
                break;
            }
        }
        snprintf(ns1, MAXSIZE, "%s | grep ^%s | grep %s$", ns, search, search1);
    }

    //Provo a gestire altri casi usando semplicemente grep
    if(found == 2) {
        for(i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                i++;
                int pos = 0;
                while(s[i] != ' ' && s[i] != '\0' && s[i] != '*') {
                    search[pos] = s[i];
                    i++;
                    pos++;
                }
                search[pos] = '\0';
                break;
            }
        }
        snprintf(ns1, MAXSIZE, "%s | grep %s", ns, search);
    }

    return ns1;
}


/*
    Legge il comando e splitta in corrispondenza di ';' eliminando gli spazi.
*/
char** split_pv (char *comando) {
	// Conta i punti e virgola e alloca un array per contenere i comandi splittati
	int pv = 0, i; for(i = 0; comando[i]; i++) if (comando[i] == ';') pv++;
	char** comandi = malloc(sizeof(char*)*(pv+2)); // Ultimo elemento NULL

	int cont_comandi = 0;
    char* c = strtok(comando, ";");

	// Per ogni token, rimuovi gli spazi e se non è vuoto aggiungilo all'array
    while (c) {
        int inizio = 0, fine = strlen(c);
        if (fine == 0) continue; // Comando vuoto

		// Rimuovo gli spazi
        while (c[inizio] == ' ' && inizio != fine) inizio++;
        c = c + inizio;
        if (inizio == fine) { // Solo spazi
			c = strtok(NULL,";");
         	continue;
	    }

        inizio = 0;
        fine = strlen(c) - 1;
        while (c[fine] == ' ' && fine != inizio) fine--;
        if (fine != inizio) c[fine + 1] = '\0';

        comandi[cont_comandi] = c;
        c = strtok(NULL,";");
        cont_comandi++;
    }

	comandi[cont_comandi] = NULL;
    return comandi;
}


/*
    Implementa gestione dei comandi del tipo "cmd1 && cmd2 && cmd3".
*/
int gest_and(char* c) {
    int i = 0, br = 0, length = strlen(c);
    char tmp[length];

	// Se trovo && eseguo fino a lì e ripeto. Se un comando fallisce ritorno un errore.
    while (i < length && length > 0) {
		strcpy(tmp, c);
        if (i != length - 1 && c[i] == '&' && c[i+1] == '&') {
            tmp[i] = '\0';
            struct PROCESS p1 = exec_line(tmp);

            if (p1.status != 0 ) {
				// Comando fallito
                br = -1;
                break;
            } else {
				// Prossimo comando
                cmd_id++;
                c = c+i+2;
                br = br+i+2;
                length = strlen(c);
                i = -1;
            }
        }
    	i++;
    }
	return br;
}


/*
	Controlla gli operatori <, >, 2>, &>, >>.
*/
int redirect(char* c, struct PROCESS *ret_p) {
    int flag = 0; // Se ci sono > o < l'esecuzione è gestita nella funzione, non nel main
    int permissions = O_RDWR; // Append, normal
    int red[3], n[3];
    int len = strlen(c);
    char new_error[len], new_input[len], new_output[len], cmd[len];
    int cmd_saved = 0, i = 0;
	red[0] = red[1] = red[2] = 0; // Di default non reindirizzare nessun canale

    while (i < len) {
        if (c[i] == '>' || c[i] == '<') { // Output, error o input
			// Cerca il nome del file.
            int file_start = i + 1;
            if (c[file_start] == '>'){ // Append
                file_start++;
                permissions |= O_CREAT | O_APPEND;
            } else { // Normal
                permissions |= O_CREAT | O_TRUNC;
            }

			// Rimuovi gli spazi
            while (c[file_start] == ' ') file_start++;
            int file_end = file_start;
            while(c[file_end] != ' ' && c[file_end] != '\0') file_end++;
            char new_file[len - file_start + 3];
            strcpy(new_file,"./");
            strcat(new_file, c + file_start);
            new_file[file_end + 2 - file_start] = '\0';

			// Apri il file e salva quali canali redirezionare
            if (c[i] == '<') { // Input
				red[0] = 1;
				strcpy(new_input, new_file);
				n[0] = open(new_input, O_RDWR | O_APPEND);
				if (n[0] < 0) { printf("Cannot use %s\n", new_input); return 1; }
            } else if (c[i-1] == '2'){ // Error
                strcpy(new_error, new_file);
                i--;
                red[2] = 1;
                n[2] = open(new_error, permissions, 0644);
            } else { // Output
                strcpy(new_output, new_file);
                red[1] = 1;
                n[1] = open(new_output, permissions, 0644);
            }

            if (!cmd_saved) {
                flag = 1; // L'esecuzione è stata gestita nella funzione
                cmd_id++;
                // Salvo il comando da eseguire
                strcpy(cmd, c);
                cmd[i] = '\0';
                cmd_saved = 1;
            }

            i = file_end;
        } else i++; // Nessuno dei precedenti
    }

    if (flag) {
		// Esegui il processo e gestisci i canali redirezionati
        struct PROCESS p = exec_cmd(cmd);
		ret_p->stdout = p.stdout;
		ret_p->stderr = p.stderr;
		ret_p->stdin = p.stdin;

		if (red[0] == 1) {
		   write_to(n[0], p.stdin, open("/dev/null", O_RDWR));
		   close(p.stdin);
		}
		if (red[1] == 1) {
		   write_to(p.stdout, n[1], open("/dev/null", O_RDWR));
		   close(p.stdout);
		}
		if (red[2] == 1) {
		   write_to(p.stderr, n[2], open("/dev/null", O_RDWR));
		   close(p.stdin);
		}
		wait(&(ret_p->status));
    }
	return flag;
}
