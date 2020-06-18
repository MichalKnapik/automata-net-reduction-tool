#ifndef AUTOMATA_INTERFACE_H
#define AUTOMATA_INTERFACE_H

#include "common.h"
#include "tools.h"

//careful with this; added to suppress warnings
void yyerror(char* s);
int yylex(void);
void yyrestart(FILE *input_file);

/************** Data structures **************/

struct TRANSITION;
struct sync_link;

typedef char auto_flags;
#define AUTOM_NONE 0x0
#define AUTOM_INCIDENCE_OK 0x1
#define AUTOM_MARKED 0x2

/* Auxiliary: a helper in parsing. */
typedef struct TR {
  char* source;
  char* name;
  char* target;
  struct TR* next;
} transition_record, *transition_record_ptr;

/* An unidirectional list of states in an automaton. The outgoing and
   incoming transitions lists are filled with collect_incidence_lists(.). */
typedef struct STATE {
  char* name;
  bool marked;
  struct STATE* next;
  struct TRANSITION* outgoing;
  struct TRANSITION* incoming;
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
     clear it using free_work_links(.) and copy sync_links
     to work_links using copy_work_links(.). The depth of copying
     is as follows:
     - other: reference is copied and needs no cleanup;
     - sync_action_names: copied via memcpy so needs to be freed;
     - next: not copied, created with the new list. */
  struct sync_link* work_links;

  /* This turned out to be quite important: everything above is
     ultimately created from transition_records. So maintain the
     consistency between transition_records and other data
     structures. */
  transition_record_ptr transition_records;

} automaton, *automaton_ptr;

/* A bidirectional list of synchronisation links. Array sync_action_names
   connects the struct's owner with other via sync_action_names. */
typedef struct sync_link {
  automaton_ptr other;
  char** sync_action_names;
  int sync_action_ctr;
  int sync_action_capacity;
  struct sync_link* next;
  struct sync_link* prev;
} sync_link, *sync_link_ptr;

/*********************** Automata/network functions ***********************/

/* Everything gets set to NULL but flags set to AUTOM_NONE. */
automaton_ptr get_fresh_automaton(void);

/* A shallow copy of sptr. Needed for square products. Does not
   copy outgoing transitions and sets them to NULL. */
state_ptr copy_state_list(state_ptr sptr);

/* The string stname is not copied via strdup. */
state_ptr make_state(char* stname);

void add_state(automaton_ptr aut, char* stname);

/* For now, (aut->states)[0] is the init state ptr. */
state_ptr get_initial_state(automaton_ptr aut);

bool is_state_initial(automaton_ptr aut, state_ptr state);

/* Checks if act_name is a local action of aut, i.e., none of automata
 connected via work_links knows act_name or it is not in sarr. */
bool is_action_local(automaton_ptr aut, char* act_name, synchro_array_ptr sarr);

/* Nothing here is copied via strdup. */
transition_record_ptr make_transition_record(char* src_name, char* act_name, char* target_name);

void add_transition_record(automaton_ptr aut, transition_record_ptr tr);

void free_automaton(automaton_ptr aut);

state_ptr get_state_by_name(automaton_ptr aut, char* state_name);

/* Returns state_name preceded by the pointer aut and "::". */
char* get_qualified_state_name(automaton_ptr aut, char* state_name);

/* Returns a concatenation of qualified_state_names with "--". */
char* get_qualified_pair_name(automaton_ptr auta, char* state_namea,
                              automaton_ptr autb, char* state_nameb);

/* Fills the incidence lists of each state of aut. Fills both outgoing
   and incoming lists. */
bool collect_incidence_lists(automaton_ptr aut);

/* Connects an automaton to the network of automata.
   The synchro_array_ptr sarr is an array of actions (see tools.h)
   s.t. two automata can synchronise over a in sarr if they both know
   a. If sarr is NULL then automata synchronise over any action with
   common labels. */
void add_automaton_to_network(automaton_ptr net, automaton_ptr new_automaton, synchro_array_ptr sarr);

/* For memory management: all parameters are strdup-ed. */
transition_record_ptr make_transition_record(char* source, char* name, char* target);

/* The string trname is not copied via strdup. */
transition_ptr make_transition(char* trname);

/* Returns true iff aut has registered trans_name as an action label and (trans_name is in sarr
or sarr is NULL. */
bool automaton_knows_transition(automaton_ptr aut, char* trans_name, synchro_array_ptr sarr);

/* Returns an array of references to the states of aut labeled with trans_name.
   Memory management: clear only the array, don't free the states. */
state_ptr* get_states_with_enabled(automaton_ptr aut, char* trans_name, int* asize);

void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd, synchro_array_ptr sarr);

void sync_automata(automaton_ptr fst, automaton_ptr snd, synchro_array_ptr sarr);

void copy_work_links(automaton_ptr aut);

void copy_work_links_network(automaton_ptr net);

void free_sync_links(automaton_ptr aut);

void free_work_links(automaton_ptr aut);

//-------- tools for marking states in automata/networks --------

void mark_state(state_ptr spt);

bool is_state_marked(state_ptr spt);

void clear_state(state_ptr spt);

/* Unmarks all the states of aut. */
void clear_all_states(automaton_ptr aut);

/* Unmarks all the states of each automaton of net. */
void clear_all_states_in_network(automaton_ptr net);

/* Given an automaton aut marks those states from which a state
   already marked is reachable. Recursive. (TODO)*/
void mark_reachable_marked(automaton_ptr aut);

/* Returns a fresh automaton that contains only the
   (copies) of the marked states of aut and transitions. */
automaton_ptr remove_unmarked_states(automaton_ptr aut);

/* Marks in aut those states where an action with label known to
   root is executable. Used in pruning of square products. */
void mark_states_with_root_active_actions(automaton_ptr root, automaton_ptr aut);

//----- tools for marking automata in networks/topologies ------

void mark_automaton(automaton_ptr aut);

void clear_automaton(automaton_ptr aut);

bool is_automaton_marked(automaton_ptr aut);

/* Removes markings from all the automata in the net. */
void clear_network(automaton_ptr net);

//--------------------------------------------------------------

void display_automaton(automaton_ptr aut);

void display_network(automaton_ptr aut);

typedef enum {MAIN_SYNCHRO_LINKS, WORKER_SYNCHRO_LINKS} SL_CHOICE;
void print_synchro_links(automaton_ptr aut, SL_CHOICE ptype);

void print_incidence_list(automaton_ptr aut);

automaton_ptr read_automaton(char* fname);

void automaton_to_dot(automaton_ptr aut, int automaton_ctr, FILE* dotf);

bool network_to_dot(automaton_ptr net, char* dotfname);

bool working_topology_to_dot(automaton_ptr net, char* dotfname);

#endif
