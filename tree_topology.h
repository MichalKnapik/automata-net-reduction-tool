#ifndef TREE_TOPOLOGY_H
#define TREE_TOPOLOGY_H

#include "common.h"
#include "tools.h"
#include "automata_interface.h"

/* Computes a spanning (DFS) tree w.r.t. synchronisation topology rooted
   at aut. The links of the tree are held in work_links of the automata
   (sync_links are not modified). Warning: recursive; consider rewriting. */
void make_subtree(automaton_ptr aut);

/* Marks those states of aut where an action from root is executable. */
void mark_root_live_states(automaton_ptr aut, automaton_ptr root);

/* Reduces the network rooted in aut using the sum-of-squares construction.
   Uses synchronisation topology described by work_links, so you should
   run make_subtree(aut) first. Returns a pointer to the new reduced product. */
automaton_ptr reduce_net(automaton_ptr aut, synchro_array_ptr sarr);

#endif

