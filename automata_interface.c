#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"
#include "tools.h"

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

  //todo - free sync_action_names
  
  free(aut);
}

void display_automaton(automaton_ptr aut) {

  printf("Automaton (%p) with states: ", aut);
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

  printf("synchronises with:\n");

  for (sync_link_ptr linx = aut->sync_links; linx != NULL; linx = linx->next) {
    printf("automaton %p via TODO\n", linx->other);
    //TODO
  }

  
}

void display_network(automaton_ptr aut) {

  automaton_ptr aptr = aut;
  int ctr = 0;
  while (aptr != NULL) {
    printf("(%d)\n", ctr++);
    display_automaton(aptr);
    aptr = aptr->next;
  }

}

state_ptr get_state_by_name(automaton_ptr aut, char* state_name) {

  state_ptr ptr = NULL;

  for (ptr = aut->states; ptr != NULL; ptr = ptr->next) {
    if (!strcmp(ptr->name, state_name)) break;
  }

  return ptr;
}

bool collect_incidence_lists(automaton_ptr aut) {

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

  return true;
}

void add_automaton_to_network(automaton_ptr net, automaton_ptr new_automaton) {

  //synchronise the entire network with the new automaton
  while (net->prev != NULL) net = net->prev;

  while (net->next != NULL) {
    sync_automata(net, new_automaton);
    net = net->next;
  }
  sync_automata(net, new_automaton);

  net->next = new_automaton;
  new_automaton->prev = net;
}

bool automaton_knows_transition(automaton_ptr aut, char* trans_name) {

  for (parsed_transition_ptr ptr = aut->parsed_transitions; ptr != NULL; ptr = ptr->next) {
    if (!strcmp(ptr->name, trans_name)) return true;
  }
  
  return false;
}

void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd) {

  bool common = false;
  for (parsed_transition_ptr ptr = snd->parsed_transitions; ptr != NULL; ptr = ptr->next) {
    if (automaton_knows_transition(fst, ptr->name)) {
      common = true;
      break;
    }
  }
  if (!common) return;
  
  int SYNCINITSIZE = 100000; //TODO - change me; dla < 10 dziala!!
  sync_link_ptr new_connection = (sync_link_ptr) malloc(sizeof(sync_link));
  if (!new_connection) {
    perror("sync_automata_one_way: mem allocation error");
    exit(1);
  }

  new_connection->other = snd;
  new_connection->sync_action_ctr = 0;
  new_connection->sync_action_capacity = SYNCINITSIZE;  
  new_connection->sync_action_names = (char**) malloc(SYNCINITSIZE * sizeof(char*));
  
  for (parsed_transition_ptr ptr = snd->parsed_transitions; ptr != NULL; ptr = ptr->next) {

    if (automaton_knows_transition(fst, ptr->name)) {
      if (new_connection->sync_action_ctr == new_connection->sync_action_capacity - 1) {
	grow_ref_array(&new_connection->sync_action_capacity, sizeof(char*), (void**)&new_connection->sync_action_names);
      }
      new_connection->sync_action_names[(new_connection->sync_action_ctr)++] = strndup(ptr->name, MAXTOKENLENGTH);
    }

  }

  //TODO now - nie dziala
  if (fst->sync_links == NULL) {
    fst->sync_links = new_connection;
  }
  else {
    sync_link_ptr sp = fst->sync_links;

    while (sp->next != NULL) {
    printf("1\n");      
      sp = sp->next;
    printf("2\n");            
    }

    sp->next = new_connection;
  }
  
}

void sync_automata(automaton_ptr fst, automaton_ptr snd) {
  sync_automata_one_way(fst, snd);
  sync_automata_one_way(snd, fst);
}

automaton_ptr read_automaton(char* fname) {

  extern FILE* yyin;

   if((yyin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading models");
     //     exit(1);
   }

   yyrestart(yyin);

   automaton_ptr retv = NULL;
   if (!yyparse()) retv = root;

   fclose(yyin);

   return retv;
}

synchro_array_ptr read_synchro_array(char* fname) {

  FILE* fin;
  if ((fin = fopen(fname, "r")) == NULL) {
     perror("an issue with reading synchronising actions");
     exit(1);
  }

  char buffer[MAXTOKENLENGTH];
  
  int AINITSIZE = 100000;
  synchro_array_ptr arr = (synchro_array_ptr) malloc(sizeof(synchro_array));
  arr->capacity = AINITSIZE;
  arr->ctr = 0;
  arr->actions = (char**) malloc(arr->capacity * sizeof(char*));

  while (fgets(buffer, MAXTOKENLENGTH, fin)) {

    if (strlen(buffer) >= 1) buffer[strlen(buffer)-1] = '\0';
    if (arr->ctr == arr->capacity - 1) 
      grow_ref_array(&arr->capacity, sizeof(char*), (void**) &arr->actions);

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

 unsigned int ctr = 0;
 unsigned int ASIZE = 10000;
 unsigned int actr = 0;
 automaton_ptr autos[ASIZE];

 while (true) {
   if (ctr == argc - 2) break;
   autos[actr++] = read_automaton(argv[++ctr]);
   collect_incidence_lists(autos[actr-1]);
   if (ctr > 1) add_automaton_to_network(autos[0], autos[actr-1]);
 }

 display_network(autos[0]);
 
 printf("*synchro array*\n");
 synchro_array_ptr sarr = read_synchro_array(argv[ctr+1]);

 // for(int i = 0; i < sarr->ctr; ++i) printf("%s\n", sarr->actions[i]);
 
 for (int i = 0; i < actr-1; ++i) free_automaton(autos[i]);
 free_synchro_array(sarr);
 
}
