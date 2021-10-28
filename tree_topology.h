#ifndef TREE_TOPOLOGY_H
#define TREE_TOPOLOGY_H

#include "common.h"
#include "tools.h"
#include "automata_interface.h"

/* Computes a spanning (DFS) tree w.r.t. synchronisation topology rooted
   at aut. The links of the tree are held in work_links of the automata
   (sync_links are not modified). Warning: recursive; consider rewriting. */
void make_subtree(automaton_ptr aut);

/* Reduces the network rooted in aut using the sum-of-squares construction.
   Uses synchronisation topology described by work_links, so you should
   run make_subtree(aut) first. Returns a pointer to the new reduced product. 
   Set one_shot to false if the network is live-reset, and true if it is one-shot 
   synchronizing. */
automaton_ptr reduce_net(automaton_ptr aut, automaton_ptr father, synchro_array_ptr sarr, bool one_shot, bool deadlock_reduction);

#endif

