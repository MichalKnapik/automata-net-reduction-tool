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
automaton_ptr reduce_net(automaton_ptr aut, synchro_array_ptr sarr) {

  automaton_ptr sq = NULL;

  //* a leaf *
  if (aut->work_links == NULL) {
    sq = get_fresh_automaton();
    sq->states = copy_state_list(aut->states);
    sq->parsed_transitions = aut->parsed_transitions; //ew. skopiuj
    assert(collect_incidence_lists(sq));
    //sq->parsed_transitions = NULL; //TODO - przesun to do cleanupu, ale tylko dla liści (moze zamarkuj liscie?)

    return sq;
  } else sq = get_fresh_automaton();

  //* an internal node *

  //--- first make the states of the product ---

  //the initial state
  add_state(sq, strdup("init"));

  //the product states
  sync_link_ptr slp = aut->work_links;
  while (slp != NULL) { //for each link...
    automaton_ptr child = slp->other;

    //...make the states of its square product with the root
    for (state_ptr rst = aut->states; rst != NULL; rst = rst->next)
      for (state_ptr otst = child->states; otst != NULL; otst = otst->next) {
        add_state(sq, get_qualified_pair_name(aut, rst->name, child, otst->name));
      }
    slp = slp->next;
  }

  //--- now handle the transitions ---

  //1. connect the fresh initial state of sq with the initial state of each square product
  //   via epsilon transitions
  slp = aut->work_links;
  while (slp != NULL) {
    automaton_ptr child = slp->other;

    parsed_transition_ptr tr = make_parsed_transition("init", "epsilon",
                                                      get_qualified_pair_name(aut, aut->states->name,
                                                      child, child->states->name));

    add_parsed_transition(sq, tr);

    slp = slp->next;
  }

  //2. add the non-synchronised transitions of square products

  //2a. handle the root's transitions
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) {
    for (transition_ptr tp = sptr->outgoing; tp != NULL; tp = tp->next) {

      slp = aut->work_links;
      while (slp != NULL) { //for each link...
        automaton_ptr child = slp->other;

        if (automaton_knows_transition(child, tp->name, sarr)) {
          //handle a transition synchronised with the child
          /* printf("sync with %p\n", slp); */
          /* printf("%s %s %s\n", tp->source->name, tp->name, tp->target->name); */
          //todo
        } else {
          //handle a local transition of the root:
          //add transition [(sptr, childst), tp->name, (tp(sptr), childst)] for any childst
          for (state_ptr childst = child->states; childst != NULL; childst = childst->next) {
            parsed_transition_ptr tr = make_parsed_transition(get_qualified_pair_name(aut, tp->source->name, child, childst->name),
                                   tp->name,
                                   get_qualified_pair_name(aut, tp->target->name, child, childst->name));
            add_parsed_transition(sq, tr);
          }

        }

        slp = slp->next;
      }

    }

  }

  //2b. handle local transitions of each child
  //    self-note: do not optimize it by merging with the above cases
  //    (this might be horrible, but it's even worse if added above; prefer more clarity)
  for (slp = aut->work_links; slp != NULL; slp = slp->next) { //for each link...
    automaton_ptr child = slp->other;
    for (state_ptr childst = child->states; childst != NULL; childst = childst->next) { //...take the child's state...
      for (transition_ptr tp = childst->outgoing; tp != NULL; tp = tp->next) {//...and a transition leaving it...
        if (!automaton_knows_transition(aut, tp->name, sarr)) {//...ensure that it's child's local transition...
          //...and add transition [(rootst,childst), tp->name, (rootst, tp(childst))] for any rootst
          char* target_name = tp->target->name;
          for (state_ptr rootst = aut->states; rootst != NULL; rootst = rootst->next) {
            parsed_transition_ptr tr = make_parsed_transition(get_qualified_pair_name(aut, rootst->name, child, childst->name),
                                   tp->name,
                                   get_qualified_pair_name(aut, rootst->name, child, target_name));
            add_parsed_transition(sq, tr);
          }
        }

      }

    }
  }

  //todo - labelings
  //a teraz przepisac?
  //todo - recursive call and cleanup

  assert(collect_incidence_lists(sq)); //koniecznie!
  display_automaton(sq);

  exit (1);  
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
  reduce_net(autos[0], sarr);

  //cleanup
  for (int i = 0; i < actr-1; ++i) free_automaton(autos[i]);
  free_synchro_array(sarr);

  printf("\nDone.\n"); 
}
