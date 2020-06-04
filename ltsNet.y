%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "automata_interface.h"
#include "common.h"

extern int yylineno;
extern automaton_ptr root;
%}

/* token value type: for future extensions */
%union {
  char* stringval;
  automaton_ptr automptr;
  parsed_transition_ptr tranptr;
  state_ptr stateptr;
}

%token STATES
%token TRANSITIONS
%token OPENBR
%token CLOSEBR
%token ERROR
%token CMM
%token ALPHASTRING

%type<tranptr> trans
%type<tranptr> translist
%type<stateptr> state
%type<stateptr> statelist
%type<stringval> ALPHASTRING
%type<automptr> component

%start component

/* grammar rules */
%%

component: STATES statelist TRANSITIONS translist
 {
   automaton_ptr ap = (automaton_ptr) malloc(sizeof(automaton));
   ap->states = $2;
   ap->parsed_transitions = $4;
   ap->flags = AUTOM_NONE;
   ap->next = NULL;
   ap->prev = NULL;
   ap->sync_links = NULL;
   ap->work_links = NULL;   
   $$ = ap;
   root = ap;
 }
;

statelist: state statelist
  {
    $1->next = $2;
    $$ = $1;
  }
| state { $$ = $1; }
;

state: ALPHASTRING
  {
    state_ptr st = (state_ptr) malloc(sizeof(state));
    st->name = $1;
    st->next = NULL;
    st->outgoing = NULL;
    $$ = st;
  }
;

translist: trans translist
 {
   $1->next = $2;
   $$ = $1;
 }
| trans {$$ = $1;}
;

trans: OPENBR ALPHASTRING CMM ALPHASTRING CMM ALPHASTRING CLOSEBR
 {
   parsed_transition_ptr tr = (parsed_transition_ptr) malloc(sizeof(parsed_transition));
   tr->source = $2;
   tr->name = $4;
   tr->target = $6;
   tr->next = NULL;
   $$ = tr;
 }
;

%%

void yyerror(char* s) {
    printf("PARSER ERROR in line %d\n", yylineno);
    }
