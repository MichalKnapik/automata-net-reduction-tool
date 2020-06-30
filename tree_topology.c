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

automaton_ptr reduce_net(automaton_ptr aut, automaton_ptr father, synchro_array_ptr sarr) {

  automaton_ptr sq = NULL;

  //* a leaf *
  if (aut->work_links == NULL) {
    sq = get_fresh_automaton();
    sq->states = copy_state_list(aut->states);
    copy_transition_records(sq, aut);
    assert(collect_incidence_lists(sq));
    return sq;
  } else {
    //* recursive call *
    for (sync_link_ptr slp = aut->work_links; slp != NULL; slp = slp->next) {
      automaton_ptr reduced_child = reduce_net(slp->other, aut, sarr);
      slp->other = reduced_child;
    }    
    sq = get_fresh_automaton();
  }

  //* an internal node *

  //--- first make the states of the product ---

  //the initial state
  add_state(sq, strdup("init"));

  //the product states
  sync_link_ptr slp = aut->work_links;
  while (slp != NULL) { //for each link...
    automaton_ptr child = slp->other;

    //...make the states of square its product with the root
    for (state_ptr rst = aut->states; rst != NULL; rst = rst->next)
      for (state_ptr otst = child->states; otst != NULL; otst = otst->next) {
        add_state(sq, get_qualified_pair_name(aut, rst->name, child, otst->name));
      }
    slp = slp->next;
  }

  //--- now handle the transitions ---
  //1. connect the fresh initial state of sq with the initial state of each square product via epsilon transitions
  slp = aut->work_links;
  while (slp != NULL) {
    automaton_ptr child = slp->other;
    transition_record_ptr tr = make_transition_record(strdup("init"), strdup("epsilon"),
                                                      get_qualified_pair_name(aut, aut->states->name,
                                                                              child, child->states->name));
    add_transition_record(sq, tr);
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
          //handle a transition synchronised with the child:
          //add transition [(sptr,s), tp->name, (tp->target,s'), for any initial state s' of any child
          //(this works for live-reset automata only!)

          int matching_state_ctr = 0;
          state_ptr* matching_child_states = get_states_with_enabled(child, tp->name, &matching_state_ctr);
          for (int i = 0; i < matching_state_ctr; ++i) {
            for (sync_link_ptr inner_slp = aut->work_links; inner_slp != NULL; inner_slp = inner_slp->next) {
              automaton_ptr other_child = inner_slp->other;
              transition_record_ptr tr = make_transition_record(get_qualified_pair_name(aut, tp->source->name, child, matching_child_states[i]->name),
                                                                strdup(tp->name),
                                                                get_qualified_pair_name(aut, tp->target->name, other_child, get_initial_state(other_child)->name));
              add_transition_record(sq, tr);
            }
          }
          free(matching_child_states);

        } else if (is_action_local(aut, tp->name, sarr)) {
          //handle a possibly local transition of the root, but first check
	  //if it is really local or maybe synchronises with root's father

	  if (false && automaton_knows_transition(father, tp->name, sarr)) { //tp->name syncs with aut's father
	    //todo - error somewhere
	    //aut and aut's root synchronise over tp->name
	    //add transition [(sptr, childst), tp->name, (init)] for any childst
	    for (state_ptr childst = child->states; childst != NULL; childst = childst->next) {
	      transition_record_ptr tr = make_transition_record(get_qualified_pair_name(aut, tp->source->name, child, childst->name),
								strdup(tp->name),
								get_initial_state(sq)->name);
	      add_transition_record(sq, tr);
	    }
	  }
	  else { //tp->name is really local
	    //add transition [(sptr, childst), tp->name, (tp(sptr), childst)] for any childst
	    for (state_ptr childst = child->states; childst != NULL; childst = childst->next) {
	      transition_record_ptr tr = make_transition_record(get_qualified_pair_name(aut, tp->source->name, child, childst->name),
								strdup(tp->name),
								get_qualified_pair_name(aut, tp->target->name, child, childst->name));
	      add_transition_record(sq, tr);
	    }
	  }

	}

        slp = slp->next;
      }

    }
 
  }

  //2b. handle local transitions of each child
  //    self-note: do not optimize it by merging with the above cases
  //    (this might be horrible, but it's even worse if added above)
  for (slp = aut->work_links; slp != NULL; slp = slp->next) { //for each link...
    automaton_ptr child = slp->other;
    for (state_ptr childst = child->states; childst != NULL; childst = childst->next) { //...take the child's state...
      for (transition_ptr tp = childst->outgoing; tp != NULL; tp = tp->next) {//...and a transition leaving it...
        if (!automaton_knows_transition(aut, tp->name, sarr)) {//...ensure that it's child's local transition...
          //...(warning: we assume that the child has no children here)...
          //...and add transition [(rootst,childst), tp->name, (rootst, tp(childst))] for any rootst
          char* target_name = tp->target->name;
          for (state_ptr rootst = aut->states; rootst != NULL; rootst = rootst->next) {
            transition_record_ptr tr = make_transition_record(get_qualified_pair_name(aut, rootst->name, child, childst->name),
                                                              strdup(tp->name),
                                                              get_qualified_pair_name(aut, rootst->name, child, target_name));
            add_transition_record(sq, tr);
          }
        }

      }

    }
  }

  assert(collect_incidence_lists(sq)); //needed, don't remove

  //*** At this stage sq is the unreduced square product. Let's reduce it. ***
  
  mark_states_with_root_active_actions(aut, sq);
  mark_reaching_marked(sq);
  automaton_ptr reduced = remove_unmarked_states(sq);
  mark_reachable_from_initial(reduced);
  automaton_ptr reduced_without_unreachables = remove_unmarked_states(reduced);
  free_automaton(reduced);

  //todo - labelings, and maybe rewrites

  //cleanup: remove results of recursive calls and square product
  for (sync_link_ptr slp = aut->work_links; slp != NULL; slp = slp->next)
    free_automaton(slp->other);
  free_automaton(sq);

  return reduced_without_unreachables;
}

int main(int argc, char **argv) {

  unsigned int ctr = 0;
  unsigned int ASIZE = 10000;
  unsigned int actr = 0;
  automaton_ptr autos[ASIZE];

  /* read synchro array */
  synchro_array_ptr sarr = read_synchro_array(argv[argc-1]);

  printf("synchro array\n");
  for(int i = 0; i < sarr->ctr; ++i) printf("%s\n", (char*) sarr->actions[i]);

  /* read automata */
  while (true) {
    if (ctr == argc - 2) break;
    autos[actr++] = read_automaton(argv[++ctr]);
    collect_incidence_lists(autos[actr-1]);
    if (ctr > 1) add_automaton_to_network(autos[0], autos[actr-1], sarr);
  }

  display_network(autos[0]);

  printf("\nsaving the last automaton to zero.dot\n");

  network_to_dot(autos[actr - 2], "net.dot");

  make_subtree(autos[0]);
  display_network(autos[0]);

  working_topology_to_dot(autos[0], "sync.dot");

  //test
  automaton_ptr red = reduce_net(autos[0], NULL, sarr);
  
  printf("\n\n\n");
  display_automaton(red);

  
  printf("\n\n\n");
  network_to_dot(red, "reduced.dot");

  //cleanup

  for (int i = 0; i < actr - 1; ++i) free_automaton(autos[i]);
  free_synchro_array(sarr);
  free_automaton(red);

  printf("\nDone.\n");
}
