#include <stdio.h>
#include <stdlib.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"

automaton_ptr root = NULL;

void free_automaton(automaton_ptr aut) {

  state_ptr st = aut->states;
  state_ptr nexts = NULL;
  while (st != NULL) {
    nexts = st->next;
    free(st->name);
    free(st);
    st = nexts;
  }

  parsed_transition_ptr tr = aut->parsed_transitions;
  parsed_transition_ptr nextt = NULL;
  while (tr != NULL) {
    nextt = tr->next;
    free(tr->source);
    free(tr->name);
    free(tr->target);
    free(tr);
    tr = nextt;
  }

  free(aut);

}

void display_automaton(automaton_ptr aut) {

  printf("Automaton with states:\n");
  state_ptr st = aut->states;
  while (st != NULL) {
    printf("%s\n", st->name);
    st = st->next;
  }

  printf("and parsed_transitions:\n");
  parsed_transition_ptr tr = aut->parsed_transitions;
  while (tr != NULL) {
    printf("(%s, %s, %s)\n", tr->source, tr->name, tr->target);
    tr = tr->next;
  }
  
}

int main(int argc, char **argv)
{

 extern FILE* yyin;
 bool presult = 0;
 unsigned int ctr = 0;

 while (true) {

   printf("%d\n", ctr);
   if (ctr == argc - 1) break;
   if((yyin = fopen(argv[++ctr], "r")) == NULL) {
     perror("an issue with reading models");
     exit(1);
   }

   yyrestart(yyin);
   presult = yyparse();
   display_automaton(root);
   free_automaton(root);
   if (presult != 0) break;
   
 }

}
