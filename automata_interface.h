#ifndef AUTOMATA_INTERFACE_H
#define AUTOMATA_INTERFACE_H

//careful with this; added to suppress warnings
void yyerror(char* s);
int yylex(void);
void yyrestart(FILE *input_file);

#include "common.h"

struct TRANSITION;

typedef struct TR {
  char* source;
  char* name;
  char* target;
  struct TR* next;
} parsed_transition, *parsed_transition_ptr;

typedef struct STATE {
  char* name;
  struct STATE* next;
  struct TRANSITION* outgoing; //TODO
} state, *state_ptr;

typedef struct TRANSITION {
  state_ptr source;
  char* name;
  state_ptr target;
  struct TRANSITION* next;
  struct TRANSITION* prev;  
} transition, *transition_ptr;

typedef struct {
  state_ptr states;
  parsed_transition_ptr parsed_transitions;
} automaton, *automaton_ptr;

void free_automaton(automaton_ptr aut); //add freeing outgoing
void display_automaton(automaton_ptr aut);
state_ptr get_state_by_name(automaton_ptr aut, char* state_name);
bool collect_incidence_lists(automaton_ptr aut);

#endif
