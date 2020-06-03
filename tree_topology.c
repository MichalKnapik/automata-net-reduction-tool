#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "automata_interface.h"
#include "tools.h"
#include "tree_topology.h"

int main(int argc, char **argv) {

 unsigned int ctr = 0;
 unsigned int ASIZE = 10000;
 unsigned int actr = 0;
 automaton_ptr autos[ASIZE];

 while (true) {
   if (ctr == argc - 1) break;
   autos[actr++] = read_automaton(argv[++ctr]);
   collect_incidence_lists(autos[actr-1]);
   if (ctr > 1) add_automaton_to_network(autos[0], autos[actr-1]);
 }

 display_network(autos[0]);

 printf("\ndott\n");
 automaton_to_dot(autos[0], "zero.dot");
 

 printf("\nDone.\n");
 
 /* printf("\n*synchro array*\n"); */
 /* synchro_array_ptr sarr = read_synchro_array(argv[ctr+1]); */

 /* // for(int i = 0; i < sarr->ctr; ++i) printf("%s\n", sarr->actions[i]); */
 
 /* for (int i = 0; i < actr-1; ++i) free_automaton(autos[i]); */
 /* free_synchro_array(sarr); */
 
}
