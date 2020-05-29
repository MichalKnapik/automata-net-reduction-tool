#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"

automaton_ptr root = NULL;

void free_automaton(automaton_ptr aut) { //todo - add freeing outgoing

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

  printf("the incidence list:\n");
  
}

state_ptr get_state_by_name(automaton_ptr aut, char* state_name)
{
  state_ptr ptr = NULL;

  for (ptr = aut->states; ptr != NULL; ptr = ptr->next) {
    if (!strcmp(ptr->name, state_name)) break;
  }

  return ptr;
}

bool collect_incidence_lists(automaton_ptr aut)
{

  for (parsed_transition_ptr pptr = aut->parsed_transitions;
       pptr != NULL; pptr = pptr->next) {

    state_ptr src = get_state_by_name(aut, pptr->source);
    state_ptr trg = get_state_by_name(aut, pptr->target);

    if (src == NULL) {
      printf("unknown source %s\n", pptr->source);
      return false;
    }
    if (trg == NULL) {
      printf("unknown target %s\n", pptr->target);
      return false;
      }

    transition_ptr tr = (transition_ptr) malloc(sizeof(transition));
    tr->source = src;
    tr->target = trg;
    tr->name = strndup(pptr->name, MAXTOKENLENGTH);
    tr->next = NULL;        
    tr->prev = NULL;

    if (!src->outgoing) src->outgoing = tr;
    else {
      src->outgoing->prev = tr;
      tr->next = src->outgoing;
      src->outgoing = tr;
    }
    
  }
  
}

int main(int argc, char **argv)
{

 extern FILE* yyin;
 bool presult = 0;
 unsigned int ctr = 0;
 unsigned int ASIZE = 10;
 unsigned int actr = 0;
 automaton_ptr autos[ASIZE];
 
 while (true) {

   if (ctr == argc - 1) break;
   printf("Automaton (%d)\n", ctr);
   
   if((yyin = fopen(argv[++ctr], "r")) == NULL) {
     perror("an issue with reading models");
     exit(1);
   }

   yyrestart(yyin);
   presult = yyparse();
   autos[actr++] = root;
   display_automaton(root);
   if (presult != 0) break;
   
 }
 automaton_ptr last = autos[actr-1];

 collect_incidence_lists(last);
 
 for (int i = 0; i < actr; ++i) free_automaton(autos[i]);
 
}

