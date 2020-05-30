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
  struct TRANSITION* outgoing;
} state, *state_ptr;

typedef struct TRANSITION {
  state_ptr source;
  char* name;
  state_ptr target;
  struct TRANSITION* next;
  struct TRANSITION* prev;  
} transition, *transition_ptr;

typedef char auto_flags;
#define AUTOM_NONE 0x0
#define AUTOM_INCIDENCE_OK 0x1

typedef struct {
  auto_flags flags;
  state_ptr states;
  parsed_transition_ptr parsed_transitions;
} automaton, *automaton_ptr;

typedef struct {
  char** actions;
  int ctr;
  int capacity;
} synchro_array, *synchro_array_ptr;

/* typedef struct {//todo */
  
/* } topology, *topology_ptr; */

/* Przemyślenia:

Może warto dodać możliwość markowania automatów w topologii
i, podobnie, markowanie stanów w automacie. Czyli:

void mark_automaton(automaton_ptr aut);
void clear_topology(automaton_ptr aut); //clears all automata in topology
void clear_topology(topology_ptr top);

void mark_state(stateptr spt);
void clear_state(stateptr spt);
void clear_automaton(automaton_ptr aut); //clears all states of automaton

 */

void free_automaton(automaton_ptr aut);
void display_automaton(automaton_ptr aut);
state_ptr get_state_by_name(automaton_ptr aut, char* state_name);
bool collect_incidence_lists(automaton_ptr aut);

automaton_ptr read_automaton(char* fname);

synchro_array_ptr read_synchro_array(char* fname);
void free_synchro_array(synchro_array_ptr sarr); 

#endif
