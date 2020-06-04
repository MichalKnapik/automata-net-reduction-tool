#ifndef AUTOMATA_INTERFACE_H
#define AUTOMATA_INTERFACE_H

#include "common.h"

struct TRANSITION;
struct sync_link;

typedef char auto_flags;
#define AUTOM_NONE 0x0
#define AUTOM_INCIDENCE_OK 0x1

//careful with this; added to suppress warnings
void yyerror(char* s);
int yylex(void);
void yyrestart(FILE *input_file);

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

/* A bidirectional list of all the automata in a network. */
typedef struct AUTOMATON {

  auto_flags flags; //see defns below auto_flags typedef
  state_ptr states; //call collect_incidence_lists(.) to build transitions

  //navigate in the network
  struct AUTOMATON* next; 
  struct AUTOMATON* prev; 

  //an array of synchronisation links with other automata
  struct sync_link* sync_links;

  /* a copy of the above, to be used in exploration algorithms
     clear it using clear_work_links(.) and copy sync_links
     to work_links using copy_work_links(.). The depth of copying
     is as follows:
     - other: reference is copied and needs no cleanup;
     - sync_action_names: copied via memcpy so needs to be freed;
     - next: not copied, created with the new list.
  */
  struct sync_link* work_links; 

  parsed_transition_ptr parsed_transitions;  

} automaton, *automaton_ptr;

/* A list of synchronisation links. Array sync_action_names 
   connects the struct's owner with other via sync_action_names. */
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

typedef enum {MAIN_SYNCHRO_LINKS, WORKER_SYNCHRO_LINKS} SL_CHOICE;
void print_synchro_links(automaton_ptr aut, SL_CHOICE ptype);

void print_incidence_list(automaton_ptr aut);

state_ptr get_state_by_name(automaton_ptr aut, char* state_name);

/* Fills the incidence list of each state of aut. */
bool collect_incidence_lists(automaton_ptr aut);

/* Connects an automaton to the network of automata. */ 
void add_automaton_to_network(automaton_ptr net, automaton_ptr new_automaton);

bool automaton_knows_transition(automaton_ptr aut, char* trans_name);

void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd);

void sync_automata(automaton_ptr fst, automaton_ptr snd);

void clear_sync_links(automaton_ptr aut);

void clear_work_links(automaton_ptr aut);

void copy_work_links(automaton_ptr aut);

automaton_ptr read_automaton(char* fname);

bool automaton_to_dot(automaton_ptr aut, char* dotfname);

#endif
