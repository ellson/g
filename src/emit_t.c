#include <stdio.h>
#include <stdlib.h>

#include "grammar.h"
#include "list.h"
#include "emit.h"

static void api_start_state_machine(context_t *C) {
    putc(char_prop(0,'_'), OUT);
}

static void api_start_state(context_t *C, char class, unsigned char prop, int nest, int repc) {
    putc('\n', OUT);
    putc(char_prop(prop,'_'), OUT);
    fprintf(OUT,"%4d%4d%4d ", class, nest, repc);
    print_string(NAMEP(class+state_machine));
}

static void api_string(context_t *C, elem_t *string, int slen) {
    putc('\t', OUT);
    print_list(OUT, string);
}

static void api_tok(context_t *C, char class, unsigned char len, unsigned char *frag) {
    putc('\t', OUT);
    putc('\t', OUT);
    print_frag(OUT, len, frag);
}

static void api_end_state(context_t *C, char class, int rc, int nest, int repc) {
    fprintf(OUT,"\n%d%4d%4d%4d ", rc, class, nest, repc);
    print_string(NAMEP(class+state_machine));
}

static void api_end_state_machine(context_t *C) {
    putc('\n', OUT);
}

static void api_error(context_t *C, char *message) {
    fprintf(OUT, "\nError: %s\n", message);
    exit(1);
}


static emit_t api = {
    /* api_start_state_machine */ api_start_state_machine,
    /* api_sep */                 NULL,
    /* api_start_state */         api_start_state,
    /* api_string */              api_string,
    /* api_frag */                NULL,
    /* api_tok */                 api_tok,
    /* api_end_state */           api_end_state,
    /* api_term */                NULL,
    /* api_end_state_machine */   api_end_state_machine,
    /* api_error */               api_error
};

emit_t *emit_t_api = &api;