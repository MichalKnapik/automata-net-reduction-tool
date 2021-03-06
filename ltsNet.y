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
  transition_record_ptr tranptr;
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
   automaton_ptr ap = get_fresh_automaton();
   ap->states = $2;
   ap->transition_records = $4;
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
    state_ptr st = make_state($1);
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
   transition_record_ptr tr = make_transition_record($2, $4, $6);
   $$ = tr;
 }
;

%%

void yyerror(char* s) {
    printf("PARSER ERROR in line %d\n", yylineno);
    }
