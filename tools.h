#ifndef TOOLS_H
#define TOOLS_H
#include "common.h"

/* An array of actions over which automata synchronise. */
typedef struct {
  char** actions;
  int ctr;
  int capacity;
} synchro_array, *synchro_array_ptr;

synchro_array_ptr read_synchro_array(char* fname);

void free_synchro_array(synchro_array_ptr sarr); 

void* grow_ref_array(int* capacity, int size_of_type, void** arr);

bool cstring_array_contains(char** arr, int size, char* elt);

#endif
