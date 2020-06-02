#ifndef TOOLS_H
#define TOOLS_H
#include "common.h"

void* grow_ref_array(int* capacity, int size_of_type, void** arr);

bool contains_ref_array(void** arr, int size, void* elt);

#endif
