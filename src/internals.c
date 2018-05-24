#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/history.h>

#include "../headers/utils.h"
#include "../headers/vector.h"
#include "../headers/internals.h"


int print_history(char *hist_arg) {
    /*
	HIST_ENTRY** hist = history_list();
    int n; // Quanti elementi della cronologia mostrare

    // Se non è specificato li mostro tutti
    if (hist_arg == NULL) n = history_length;
    else n = min(atoi(hist_arg), history_length);

    int i;
    for (i = history_length - n; i < history_length; i++)
        printf("  %d\t%s\n", i + history_base, hist[i]->line);

    return 0;
    */

}

char *expand_wildcar(char *s) {
    char *ns = (char* ) malloc(sizeof(char) * 1024);
    char *ns1 = (char* ) malloc(sizeof(char) * 1024);
    char *search = (char* ) malloc(sizeof(char) * 1024);
    int tmp = 0, found = 0, letters_before = 0, letters_after = 0, i = 0, pos = 0;

    for(i=0;i<1024;i++) ns[i] = '\0';


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



    if(found > 2) {
        ns[0] = '\0';
        printf("ERROR\n");
        return ns;
    }

    if(found == 0) {
        free(ns);
        return s;
    }

    //printf("found: %d letters_before: %d\n", found, letters_before);

    if(found == 1 && letters_before == 1) {
        //int i;
        for(i=0; s[i]; i++) {
            //printf("%c\n", s[i]);
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
        //printf("%s\n", ns);
        //ns[i-1] = '\0';
        snprintf(ns1, 1024, "%s | grep %s$", ns, search);
        //printf("%s\n", ns1);
    }

    if(found == 1 && letters_before != 1 && letters_after == 0) {
        //int i;
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
        snprintf(ns1, 1024, "%s | grep ^%s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 2) {
        //int i;
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
        snprintf(ns1, 1024, "%s | grep %s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 1 && letters_before != 1 && letters_after != 0) {
        char *search1 = (char* ) malloc(sizeof(char) * 1024);
        //int i;
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
        snprintf(ns1, 1024, "%s | grep ^%s | grep %s$", ns, search, search1);
        //printf("%s\n", ns);
    }
    //printf("%s\n", ns);
    //printf("%s\n", ns1);
    return ns1;
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
        if( inizio == fine){ // Solo spazi
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

//
int gest_and(char* c, int* cmd_id, int subcmd_id, int log_out, int log_err) {
    int i = 0;
    int br = 0;
    int length= strlen(c);
    char tmp[length];

    while (i < length && length > 0){
        strcpy(tmp,c);
        if (c[i] == '&' && c[i+1] == '&' && i!= length - 1) {
            tmp[i] = '\0';
            struct PROCESS p1 = exec_line(tmp, *(cmd_id), &subcmd_id, log_out, log_err);

            if( p1.status != 0 ){
                printcolor("! Error: One of the command failed.\n", KRED);
                br = -1;
                break;
            } else {
                (*(cmd_id))++;
                c = c+i+2;
                br = br+ i+2;
                length = strlen(c);
                i = -1 ;
            }
        }
    i++;
    }
return br;
}
