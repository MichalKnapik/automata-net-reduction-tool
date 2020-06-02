#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tools.h"
#include "common.h"

void* grow_ref_array(int* capacity, int size_of_type, void** arr) {

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
  for (int i = 0; i < size; ++i) if (arr[i] == elt) return true;
  return false;
}
