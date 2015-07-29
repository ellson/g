#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "emit.h"
#include "grammar.h"
#include "list.h"
#include "parse.h"

#define OUT stdout
#define ERR stderr

#define styleLAN sstyle?"< ":"<"
#define styleRAN sstyle?"\n>\n":">"
#define styleLPN sstyle?"\n  ( ":"("
#define styleRPN sstyle?"\n  )\n":")"
#define styleLBR sstyle?"\n  [ ":"["
#define styleRBR sstyle?"\n  ]\n":"]"
#define styleLBE sstyle?"\n  { ":"{"
#define styleRBE sstyle?"\n  }\n":"}"

static int sstyle=0;

void set_sstyle (void) { 
    sstyle=1;
}

static char *oleg1=NULL, *oleg2=NULL;
static char *sleg1, *sleg2;

static void print_next( char *leg1, char *leg2 ) {
    if (leg1 != oleg1) {
	oleg1 = leg1;
        sleg1 = NAMEP(leg1);

        oleg2 = NULL;
    }
    else {
	sleg1 = "=";
    }
    if (leg2 != oleg1) {
	oleg2 = leg2;
        sleg2 = NAMEP(leg2);
    }
    else {
	sleg2 = "=";
    }
    fprintf(OUT,"%s%s %s%s", styleLAN, sleg1, sleg2, styleRAN);
}

static void print_attr ( char attr, char *attrid, int *inlist ) {
    if (attr) {
	if ((*inlist)++) putc (' ', OUT);
        fprintf(OUT,attrid);
    }
}

static void print_prop(char *p) {
    unsigned char prop;
    int inlist;

    prop = *PROPP(p);
    if (prop & (ALT|OPT|SREP|REP)) {
        inlist=0;
        fprintf(OUT,"%s", styleLBR);
        print_attr( prop & ALT, "ALT", &inlist);
        print_attr( prop & OPT, "OPT", &inlist);
        print_attr( prop & SREP, "SREP", &inlist);
        print_attr( prop & REP, "REP", &inlist);
        fprintf(OUT,"%s", styleRBR);
    }
}

static void printg_r(char *sp, int indent) {
    char *p, *np, nxt;
    int i;

    p = sp;
    while (( nxt = *p )) {
        np = p+nxt;
        for (i = indent; i--; ) putc (' ', OUT);
        print_next(sp, np);
        print_prop(p);
        fprintf(OUT,"%s\n", styleLBE);
	if (np != state_machine) { // stop recursion
            printg_r(np, indent+2);
        }
        for (i = indent; i--; ) putc (' ', OUT);
        fprintf(OUT,"%s\n", styleRBE);

        p++;
    }
}

// recursively walk the grammar - tests all possible transitions
void printg (void) {
    printg_r(state_machine, 0);
}

static void print_chars ( char *p ) {
    int i, cnt, si;

    si = (p - state_machine);
    cnt=0;
    for (i=0; i<0x100; i++) {
        if (si == char2state[i]) {
	    if (cnt++) {
	        putc (' ', OUT);
	    }
	    else {
		fprintf(OUT,"%s",styleLBE);
	    }
            fprintf(OUT,"%02x", i);
	}
    }
    if (cnt++) {
	fprintf(OUT,"%s",styleRBE);
    }
}

// just dump the grammar linearly,  should result in same logical graph as printg()
void dumpg (void) {
    char *p, *sp, nxt;

    p = state_machine;
    while (p < (state_machine + sizeof_state_machine)) {
        if (*p) { // non-terminal
            sp = p;
            while (( nxt = *p )) {
                print_next(sp, p+nxt);
                print_prop(p);
		p++;
	    }
	}
	else { // else terminal
	    fprintf(OUT,"%s", NAMEP(p));
            print_chars(p);
	}
        p++;
        fprintf(OUT,"\n");
    }
}