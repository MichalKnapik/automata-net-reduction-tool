#ifndef AUTOMATA_INTERFACE_H
#define AUTOMATA_INTERFACE_H

//careful with this; added to suppress warnings
void yyerror(char* s);
int yylex(void);
void yyrestart(FILE *input_file);

#include "common.h"

typedef struct TR {
  char* source;
  char* name;
  char* target;
  struct TR* next;
} parsed_transition, *parsed_transition_ptr;

typedef struct STATE {
  char* name;
  struct STATE* next;
  parsed_transition_ptr outgoing; //TODO
} state, *state_ptr;

typedef struct {
  state_ptr states;
  parsed_transition_ptr parsed_transitions;
} automaton, *automaton_ptr;

void free_automaton(automaton_ptr aut);
void display_automaton(automaton_ptr aut);

#endif
