#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/vector.h"
#include "../headers/utils.h"

vector vector_alias;
vector vector_vars;


/*
    Inizializza i vector per alias e variabili
*/
void vectors_initializer() {
    vector_init(&vector_alias);
    vector_init(&vector_vars);
}


/*
    Se in comando è presente un alias lo sostituisce e ritorna una nuova stringa
*/
char* parse_alias(char* comando) {
    char *init = (char*)malloc(sizeof(char)*(strlen(comando)));
    strcpy(init, comando);
    char *token = strtok(comando, " "), *ns;

    int ok = 0, i;
    for(i = 0; i < vector_total(&vector_alias); i++) {
        elemento* tmp = (elemento*)vector_get(&vector_alias, i);

        if(strcmp(token, tmp->name) == 0) {
            ok = 1;
            ns = (char*)malloc(sizeof(char)*(strlen(comando) + strlen(tmp->data)));
            strcpy(ns, tmp->data);
            token = strtok(NULL, " ");
            while(token) {
                strcat(ns, " ");
                strcat(ns, token);
                token = strtok(NULL, " ");
            }
            strcat(ns, "\0");
            break;
        }

    }

    if(ok == 0) return init;
    else return ns;
}


/*
    Stampa tutti gli alias in memoria
*/
int list_alias() {
    elemento* tmp;
    int i;
    for(i = 0; i < vector_total(&vector_alias); i++) {
        tmp = (elemento*)vector_get(&vector_alias, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
    return 0;
}


/*
    Cerca un alias e ne ritorna l'indice nel vettore o -1.
*/
int search_alias(elemento* el) {
    int i;
    for(i = 0; i < vector_total(&vector_alias); i++) {
        elemento* tmp = (elemento*)vector_get(&vector_alias, i);
        if(strcmp(el->name, tmp->name) == 0) return i;
    }

    return -1;
}


/*
    Crea un alias
*/
int make_alias(char *copy_line) {
	char *alias = (char*)malloc(sizeof(char) * strlen(copy_line));
    char *content = (char*)malloc(sizeof(char) * strlen(copy_line));
    int active = 0, first = 1, alias_index = 0, content_index = 0, i;

    // Metti in alias e content il nome dell'alias e la stringa corrispettiva.
    for(i = 0; copy_line[i] != '\0'; i++) {
        if(active == 1 && first == 1) alias[alias_index++] = copy_line[i];
        if(active == 1 && first == 0) content[content_index++] = copy_line[i];
        if(copy_line[i] == '\'' && active == 0) active = 1;
        else if(copy_line[i] == '\'' && active == 1) {
            active = 0;
            first = 0;
        }
    }
    alias[alias_index-1] = '\0';
    content[content_index-1] = '\0';

    if(strlen(alias) == 0 || strlen(content) == 0) {
        printcolor("! Error: format \'alias\'=\'command\'\n", KRED);
    } else {
        elemento *insert = (elemento*)malloc(sizeof(elemento));
        insert -> name = alias;
        insert -> data = content;
        if (search_alias(insert) != -1) printcolor("! Error: this alias already exists.", KRED);
        else vector_add(&vector_alias, insert);
    }

    return 0;
}


/*
    Stampa tutte le variabili in memoria
*/
int list_vars() {
    elemento* tmp;
    int i;
    for(i = 0; i < vector_total(&vector_vars); i++) {
        tmp = (elemento*)vector_get(&vector_vars, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
    return 0;
}


/*
int search_var_elemento(elemento* el) {
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(el->name, tmp->name) == 0) {
            tmp -> data = el -> data;
            return 1;
        }

    }

    return 0;
}
*/


/*
    Cerca una variable e ritorna il contenuto
*/
char* search_var_name(char *name) {
    int i;
    for(i = 0; i < vector_total(&vector_vars); i++) {
        elemento* tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0)
            return tmp -> data;
    }
    return NULL;
}



/*
    Sostituisce le variabili presenti in comando e restituisce una nuova stringa
*/
char* parse_vars(char *comando) {
    char *c = (char*)malloc(sizeof(char) * strlen(comando)*2);
    char *var = (char*)malloc(sizeof(char) * strlen(comando));
    int j = 0, i = 0, z = 0, k = 0;
    char* data;

    for(i = 0; comando[i]; i++) {
        if(comando[i] != '$') {
            c[j] = comando[i];
            j++;
        } else {
            z = 0;
            i++;
            while(comando[i] != '\0' && comando[i] != ' ' && comando[i] != '$') {
                var[z] = comando[i];
                i++;
                z++;
            }
            var[z] = '\0';
            //printf("FOUND VAR: %s\n", var);
            data = search_var_name(var);

            if(data == NULL) {
                printcolor("! Error: variable not found\n", KRED);
                return "";
            }
            //printf("FOUND DATA: %s\n", data);
            for(k = 0; data[k]; k++) {
                c[j] = data[k];
                j++;
            }
            i--;
        }
    }
    c[j] = '\0';
    free(var);
    //printf("%s\n", c);
    return c;
}


/*
    Crea una nuova coppia nome_variabile -> valore
*/
int make_var(char *copy_line) {
    char *var = (char*)malloc(sizeof(char) * strlen(copy_line));
    char *content = (char*)malloc(sizeof(char) * strlen(copy_line));
    int active = 0, first = 1, var_index = 0, content_index = 0, i;

    for(i=0; copy_line[i]!='\0'; i++) {
        if(active == 1 && first == 1) var[var_index++] = copy_line[i];
        if(active == 1 && first == 0) content[content_index++] = copy_line[i];
        if(copy_line[i] == '\'' && active == 0) active = 1;
        else if(copy_line[i] == '\'' && active == 1) {
            active = 0;
            first = 0;
        }
    }
    var[var_index-1] = '\0';
    content[content_index-1] = '\0';

    if(strlen(var) == 0 || strlen(content) == 0) {
        printcolor("! Error: format \'var\'=\'content\'\n", KRED);
    } else {
        elemento *insert = (elemento*)malloc(sizeof(elemento));
        insert -> name = var;
        insert -> data = content;
        if (search_var_name(insert->name) != NULL) {
            printcolor("! Error: variable already exists.\n", KRED);
        } else {
            vector_add(&vector_vars, insert);
        }
    }

    return 0;
}


/*
    Incrementa una variabile
*/
void inc_var(char *name) {
    int i;
    for(i = 0; i < vector_total(&vector_vars); i++) {
        elemento* tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0)
            snprintf(tmp->data, 100, "%d", atoi(tmp -> data) + 1);
    }
}

/*
    Azzera una variabile
*/
void azz_var(char *name) {
    int i;
    for(i = 0; i < vector_total(&vector_vars); i++) {
        elemento* tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0)
            snprintf(tmp->data, 100, "%d", 0);
    }
}
