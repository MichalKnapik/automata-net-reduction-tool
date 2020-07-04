#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tools.h"
#include "common.h"

synchro_array_ptr get_growing_array(void) {

  synchro_array_ptr arr = (synchro_array_ptr) malloc(sizeof(synchro_array));
  arr->capacity = AINITSIZE;
  arr->ctr = 0;
  arr->actions = malloc(arr->capacity * sizeof(void*));

  return arr;
}

synchro_array_ptr get_synchro_array(void) {
  return get_growing_array();
}

synchro_array_ptr read_synchro_array(char* fname) {

  FILE* fin;
  if ((fin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading synchronising actions");
     exit(1);
  }

  char buffer[MAXTOKENLENGTH];

  synchro_array_ptr arr = get_synchro_array();

  while (fgets(buffer, MAXTOKENLENGTH, fin)) {

    if (strlen(buffer) >= 1) buffer[strlen(buffer)-1] = '\0';
    if (arr->ctr == arr->capacity - 1) 
      grow_ref_array(&arr->capacity, (void**) &arr->actions);

    arr->actions[(arr->ctr)++] = strdup(buffer);
  }

  if (!fin) fclose(fin);
  return arr;
}

void free_synchro_array_shallow(synchro_array_ptr sarr) {
  free(sarr->actions);
  free(sarr);
}

void free_synchro_array(synchro_array_ptr sarr) {
  while(--(sarr->ctr) >= 0) free(sarr->actions[sarr->ctr]);
  free_synchro_array_shallow(sarr);
}

void* grow_ref_array(int* capacity, void** arr) {

  void* new_act_array = malloc( 2 * (*capacity)  * sizeof(void*) );
  if (new_act_array == NULL) {
    perror("array growth error");
    exit(1);
  }

  memcpy(new_act_array, *arr, (*capacity) * sizeof(void*));
  *capacity = 2 * (*capacity);
  free(*arr);
  *arr = new_act_array;

  return arr;
}

void* insert_and_grow_maybe(growable_array_ptr* arr, void* elt) {

  growable_array_ptr arrp = *arr;
  if (arrp->ctr >= arrp->capacity) grow_ref_array(&arrp->capacity, (void**) arr);
  arrp->actions[(arrp->ctr)++] = elt;

  return arr;
}

bool contains_ref_array(void** arr, int size, void* elt) {

  for (int i = 0; i < size; ++i) 
      if (arr[i] == elt) return true;

  return false;
}

bool cstring_array_contains(char** arr, int size, char* elt) {

  for (int i = 0; i < size; ++i) 
    if (!strcmp(arr[i], elt)) return true;

  return false;
}
