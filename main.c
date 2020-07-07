#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "tree_topology.h"

extern int optind;

void print_usage(void) {
  printf("Usage: tree_reduce -m module1 module2 ... modulen -s sync_actions [-d] [-v]\n\
       Where: -d saves the network in net.dot, the synchronisation scheme in sync.dot,\n\
       and the EF-reduced product in reduced.dot; -v stands for verbose. \n ");
}

int main(int argc, char **argv) {

  int c;
  int sarr_ind = -1;
  int module_ind_init = -1;
  int module_ind_end = -1;
  bool todot = false;
  bool verbose = false;  

  //todo: this might be a bad way of using getopt
  while ((c = getopt(argc, argv, "msdv")) != -1) {

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

  printf("Read network of %d automata.\n", module_ind_end - module_ind_init + 1);
  if (verbose) {
    display_network(autos[0]);
    printf("\n");
  }

  if (todot) {
    printf("Exporting the automata to zero.dot.\n");
    network_to_dot(autos[0], "net.dot");
  }

  printf("Computing synchronisation subtree rooted in the first automaton.\n");
  make_subtree(autos[0]);
  if (verbose) {
    printf("Displaying network:");
    display_network(autos[0]);
    printf("\n");    
  }
  if (todot) {
    printf("Exporting the synchronisation scheme to sync.dot.\n");    
    working_topology_to_dot(autos[0], "sync.dot");
  }

  printf("EF-reducing the network...");
  automaton_ptr red = reduce_net(autos[0], NULL, sarr);
  printf("..the result has %d states.\n", count_states(red));

  if (verbose) {
    printf("Displaying reduced automaton\n");  
    display_automaton(red);
    printf("\n");    
  }

  if (todot) {
    printf("Exporting the EF-reduced product to zero.dot.\n");
    network_to_dot(red, "reduced.dot");
  }

  //cleanup
  for (int i = 0; i < actr - 1; ++i) free_automaton(autos[i]);
  free_synchro_array(sarr);
  free_automaton(red);

  printf("Done.\n");
  if (todot) printf("Run 'make dot' to produce pdfs from dot files.\n");

}
