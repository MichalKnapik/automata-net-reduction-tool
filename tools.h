#ifndef TOOLS_H
#define TOOLS_H

/* The initial size of growing/synchro_array. */
#define AINITSIZE 1000

#include "common.h"

/* A growing array. */
typedef struct {
  void** actions;
  int ctr;
  int capacity;
} growable_array, *growable_array_ptr, synchro_array, *synchro_array_ptr;

synchro_array_ptr get_growing_array(void);

synchro_array_ptr get_synchro_array(void);

synchro_array_ptr read_synchro_array(char* fname);

void free_synchro_array_shallow(synchro_array_ptr sarr); 

/* Calls free for each elements of sarr. */
void free_synchro_array(synchro_array_ptr sarr); 

void* grow_ref_array(int* capacity, void** arr);

void* insert_and_grow_maybe(growable_array_ptr* arr, void* elt);

bool contains_ref_array(void** arr, int size, void* elt);

bool cstring_array_contains(char** arr, int size, char* elt);

#endif
