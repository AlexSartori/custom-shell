/*
    Implementazione di un array dinamico, viene usato per gestire variabili e alias
    Raggiunta la capacità massima, viene raddoppiata in modo da contenere altri elementi
*/

#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_INIT_CAPACITY 4

typedef struct vector {
    void **items;
    int capacity;
    int total;
} vector;

typedef struct elemento {
    char* name;
    char* data;
} elemento;

void vector_init(vector *);
int vector_total(vector *);
/* static */ void vector_resize(vector *, int);
void vector_add(vector *, void *);
void vector_set(vector *, int, void *);
void *vector_get(vector *, int);
void vector_delete(vector *, int);
void vector_free(vector *);

#endif
