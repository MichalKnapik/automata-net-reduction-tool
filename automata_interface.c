#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"
#include "tools.h"

automaton_ptr root = NULL;

automaton_ptr get_fresh_automaton(void) {

  automaton_ptr ap = malloc(sizeof(automaton));
  ap->flags = AUTOM_NONE;
  ap->states = NULL;
  ap->transition_records = NULL;
  ap->next = NULL;
  ap->prev = NULL;
  ap->sync_links = NULL;
  ap->work_links = NULL;

  return ap;
}

state_ptr copy_state_list(state_ptr sptr) {

  if (sptr == NULL) return NULL;

  state_ptr bptr = make_state(strdup(sptr->name));

  state_ptr cptr = bptr;
  sptr = sptr->next;

  while (sptr != NULL) {
    cptr->next = make_state(strdup(sptr->name));
    cptr = cptr->next;
    sptr = sptr->next;
  }

  return bptr;
}

state_ptr make_state(char* stname) {

    state_ptr st = malloc(sizeof(state));
    st->next = NULL;
    st->marked = false;
    st->outgoing = NULL;
    st->incoming = NULL;
    st->name = stname;

    return st;
}

void add_state(automaton_ptr aut, char* stname) {

  state_ptr sptr = aut->states;

  if (sptr == NULL) {
    aut->states = make_state(stname);
    return;
  }

  while (sptr->next != NULL) {
    sptr = sptr->next;
  }
  sptr->next = make_state(stname);

}

state_ptr get_initial_state(automaton_ptr aut) {
  return aut->states;
}

bool is_state_initial(automaton_ptr aut, state_ptr state) {
  return (get_initial_state(aut) == state);
}

/* Checks if act_name is a local action of aut, i.e., none of automata
 connected via work_links knows act_name or it is not in sarr. */
bool is_action_local(automaton_ptr aut, char* act_name, synchro_array_ptr sarr) {

  /* Note: this is inefficient, caching or precomputing is needed. */
  bool is_local = true;

  if (!cstring_array_contains((char**) sarr->actions, sarr->ctr, act_name)) return true;

  sync_link_ptr sl = aut->work_links;
  automaton_ptr neighbour = NULL;

  while (sl != NULL) {
    neighbour = sl->other;
    if (automaton_knows_transition(neighbour, act_name, sarr)) {
      is_local = false;
      break;
    }
    sl = sl->next;
  }

  return is_local;
}

void add_transition_record(automaton_ptr aut, transition_record_ptr tr) {

   if (aut->transition_records == NULL) aut->transition_records = tr;
   else {
     tr->next = aut->transition_records;
     aut->transition_records = tr;
   }

}

void copy_transition_records(automaton_ptr aut_dest, automaton_ptr aut_src) {

  assert(aut_dest->transition_records == NULL);
  for (transition_record_ptr trp = aut_src->transition_records; trp != NULL; trp = trp->next) 
    add_transition_record(aut_dest, make_transition_record(strdup(trp->source), strdup(trp->name), strdup(trp->target)));

}

void mark_state(state_ptr spt) {
  spt->marked = true;
}

void clear_state(state_ptr spt) {
  spt->marked = false;
}

bool is_state_marked(state_ptr spt) {
  return spt->marked;
}

void clear_all_states(automaton_ptr aut) {
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next)
    clear_state(sptr);
}

void clear_all_states_in_network(automaton_ptr net) {

  while (net->prev != NULL) net = net->prev;

  while (net->next != NULL) {
    clear_all_states(net);
    net = net->next;
  }

}

