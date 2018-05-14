#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <readline/history.h>

#include "utils.h"
#include "vector.h"
#include "internals.h"


vector vector_alias;

void vector_alias_initializer() {
    vector_init(&vector_alias);
}

char* parse_alias(char* comando) {
    char *init = (char*)malloc(sizeof(char)*(strlen(comando)));
    strcpy(init, comando);
    char* token = strtok(comando, " ");
    char *ns;
    int ok = 0;
    for(int i=0;i<vector_total(&vector_alias);i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_alias, i);

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

void list_alias() {
    elemento* tmp;
    for(int i=0;i<vector_total(&vector_alias);i++) {
        tmp = (elemento*)vector_get(&vector_alias, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
}

int search_alias(elemento* el) {
    for(int i=0;i<vector_total(&vector_alias);i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_alias, i);

        if(strcmp(el->name, tmp->name) == 0) {
            tmp -> data = el -> data;
            return 1;
        }

    }

    return 0;
}


void make_alias(char *copy_line) {
	char *alias = (char*)malloc(sizeof(char) * strlen(copy_line));
    char *content = (char*)malloc(sizeof(char) * strlen(copy_line));
    int active = 0;
    int first = 1;
    int alias_index = 0;
    int content_index = 0;
    for(int i=0;copy_line[i]!='\0';i++) {
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
        if (search_alias(insert) == 1) {
            printf("Warning: alias \'%s\' has been overwritten\n", alias);
        } else {
            vector_add(&vector_alias, insert);
        }
    }
}

void print_history(char *hist_arg) {
	HIST_ENTRY** hist = history_list();
    int n; // Quanti elementi della cronologia mostrare

    // Se non è specificato li mostro tutti
    if (hist_arg == NULL) n = history_length;
    else n = min(atoi(hist_arg), history_length);

    for (int i = history_length - n; i < history_length; i++)
        printf("  %d\t%s\n", i + history_base, hist[i]->line);
}
