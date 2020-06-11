#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "automata_interface.h"
#include "tools.h"
#include "tree_topology.h"

void make_subtree(automaton_ptr aut) {

  mark_automaton(aut);
  copy_work_links(aut);

  sync_link_ptr slp = aut->work_links;
  sync_link_ptr hlp = NULL;
  while (slp != NULL) {

    if (is_automaton_marked(slp->other)) { //the target is marked, remove and free the link

      if (slp->prev != NULL) (slp->prev)->next = slp->next;
      else aut->work_links = slp->next;

      if (slp->next != NULL) (slp->next)->prev = slp->prev;

      free(slp->sync_action_names);
      slp->sync_action_names = NULL;
      hlp = slp->next;
      free(slp);
      slp = hlp;

    } else { //not marked; recursive call and move further in the work_links
      make_subtree(slp->other);
      slp = slp->next;
    }
  }

}

//TODO
automaton_ptr reduce_net(automaton_ptr aut) {

  automaton_ptr sq = NULL;

  //* a leaf *
  if (aut->work_links == NULL) {
    sq = get_fresh_automaton();
    sq->states = copy_state_list(aut->states);
    sq->parsed_transitions = aut->parsed_transitions;
    assert(collect_incidence_lists(sq));
    sq->parsed_transitions = NULL;

    return sq;
  } else sq = get_fresh_automaton();

  //* an internal node *

  //--- first make the states of the product ---
  //the initial state
  add_state(sq, strdup("init"));

  //the product states
  sync_link_ptr slp = aut->work_links;
  while (slp != NULL) { //for each child...

    //...make the states of its square product with the root
    for (state_ptr rst = aut->states; rst != NULL; rst = rst->next)
      for (state_ptr otst = slp->other->states; otst != NULL; otst = otst->next) {
        add_state(sq, get_qualified_pair_name(aut, rst->name, slp->other, otst->name));
      }
    slp = slp->next;
  }

  //--- now handle the transitions ---

  //connect the fresh initial state of sq with the initial state of each square product
  slp = aut->work_links;
  while (slp != NULL) {

   parsed_transition_ptr tr = (parsed_transition_ptr) malloc(sizeof(parsed_transition));
   tr->source = "init";
   tr->name = "epsilon";
   tr->target = get_qualified_pair_name(aut, aut->states->name,
                            slp->other, slp->other->states->name);
   tr->next = NULL;

   if (sq->parsed_transitions == NULL) sq->parsed_transitions = tr;
   else {
     tr->next = sq->parsed_transitions;
     sq->parsed_transitions = tr;
   }

   slp = slp->next;
  }
  display_automaton(sq);
  //todo - more connections

  //todo - labelings
  //a teraz przepisac?
  //todo - recursive call and cleanup

  return sq;
}

int main(int argc, char **argv) {

  unsigned int ctr = 0;
  unsigned int ASIZE = 10000;
  unsigned int actr = 0;
  automaton_ptr autos[ASIZE];

  /* read synchro array */
  synchro_array_ptr sarr = read_synchro_array(argv[argc-1]);

  printf("synchro array\n");
  for(int i = 0; i < sarr->ctr; ++i) printf("%s\n", sarr->actions[i]);

  /* read automata */
  while (true) {
    if (ctr == argc - 2) break;
    autos[actr++] = read_automaton(argv[++ctr]);
    collect_incidence_lists(autos[actr-1]);
    if (ctr > 1) add_automaton_to_network(autos[0], autos[actr-1], sarr);
  }

  display_network(autos[0]);

  printf("\nsaving the last automaton to zero.dot\n");

  network_to_dot(autos[actr-2], "net.dot");

  make_subtree(autos[0]);
  display_network(autos[0]);
  working_topology_to_dot(autos[0], "sync.dot");

  //test
  reduce_net(autos[0]);

  //cleanup
  for (int i = 0; i < actr-1; ++i) free_automaton(autos[i]);
  free_synchro_array(sarr);

  printf("\nDone.\n"); 
}