void mark_reaching_marked(automaton_ptr aut) {
  
  //TODO later: use the proper wrapper for dynamic structs

  int marked_ctr = 0;
  int INIT_ARR_SIZE = 1000;
  int scapacity = INIT_ARR_SIZE;

  //stt holds states from which a marked states is reachable
  state_ptr* stt = malloc(INIT_ARR_SIZE * sizeof(state_ptr));

  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) {
    if (is_state_marked(sptr)) {
      if (marked_ctr >= scapacity) {
        grow_ref_array(&scapacity, (void**) &stt);
      }
      stt[marked_ctr++] = sptr;

    }
  }

  //a bit tricky, if dirty: a growing array is treated as a FIFO
  //with marked_ctr pointing to its end
  for (int i = 0; i < marked_ctr; ++i) {

    for (transition_ptr iptr = stt[i]->incoming; iptr != NULL; iptr = iptr->next) {
      state_ptr state_in_preimage = iptr->source;
      if (!is_state_marked(state_in_preimage)) {
          if (marked_ctr >= scapacity) {
            grow_ref_array(&scapacity, (void**) &stt);
          }
          stt[marked_ctr++] = state_in_preimage;
          mark_state(state_in_preimage);
      }
    }

  }

   free(stt);
}

void mark_reachable_from_state(state_ptr sptr) {
  mark_state(sptr);
  for (transition_ptr tp = sptr->outgoing; tp != NULL; tp = tp->next) {
    if (!is_state_marked(tp->target)) mark_reachable_from_state(tp->target);
  }
}

void mark_reachable_from_initial(automaton_ptr aut) {
  mark_reachable_from_state(get_initial_state(aut));
}

automaton_ptr remove_unmarked_states(automaton_ptr aut) {

  automaton_ptr aut_rem = get_fresh_automaton();

  //first copy marked states
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) {
    if (is_state_marked(sptr)) {
      add_state(aut_rem, strdup(sptr->name));
    }
  }

  //then copy transition records
  state_ptr src = NULL;
  state_ptr trgt = NULL;
  for (transition_record_ptr trp = aut->transition_records; trp != NULL; trp = trp->next) {
    src = get_state_by_name(aut, trp->source);
    trgt = get_state_by_name(aut, trp->target);
    if (is_state_marked(src) && is_state_marked(trgt)) {
      add_transition_record(aut_rem, make_transition_record(strdup(trp->source),
                                                            strdup(trp->name),
                                                            strdup(trp->target)));
    }
  }

  //and generate incidence lists
  assert(collect_incidence_lists(aut_rem));

  return aut_rem;
}

void mark_states_with_root_active_actions(automaton_ptr root, automaton_ptr aut, synchro_array_ptr sarr) {
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) {
    for (transition_ptr tp = sptr->outgoing; tp != NULL; tp = tp->next) {

      // TODO - debug this part
      if (automaton_knows_transition(root, tp->name, NULL) && !is_action_local(root, tp->name, sarr)) {
        mark_state(sptr);
        break;
      }
    }
  }
}

void mark_automaton(automaton_ptr aut) {
  aut->flags |= AUTOM_MARKED;
}

void clear_automaton(automaton_ptr aut) {
  aut->flags &= ~AUTOM_MARKED;
}

bool is_automaton_marked(automaton_ptr aut) {
  return (aut->flags & AUTOM_MARKED) != 0 ;
}

