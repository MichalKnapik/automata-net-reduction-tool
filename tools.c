#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tools.h"
#include "common.h"

synchro_array_ptr read_synchro_array(char* fname) {

  FILE* fin;
  if ((fin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading synchronising actions");
     exit(1);
  }

  char buffer[MAXTOKENLENGTH];

  int AINITSIZE = 1000;
  synchro_array_ptr arr = (synchro_array_ptr) malloc(sizeof(synchro_array));
  arr->capacity = AINITSIZE;
  arr->ctr = 0;
  arr->actions = (char**) malloc(arr->capacity * sizeof(char*));

  while (fgets(buffer, MAXTOKENLENGTH, fin)) {

    if (strlen(buffer) >= 1) buffer[strlen(buffer)-1] = '\0';
    if (arr->ctr == arr->capacity - 1) 
      grow_ref_array(&arr->capacity, sizeof(char*), (void**) &arr->actions);

    arr->actions[(arr->ctr)++] = strndup(buffer, MAXTOKENLENGTH);
  }

  if (!fin) fclose(fin);
  return arr;
}

void free_synchro_array(synchro_array_ptr sarr) {

  while(--(sarr->ctr) >= 0) free(sarr->actions[sarr->ctr]);
  free(sarr->actions);
  free(sarr);

}

void* grow_ref_array(int* capacity, int size_of_type, void** arr) {
  //maybe move to realloc?
  void* new_act_array = malloc( 2 * (*capacity) * size_of_type);
  if (new_act_array == NULL) {
    perror("array growth error");
    exit(1);
  }

  memcpy(new_act_array, *arr, size_of_type * (*capacity));
  *capacity = 2 * (*capacity);
  free(*arr);
  *arr = new_act_array;

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
