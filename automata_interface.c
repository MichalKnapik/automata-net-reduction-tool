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

  free_sync_links(aut);

  free_work_links(aut);

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

  printf("the incidence list:");
  if (aut->flags & AUTOM_INCIDENCE_OK) {
    print_incidence_list(aut);
  } else printf(" (not computed)");

  printf("\nsynchronises with: ");
  print_synchro_links(aut, MAIN_SYNCHRO_LINKS);

  printf("\nthe working synchronisation list:");
  if (aut->work_links != NULL) {
    print_synchro_links(aut, WORKER_SYNCHRO_LINKS);
  } else printf(" (not computed)");

}

void display_network(automaton_ptr aut) {

  automaton_ptr aptr = aut;
  int ctr = 0;
  while (aptr != NULL) {
    printf("\n(%d)\n", ctr++);
    display_automaton(aptr);
    aptr = aptr->next;
  }

}

void print_synchro_links(automaton_ptr aut, SL_CHOICE ptype) {

  if (aut->sync_links == NULL) printf("(none)");

  sync_link_ptr linx = NULL;
  if (ptype == MAIN_SYNCHRO_LINKS) {
    linx = aut->sync_links;
  } else if (ptype == WORKER_SYNCHRO_LINKS) {
    linx = aut->work_links;
  }

  while (linx != NULL) {
    printf("\nautomaton %p via:\n", linx->other);
    for (int i = 0; i < linx->sync_action_ctr; ++i)
      printf("%s ", linx->sync_action_names[i]);

    linx = linx->next;
  }

}

void print_incidence_list(automaton_ptr aut) {

  transition_ptr tp = NULL;
  state_ptr st = aut->states;

  while (st != NULL) {
    printf("\n(%s): ", st->name);
    for (tp = st->outgoing; tp != NULL; tp = tp->next) {
      printf(" %s (%s).", tp->name, tp->target->name);
    }
    st = st->next;
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

void add_automaton_to_network(automaton_ptr net, automaton_ptr new_automaton, synchro_array_ptr sarr) {

  while (net->prev != NULL) net = net->prev;

  while (net->next != NULL) {
    sync_automata(net, new_automaton, sarr);
    net = net->next;
  }
  sync_automata(net, new_automaton, sarr);

  net->next = new_automaton;
  new_automaton->prev = net;
}

bool automaton_knows_transition(automaton_ptr aut, char* trans_name, synchro_array_ptr sarr) {

  for (parsed_transition_ptr ptr = aut->parsed_transitions; ptr != NULL; ptr = ptr->next) {
    if (!strcmp(ptr->name, trans_name) &&
        (sarr == NULL || cstring_array_contains(sarr->actions, sarr->ctr, trans_name))) return true;
  }

  return false;
}

void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd, synchro_array_ptr sarr) {

  bool common = false;
  for (parsed_transition_ptr ptr = snd->parsed_transitions; ptr != NULL; ptr = ptr->next) {

    if (automaton_knows_transition(fst, ptr->name, sarr)) {
      common = true;
      break;
    }
  }
  if (!common) return;

  int SYNCINITSIZE = 10;
  sync_link_ptr new_connection = (sync_link_ptr) malloc(sizeof(sync_link));
  if (!new_connection) {
    perror("sync_automata_one_way: mem allocation error");
    exit(1);
  }

  new_connection->other = snd;
  new_connection->sync_action_ctr = 0;
  new_connection->sync_action_capacity = SYNCINITSIZE;
  new_connection->sync_action_names = (char**) malloc(SYNCINITSIZE * sizeof(char*));
  new_connection->next = NULL;

  for (parsed_transition_ptr ptr = snd->parsed_transitions; ptr != NULL; ptr = ptr->next) {

    if (automaton_knows_transition(fst, ptr->name, sarr)) {
      if (new_connection->sync_action_ctr == new_connection->sync_action_capacity - 1) {
	grow_ref_array(&new_connection->sync_action_capacity, sizeof(char*), (void**)&new_connection->sync_action_names);
      }
      new_connection->sync_action_names[(new_connection->sync_action_ctr)++] = strndup(ptr->name, MAXTOKENLENGTH);
    }

  }

  if (fst->sync_links == NULL) {
    fst->sync_links = new_connection;
  }
  else {
    sync_link_ptr sp = fst->sync_links;
    while (sp->next != NULL)
      sp = sp->next;

    sp->next = new_connection;
  }

}

void sync_automata(automaton_ptr fst, automaton_ptr snd, synchro_array_ptr sarr) {
  sync_automata_one_way(fst, snd, sarr);
  sync_automata_one_way(snd, fst, sarr);
}

void free_sync_links(automaton_ptr aut) {

  sync_link_ptr hlp = NULL;
  sync_link_ptr sl = aut->sync_links;

  while (sl != NULL) {
    hlp = sl;
    for (int i = 0; i < sl->sync_action_ctr; ++i) {
      free(sl->sync_action_names[i]);
    }
    free(sl->sync_action_names);
    sl = sl->next;
    if (hlp != NULL) free(hlp);
  }

}

void free_work_links(automaton_ptr aut) {

  sync_link_ptr hlp = NULL;
  sync_link_ptr sl = aut->work_links;

  while (sl != NULL) {
    hlp = sl;
    free(sl->sync_action_names);
    sl = sl->next;
    if (hlp != NULL) free(hlp);
  }

}

void copy_work_links(automaton_ptr aut) {

  sync_link_ptr lptr = NULL;
  sync_link_ptr bgnptr = NULL;

  for (sync_link_ptr link = aut->sync_links; link != NULL; link = link->next) {
    if (bgnptr == NULL) {
      lptr = (sync_link_ptr) malloc(sizeof(sync_link));
      bgnptr = lptr;
    } else {
      lptr->next = (sync_link_ptr) malloc(sizeof(sync_link));
      lptr = lptr->next;
    }

    lptr->other = link->other;
    lptr->sync_action_ctr = link->sync_action_ctr;
    lptr->sync_action_capacity = link->sync_action_capacity;
    lptr->sync_action_names = (char**) malloc(link->sync_action_capacity * sizeof(char*));
    lptr->next = NULL;

    memcpy(lptr->sync_action_names, link->sync_action_names, link->sync_action_ctr * sizeof(char*));
  }

  aut->work_links = bgnptr;
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

   fclose(yyin);

   return retv;
}

bool automaton_to_dot(automaton_ptr aut, char* dotfname) {

  FILE* dotf = NULL;
  if ((dotf = fopen(dotfname, "w")) == NULL) {
    perror("can't write dot file");
    return false;
  }

  fprintf(dotf, "graph g%p {\n", aut);
  fprintf(dotf, "%s [style = filled, color = green];\n", aut->states->name);
  for (state_ptr state = aut->states; state != NULL; state = state->next) {

    for (transition_ptr trans = state->outgoing; trans != NULL; trans = trans->next) {
      fprintf(dotf, "%s -- %s [label = \"%s\"];\n", trans->source->name, trans->target->name, trans->name);
    }

  }

  fprintf(dotf, "label = \"automaton: %p\\n\\n\";\n", aut);
  fprintf(dotf, "labelloc = t;\n");
  fprintf(dotf, "}");

  if (dotf != NULL) fclose(dotf);

  return true;
}