void clear_network(automaton_ptr net) {

  while (net->prev != NULL) net = net->prev;

  while (net->next != NULL) {
    clear_automaton(net);
    net = net->next;
  }

}

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

    tp = st->incoming;
    while (tp != NULL) {
      tnxt = tp->next;
      free(tp->name);
      free(tp);
      tp = tnxt;
    }

    free(st);
    st = nexts;
  }

  transition_record_ptr tr = aut->transition_records;
  transition_record_ptr nextt = NULL;
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

  if (aut->flags & AUTOM_MARKED) printf("Marked ");
  else printf("Unmarked ");

  printf("automaton (%p) with states:\n", aut);
  state_ptr st = aut->states;
  while (st != NULL) {
    if (st->marked) printf("*");
    printf("%s ", st->name);
    st = st->next;
  }

  printf("\nand transition_records:\n");
  transition_record_ptr tr = aut->transition_records;
  if (tr == NULL) printf(" (empty)");
  while (tr != NULL) {
    printf("(%s, %s, %s)\n", tr->source, tr->name, tr->target);
    tr = tr->next;
  }

  printf("the incidence list:");
  if (aut->flags & AUTOM_INCIDENCE_OK) {
    print_incidence_list(aut);
  } else printf(" (not computed)");

  printf("\nsynchronises with:");
  if (aut->sync_links != NULL) {
    print_synchro_links(aut, MAIN_SYNCHRO_LINKS);
  } else printf(" (nothing)");

  printf("\nthe working synchronisation list:");
  if (aut->work_links != NULL) {
    print_synchro_links(aut, WORKER_SYNCHRO_LINKS);
  } else printf(" (empty or not computed)");

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

  sync_link_ptr linx = NULL;
  if (ptype == MAIN_SYNCHRO_LINKS) {
    linx = aut->sync_links;
  } else if (ptype == WORKER_SYNCHRO_LINKS) {
    linx = aut->work_links;
  }

  while (linx != NULL) {
    printf("\nautomaton %p via:\n", linx->other);
    for (int i = 0; i < linx->sync_action_ctr; ++i) {
      printf("%s ", linx->sync_action_names[i]);
    }

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

char* get_qualified_state_name(automaton_ptr aut, char* state_name) {

  char* state_str = (char*) malloc(MAXTOKENLENGTH * sizeof(char));
  sprintf(state_str, "%p_%s", aut, state_name);

  return state_str;
}

char* get_qualified_pair_name(automaton_ptr auta, char* state_namea,
                              automaton_ptr autb, char* state_nameb) {

  char* snamea = get_qualified_state_name(auta, state_namea);
  char* snameb = get_qualified_state_name(autb, state_nameb);
  strcat(snamea, "_");
  strcat(snamea, snameb);
  free(snameb);
  
  return snamea;
}


bool collect_incidence_lists(automaton_ptr aut) {

  aut->flags |= AUTOM_INCIDENCE_OK;

  for (transition_record_ptr pptr = aut->transition_records;
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

    transition_ptr trout = make_transition(strdup(pptr->name));
    transition_ptr trin = make_transition(strdup(pptr->name));
    trout->source = src;
    trin->source = src;
    trout->target = trg;
    trin->target = trg;

    //transitions that leave src
    if (!src->outgoing) src->outgoing = trout;
    else {
      src->outgoing->prev = trout;
      trout->next = src->outgoing;
      src->outgoing = trout;
    }

    //transitions that leave trg
    if (!trg->incoming) trg->incoming = trin;
    else {
      trg->incoming->prev = trin;
      trin->next = trg->incoming;
      trg->incoming = trin;
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

transition_record_ptr make_transition_record(char* source, char* name, char* target) {

   transition_record_ptr tr = malloc(sizeof(transition_record));
   tr->source = source;
   tr->name = name;
   tr->target = target;
   tr->next = NULL;

   return tr;
}

transition_ptr make_transition(char* trname) {

    transition_ptr tr = malloc(sizeof(transition));
    tr->next = NULL;
    tr->prev = NULL;
    tr->source = NULL;
    tr->target = NULL;
    tr->name = trname;

    return tr;
}

bool automaton_knows_transition(automaton_ptr aut, char* trans_name, synchro_array_ptr sarr) {

  if (aut == NULL) return false;

  for (transition_record_ptr ptr = aut->transition_records; ptr != NULL; ptr = ptr->next) {
    if (!strcmp(ptr->name, trans_name) &&
        (sarr == NULL || cstring_array_contains((char**) sarr->actions, sarr->ctr, trans_name))) return true;
  }

  return false;
}

state_ptr* get_states_with_enabled(automaton_ptr aut, char* trans_name, int* asize) {

  //TODO later: use proper wrappers for dynamic structures

  int SYNCTRANSIZE = 100;
  int scapacity = SYNCTRANSIZE;

  state_ptr* stt = malloc(SYNCTRANSIZE * sizeof(state_ptr));

  *asize = 0;
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) {
    for (transition_ptr tptr = sptr->outgoing; tptr != NULL; tptr = tptr->next) {
      if (!strcmp(trans_name, tptr->name)) {
        if ((*asize) >= scapacity) {
          grow_ref_array(&scapacity, (void**) &stt);
        }
        stt[(*asize)++] = sptr;
        break;
      }
    }
  }

  return stt;
}

int count_states(automaton_ptr aut) {

  int ctr = 0;
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next) ++ctr;

  return ctr;
}

int count_transitions(automaton_ptr aut) {
  automaton_ptr curr_aut = aut;
  int arcs = 0;

  while (curr_aut != NULL) {

    transition_record_ptr trans = curr_aut->transition_records;
    while (trans != NULL) {
      ++arcs;
      trans = trans->next;
    }

    curr_aut = curr_aut->next;
  }

  return arcs;
}

void sync_automata_one_way(automaton_ptr fst, automaton_ptr snd, synchro_array_ptr sarr) {

  bool common = false;
  for (transition_record_ptr ptr = snd->transition_records; ptr != NULL; ptr = ptr->next) {

    if (automaton_knows_transition(fst, ptr->name, sarr)) {
      common = true;
      break;
    }
  }
  if (!common) return;

  int SYNCINITSIZE = 1000; 
  sync_link_ptr new_connection = malloc(sizeof(sync_link));
  if (!new_connection) {
    perror("sync_automata_one_way: mem allocation error");
    exit(1);
  }

  new_connection->other = snd;
  new_connection->sync_action_ctr = 0;
  new_connection->sync_action_capacity = SYNCINITSIZE;
  new_connection->sync_action_names = malloc(SYNCINITSIZE * sizeof(char*));
  new_connection->next = NULL;
  new_connection->prev = NULL;

  for (transition_record_ptr ptr = snd->transition_records; ptr != NULL; ptr = ptr->next) {

    if (automaton_knows_transition(fst, ptr->name, sarr)) {
      if (new_connection->sync_action_ctr == new_connection->sync_action_capacity - 1) {
	grow_ref_array(&new_connection->sync_action_capacity, (void**)&new_connection->sync_action_names);
      }
      new_connection->sync_action_names[(new_connection->sync_action_ctr)++] = strdup(ptr->name);
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
    new_connection->prev = sp;
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
    if (sl->sync_action_names != NULL) free(sl->sync_action_names);
    sl = sl->next;
    if (hlp != NULL) free(hlp);
  }

}

void free_work_links(automaton_ptr aut) {

  sync_link_ptr hlp = NULL;
  sync_link_ptr sl = aut->work_links;

  while (sl != NULL) {
    hlp = sl;
    if (sl->sync_action_names != NULL) free(sl->sync_action_names);
    sl = sl->next;
    if (hlp != NULL) free(hlp);
  }

}

void copy_work_links(automaton_ptr aut) {

  sync_link_ptr lptr = NULL;
  sync_link_ptr bgnptr = NULL;

  for (sync_link_ptr link = aut->sync_links; link != NULL; link = link->next) {

    if (bgnptr == NULL) {
      lptr = malloc(sizeof(sync_link));
      lptr->prev = NULL;
      bgnptr = lptr;
    } else {
      lptr->next = malloc(sizeof(sync_link));
      lptr->next->prev = lptr;
      lptr = lptr->next;
    }

    lptr->other = link->other;
    lptr->sync_action_ctr = link->sync_action_ctr;
    lptr->sync_action_capacity = link->sync_action_capacity;
    lptr->sync_action_names = malloc(link->sync_action_capacity * sizeof(char*));
    lptr->next = NULL;

    for (int i = 0; i < link->sync_action_ctr; ++i) lptr->sync_action_names[i] = strdup(link->sync_action_names[i]);

  }

  aut->work_links = bgnptr;
}

void copy_work_links_network(automaton_ptr net) {

  while (net->prev != NULL) net = net->prev;

  while (net->next != NULL) {
    copy_work_links(net);
    net = net->next;
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

  fclose(yyin);

  return retv;
}

void relabel_net(automaton_ptr aut) {

  //todo: this is quite bad and really (~n^3 w.r.t. number of states...)
  //inefficient; redesign but maybe when rewriting in something else than C
  //quick fix for now: maybe add a fast map from state names to ints and
  //use binary search?
  
  //todo
  
  int ctr = 0;
  for (state_ptr sptr = aut->states; sptr != NULL; sptr = sptr->next, ++ctr) {
    for (transition_record_ptr tptr = aut->transition_records; tptr != NULL; tptr = tptr->next) {
      if (!strcmp(sptr->name, tptr->source)) {
	sprintf(tptr->source, "%d", ctr);
      }
      if (!strcmp(sptr->name, tptr->target)) {
	sprintf(tptr->target, "%d", ctr);
      }
    }
    sprintf(sptr->name, "%d", ctr);
  }

}

void automaton_to_dot(automaton_ptr aut, int automaton_ctr, FILE* dotf) {

  fprintf(dotf, "subgraph cluster%d {\n", automaton_ctr);
  char* color;
  state_ptr state = aut->states;
  if (state->marked) color = "pink"; else color = "yellow";
  fprintf(dotf, "N%dN%s [style = filled, color = %s, label = \"%s\" shape = invtriangle];\n", automaton_ctr, state->name, color, state->name);
  for (state = state->next; state != NULL; state = state->next) {
    if (state->marked) color = "pink"; else color = "yellow";
    fprintf(dotf, "N%dN%s [style = filled, color = %s, label = \"%s\"];\n", automaton_ctr, state->name, color, state->name);
  }

  for (state = aut->states; state != NULL; state = state->next) {

    for (transition_ptr trans = state->outgoing; trans != NULL; trans = trans->next) {
      fprintf(dotf, "N%dN%s -> N%dN%s [label = \"%s\"];\n", automaton_ctr, trans->source->name, automaton_ctr, trans->target->name, trans->name);
    }

  }

  fprintf(dotf, "label = \"automaton: %p\\n\\n\";\n", aut);
  fprintf(dotf, "labelloc = t;\n");
  fprintf(dotf, "}\n");
}

bool network_to_dot(automaton_ptr net, char* dotfname) {

  FILE* dotf = NULL;
  if ((dotf = fopen(dotfname, "w")) == NULL) {
    perror("can't write dot file");
    return false;
  }

  int mod_ctr = 0;
  while (net->prev != NULL) net = net->prev;

  fprintf(dotf, "digraph G {\n");
  fprintf(dotf, "compound = true;\n");
  while (net != NULL) {
    automaton_to_dot(net, mod_ctr++, dotf);
    net = net->next;
  }
  fprintf(dotf, "}\n");

  if (dotf != NULL) fclose(dotf);

  return true;
}

bool working_topology_to_dot(automaton_ptr net, char* dotfname) {

  FILE* dotf = NULL;
  if ((dotf = fopen(dotfname, "w")) == NULL) {
    perror("can't write dot file");
    return false;
  }

  while (net->prev != NULL) net = net->prev;

  fprintf(dotf, "graph Sync {\n");
  sync_link_ptr slp = NULL;

  while (net != NULL) {
    slp = net->work_links;

    while (slp != NULL) {
      fprintf(dotf, "A%p -- A%p [label = \"", net, slp->other);
      for(int i = 0; i < slp->sync_action_ctr; ++i) {
        fprintf(dotf, "%s ", slp->sync_action_names[i]);
      }
      fprintf(dotf, "\"];\n");
      slp = slp->next;
    }

    net = net->next;
  }
  fprintf(dotf, "}\n");

  if (dotf != NULL) fclose(dotf);
  return true;
}

