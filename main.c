#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "tree_topology.h"

extern int optind;
automaton_ptr global_root;

void print_usage(void) {
  printf("Usage: tree_reduce -m module1 module2 ... modulen -s sync_actions [-d] [-o] [-v]\n\
where: \n-d saves the network in net.dot, the synchronisation scheme in sync.dot, and the EF-reduced product in reduced.dot;\
\n-o means that the synchronizations follow the single-sync rule while by default the automata are live-reset;\
 \n-v stands for verbose; \n-u turns off optimizing deadlock removal (experimental, as all here).\n");
}

int main(int argc, char **argv) {

  int c;
  int sarr_ind = -1;
  int module_ind_init = -1;
  int module_ind_end = -1;
  bool todot = false;
  bool verbose = false;
  bool one_shot = false;    
  bool no_deadlock_reduction = false;

  //todo: this might be a bad way of using getopt
  while ((c = getopt(argc, argv, "msduvo")) != -1) {

    switch (c) {
    case 'm':
      module_ind_init = optind;
      while (optind < argc && argv[optind][0] != '-') ++optind;
      module_ind_end = optind - 1;
      break;
    case 's':
      sarr_ind = optind;
      while (optind < argc && argv[optind][0] != '-') ++optind;
      if (optind - sarr_ind > 1 || optind - sarr_ind == 0) sarr_ind = -1;
      break;
    case 'd':
      todot = true;
      break;
    case 'v':
      verbose = true;
      break;
    case 'o':
      one_shot = true;
      break;      
    case 'u':
      no_deadlock_reduction = true;
      break;      
    default:
      exit(EXIT_FAILURE);
    }
    
  }

  if (module_ind_init == -1 || module_ind_end - module_ind_init + 1 == 0) {
    printf("Error: no modules given.\n");
    print_usage();
    exit(EXIT_FAILURE);
  }
  if (sarr_ind == -1) {
    printf("Error: no list of synchronising actions given or multiple lists given.\n");
    print_usage();
    exit(EXIT_FAILURE);
  }
  
  unsigned int ctr = module_ind_init;
  unsigned int ASIZE = 50000; //change this to allow bigger nets
  unsigned int actr = 0;
  automaton_ptr autos[ASIZE];

  /* read synchro array */
  synchro_array_ptr sarr = read_synchro_array(argv[sarr_ind]);

  /* read automata */
  while (true) {
    if (ctr > module_ind_end) break;
    autos[actr] = read_automaton(argv[ctr]);
    collect_incidence_lists(autos[actr]);
    if (actr > 0) add_automaton_to_network(autos[0], autos[actr], sarr);
    ++ctr; ++actr;
  }
  global_root = autos[0];

  printf("Read network of %d automata.\n", module_ind_end - module_ind_init + 1);
  if (verbose) {
    display_network(global_root);
    printf("\n");
  }

  if (todot) {
    printf("Exporting the automata to net.dot.\n");
    network_to_dot(global_root, "net.dot");
  }

  printf("Computing synchronisation subtree rooted in the first automaton.\n");
  make_subtree(global_root);
  if (verbose) {
    printf("Displaying network:");
    display_network(global_root);
    printf("\n");    
  }
  if (todot) {
    printf("Exporting the synchronisation scheme to sync.dot.\n");    
    working_topology_to_dot(global_root, "sync.dot");
  }

  printf("EF-reducing the network...");
  automaton_ptr red = reduce_net(global_root, NULL, sarr, one_shot, no_deadlock_reduction);

  if (red == NULL) {
    printf("..the result is empty. Check the network!\n");
    return 1;
  }
  printf("..the result has %d states and %d transitions.\n", count_states(red), count_transitions(red));

  if (verbose) {
    printf("Displaying reduced automaton\n");  
    display_automaton(red);
    printf("\n");    
  }

  if (todot) {
    printf("Exporting the EF-reduced product to reduced.dot.\n");
    network_to_dot(red, "reduced.dot");
  }

  //cleanup
  for (int i = 0; i < actr - 1; ++i) free_automaton(autos[i]);
  free_synchro_array(sarr);
  free_automaton(red);

  printf("Done.\n");
  if (todot) printf("Run 'make dot' to produce pdfs from dot files.\n");

  return 0;
}
