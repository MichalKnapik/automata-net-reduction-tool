#ifndef AUTOMATA_INTERFACE_H
#define AUTOMATA_INTERFACE_H

//careful with this; added to suppress warnings
void yyerror(char* s);
int yylex(void);
void yyrestart(FILE *input_file);

#include "common.h"

struct TRANSITION;

/* Auxiliary: a helper in parsing. */
typedef struct TR {
  char* source;
  char* name;
  char* target;
  struct TR* next;
} parsed_transition, *parsed_transition_ptr;

/* An unidirectional list of states in an automaton. The outgoing 
   transitions list is filled with collect_incidence_lists(.) call. */
typedef struct STATE {
  char* name;
  struct STATE* next;
  struct TRANSITION* outgoing;
} state, *state_ptr;

/* A bidirectional list of all the transitions in an automaton. */
typedef struct TRANSITION {
  state_ptr source;
  char* name;
  state_ptr target;
  struct TRANSITION* next;
  struct TRANSITION* prev;  
} transition, *transition_ptr;

/* An array of actions over which automata synchronise. */
typedef struct {
  char** actions;
  int ctr;
  int capacity;
} synchro_array, *synchro_array_ptr;

struct sync_link;

typedef char auto_flags;
#define AUTOM_NONE 0x0
#define AUTOM_INCIDENCE_OK 0x1

/* A bidirectional list of all the automata in a network. */
typedef struct AUTOMATON {

  auto_flags flags; //see defns below auto_flags typedef
  state_ptr states; //call collect_incidence_lists(.) to build transitions

  //navigate in the network
  struct AUTOMATON* next; 
  struct AUTOMATON* prev; 

  //an array of synchronisation links with other automata
  //(TODO)
  struct sync_link* sync_links;

  parsed_transition_ptr parsed_transitions;  

} automaton, *automaton_ptr;

/* A list of synchronisation links. Array sync_action_names 
   connects the struct's owner with other via sync_action_names. */
//(TODO)
typedef struct sync_link {
  automaton_ptr other; 
  char** sync_action_names;
  int sync_action_ctr;
  int sync_action_capacity;  
  struct sync_link* next;
} sync_link, *sync_link_ptr;

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
void display_network(automaton_ptr aut);
state_ptr get_state_by_name(automaton_ptr aut, char* state_name);
bool collect_incidence_lists(automaton_ptr aut);

bool automaton_knows_transition(automaton_ptr aut, char* trans_name);

/* Connects an automaton to the network of automata. */ 
void add_automaton_to_network(automaton_ptr net, automaton_ptr new_automaton);
void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd);
void sync_automata(automaton_ptr fst, automaton_ptr snd);

automaton_ptr read_automaton(char* fname);

synchro_array_ptr read_synchro_array(char* fname);
void free_synchro_array(synchro_array_ptr sarr); 

#endif
