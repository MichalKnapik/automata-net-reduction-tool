#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"

automaton_ptr root = NULL;

void free_automaton(automaton_ptr aut) {

  state_ptr st = aut->states;
  state_ptr nexts = NULL;
  transition_ptr tp,tnxt = NULL;

  while (st != NULL) {
    nexts = st->next;
    free(st->name);

    tp = st->outgoing;
    while (tp != NULL) {
     tnxt = tp->next;
     free(tp->name);
     free(tp);
     tp = tnxt;
    }
    
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

  printf("Automaton with states: ");
  state_ptr st = aut->states;
  while (st != NULL) {
    printf("%s ", st->name);
    st = st->next;
  }

  printf("\nand parsed_transitions:\n");
  parsed_transition_ptr tr = aut->parsed_transitions;
  while (tr != NULL) {
    printf("(%s, %s, %s)\n", tr->source, tr->name, tr->target);
    tr = tr->next;
  }

  transition_ptr tp = NULL;
  printf("the incidence list:");
  if (aut->flags & AUTOM_INCIDENCE_OK) {
    st = aut->states;
    while (st != NULL) {
      printf("\n(%s): ", st->name);
      for (tp = st->outgoing; tp != NULL; tp = tp->next) {
	printf(" %s (%s).", tp->name, tp->target->name);
      }
      st = st->next;
    }
  } else printf(" (not computed)");
  printf("\n");
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
  aut->flags |= AUTOM_INCIDENCE_OK;

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

automaton_ptr read_automaton(char* fname) {

  extern FILE* yyin;

   if((yyin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading models");
     exit(1);
   }

   yyrestart(yyin);

   automaton_ptr retv = NULL;
   if (!yyparse()) retv = root;
   if (!yyin) fclose(yyin);

   return retv;
}

synchro_array_ptr read_synchro_array(char* fname) {

  FILE* fin;
  if ((fin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading synchronising actions");
     exit(1);
  }

  char buffer[MAXTOKENLENGTH];
  
  int AINITSIZE = 10;
  synchro_array_ptr arr = (synchro_array_ptr) malloc(sizeof(synchro_array));
  arr->capacity = AINITSIZE;
  arr->ctr = 0;
  arr->actions = (char**) malloc(arr->capacity * sizeof(char*));

  while (fgets(buffer, MAXTOKENLENGTH, fin)) {

    if (strlen(buffer) >= 1) buffer[strlen(buffer)-1] = '\0';
    if (arr->ctr == arr->capacity) {
      arr->capacity *= arr->capacity;
      char** new_act_array = (char**) malloc(arr->capacity * sizeof(char*));
      memcpy(new_act_array, arr->actions, sizeof(char*) * arr->ctr);
      free(arr->actions);
      arr->actions = new_act_array;
    }

    arr->actions[(arr->ctr)++] = strndup(buffer, MAXTOKENLENGTH);
  }

  if (!fin) fclose(fin);
  return arr;
}

void free_synchro_array(synchro_array_ptr sarr) {
  while(--(sarr->ctr) >= 0) free(sarr->actions[sarr->ctr]);
  free(sarr->actions);
  free(sarr);
}

int main(int argc, char **argv) {

 extern FILE* yyin;
 bool presult = 0;
 unsigned int ctr = 0;
 unsigned int ASIZE = 10;
 unsigned int actr = 0;
 automaton_ptr autos[ASIZE];
 
 while (true) {

   if (ctr == argc - 1) break;
   printf("Automaton (%d)\n", ctr);

   autos[actr++] = read_automaton(argv[++ctr]);

   display_automaton(root);
   
 }

 automaton_ptr last = autos[actr-2];

 synchro_array_ptr sarr = read_synchro_array(argv[ctr]);

 collect_incidence_lists(last);
 display_automaton(last);

 for(int i = 0; i < sarr->ctr; ++i) printf("%s\n", sarr->actions[i]);
 
 for (int i = 0; i < actr-1; ++i) free_automaton(autos[i]);
 free_synchro_array(sarr);
}
