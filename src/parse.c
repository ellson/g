#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "grammar.h"
#include "inbuf.h"
#include "list.h"
#include "context.h"
#include "stats.h"
#include "emit.h"
#include "parse.h"
#include "token.h"

static success_t more_rep(context_t *C, unsigned char prop) {
    state_t ei, bi;

    if (! (prop & (REP|SREP))) return FAIL;
  
    ei = C->ei;
    if (ei == RPN || ei == RAN || ei == RBR || ei == RBE ) {
        return FAIL;           // no more repetitions
    }
    bi = C->bi;
    if (bi == RPN || bi == RAN || bi == RBR || bi == RBE || (ei != ABC && ei != AST)) {
        return SUCCESS;        // more repetitions, but additional WS sep is optional
    }
    if (prop & SREP) {
        emit_sep(C);           // sep is non-optional, emit the minimal sep
    }
    return SUCCESS;            // more repetitions
}

static success_t parse_activity(context_t *C); // forward declaration for recursion

static success_t parse_r(container_context_t *CC, elem_t *root, 
		 state_t si, unsigned char prop, int nest, int repc) {
    unsigned char nprop;
    char so;                  // offset to next state, signed
    state_t ti, ni;
    success_t rc;
    elem_t *elem;
    elem_t branch = {0};
    context_t *C;
    static unsigned char nullstring[] = {'\0'};

    C = CC->context;

    rc = SUCCESS;
    emit_start_state(C, si, prop, nest, repc);
    branch.state = si;

    nest++;
    assert (nest >= 0);        // catch overflows

    if (! C->inbuf) {          // state_machine just started
        C->bi = WS;           // pretend preceeded by WS
			       // to satisfy toplevel SREP or REP
			       // (Note, first REP of a REP sequence *can* be preceeded by WS,
		               //      just not the rest of the REPs. )
        C->in = nullstring;    // fake it;
        C->insi = NLL;         // pretend last input was a terminating NLL
    }

    // deal with "terminal" states: whitespace, string, token, and contained activity
    
    C->ei = C->insi;          // the char class that ended the last token
    if ( (rc = parse_whitespace(C)) == FAIL ) {
	goto done;             // EOF during whitespace
    }
    if (si == C->insi) {       // single character terminals matching state_machine expectation
	C->bi = C->insi;
        rc = parse_token(C);
	C->ei = C->insi;
        goto done;
    }
    switch (si) {
    case ACTIVITY:
	if (C->bi == LBE) {   // if not top-level of containment
            C->bi = NLL;
            rc = parse_activity(C);// recursively process contained ACTIVITY in to its own root
            C->bi = C->insi;  // the char class that terminates the ACTIVITY
            goto done;
        }
	break;
    case ACT:
#ifdef EMIT_TERM
        if (CC->unterm) {             // implicitly terminates preceeding ACT
 	    emit_term(C);
	}
	CC->unterm = 1;               // indicate that this new ACT is unterminated
#endif
        emit_start_act(C);
	break;
    case STRING:
        rc = parse_string(C, &branch);
        C->bi = C->insi;          // the char class that terminates the STRING
        goto done;
	break;
    case SUBJECT:
        emit_start_subject(C);
	CC->subj = 0;
	break;
    case ATTRIBUTES:
        emit_start_attributes(C);
	break;
    case CONTAINER:
        emit_start_container(C);
        C->containment++;
	break;
    default:
	break;
    }

    // Use state_machine[si] to determine next state
 
    rc = FAIL;                    // init rc to FAIL in case no ALT is satisfied
    ti = si;
    while (( so = state_machine[ti] )) { // iterate over ALTs or sequences
        nprop = state_props[ti];  // get the props for the transition from the current state (OPT, ALT, REP etc)
	                          // at this point, ni is a signed, non-zero offset to the next state
        ni = ti + so;             // we get to the next state by adding the offset to the current state.
	if (nprop & ALT) {        // look for ALT
	    if (( rc = parse_r(CC, &branch, ni, nprop, nest, 0)) == SUCCESS) {
                break;            // ALT satisfied
	    }
	                          // we failed an ALT so continue iteration to try next ALT
	} 
	else {                    // else it is a sequence
	    repc = 0;
	    if (nprop & OPT) {    // optional
	        if (( parse_r(CC, &branch, ni, nprop, nest, repc++)) == SUCCESS) {
	            while (more_rep(C, nprop) == SUCCESS) {
                        if (parse_r(CC, &branch, ni, nprop, nest, repc++) == FAIL) {
			    break;
			}
		    }
	        }
	    }
	    else {                // else not OPTional
	        if (( rc = parse_r(CC, &branch, ni, nprop, nest, repc++)) == FAIL) {
		    break; 
		}
                // A 1-or-more repetition is successful if the first one was a success
	        while (more_rep(C, nprop) == SUCCESS) {
                    if (( rc = parse_r(CC, &branch, ni, nprop, nest, repc++)) == FAIL) {
			break;
		    }
		}
	    }
	}
	ti++;                     // next ALT (if not yet satisfied), or next sequence item
    }

    // Any subtree rewrites or emit before adding branch to root in the state exit processing
    if (rc == SUCCESS) {
        switch (si) {
	case ACT:
            stat_actcount++;
            emit_act(C, &branch);
            emit_end_act(C);
#if 0
            pop_list(&(C->subject));  // discard the subject of this act at this level of containment
            free_list(&branch);
#endif
            break;
        case SUBJECT:
            // update samends
            //    -- free old samends
	    free_list(&(CC->prev_subject));
//            elem = ref_list(si, &branch);
//    append_list(&(CC->prev_subject), elem);

            emit_subject(C, &branch);
            emit_end_subject(C);
#if 0
putc ('\n', stdout);
print_list(stdout, &(C->subject), 0, ' ');
putc ('\n', stdout);
#endif
	    break;
        case ATTRIBUTES :
            emit_attributes(C, &branch);
            emit_end_attributes(C);
            break;
        case CONTAINER:
            C->containment--;
            emit_end_container(C);
	    break;
        case LEG:
#if 0
	    if (bi == EQL) {
                if (! sameend_elem) {
	            emit_error(C, si, "No prior LEG found for sameend substitution in");
	        }
//		elem = ref_list(si, elem);

                elem = ref_list(si, sameend_elem);
// FIXME can be multiple ENDPOINTS in a LEG, need a while here
//                append_list(&branch, sameend_elem->u.list.first);
            }
            if (sameend_elem) {
	        sameend_elem = sameend_elem -> next;
            }
#endif
	    break;
        default:
	    break;
	}
    }

    // State exit processing

done:
    if (rc == SUCCESS) {             
        if (branch.u.list.first != NULL) { // ignore empty lists
            elem = move_list(si, &branch);
            append_list(root, elem);
    //putc ('\n', stdout);
    //print_list(stdout, elem, 0, ' ');
    //putc ('\n', stdout);
    //    free_list(&sameend_legs);      // free old sameend list
    //    elem = ref_list(si, root->u.list.last);  // replace with new list, fully substituted.
    
    //    append_list(&(C->sameend_legs_new), elem);
    
    // putc ('\n', stdout);
    // print_list(stdout, &new_sameend_legs, 0, ' ');
    // putc ('\n', stdout);
            switch (si) {
            case SUBJECT :
            // update samends
            //    -- free old samends
//	    free_list(&(CC->prev_subject));
                emit_end_subject(C);
                break;
            case ATTRIBUTES :
                emit_end_attributes(C);
                break;
            case CONTAINER :
                stat_containercount++;
                emit_end_container(C);
#ifdef EMIT_TERM
                if (CC->unterm) {
                    emit_term(C);
                }
                CC->unterm = 0;
#endif
                break;
#ifdef EMIT_TERM
            case TERM :   
                if (CC->unterm) {
                     emit_term(C);
                }
                CC->unterm = 0;
                break;
#endif
            case EDGE :  // SUBJECTS that start with an EDGE must have only EDGEs
                if (CC->subj == 0) {
                    CC->subj = EDGE;
                }
                else {
                    if (CC->subj == NODE) {
                        emit_error(C, si, "NODE subject includes");
                    }
                }
                break;
            case NODE : // SUBJECTS that start with a NODE must have only NODEs
                if (CC->subj == 0) {
                    CC->subj = NODE;
                }
                else {
                    if (CC->subj == EDGE) {
                        emit_error(C, si, "EDGE subject includes");
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    nest--;
    assert (nest >= 0);
    emit_end_state(C, si, rc, nest, repc);

    return rc;
}

static success_t parse_activity(context_t *C) {
    success_t rc;
    elem_t root = {0};   // the output parse tree
    container_context_t container_context = {0};

    container_context.context = C;
    container_context.out = stdout;
    container_context.err = stderr;

    C->container_context = &container_context;

    emit_start_activity(C);

    if ((rc = parse_r(&container_context, &root, ACTIVITY, SREP, 0, 0)) != SUCCESS) {
        if (C->insi == NLL) { // EOF is OK
            rc = SUCCESS;
        }
        else {
            emit_error(C, ACTIVITY, "Parse error");
        }
    }

#ifdef EMIT_TERM
    if (container_context.unterm) {
 	emit_term(C);      // end_of_activity (EOF or end_of_container) is an implicit terminator
    }
    // no nead to update .unterm as we're leaving this context now.
#endif

    free_list(&container_context.prev_subject);
    free_list(&container_context.pattern_acts);

    emit_end_activity(C);
    return rc;
}

success_t parse(int *pargc, char *argv[], FILE *out, FILE *err) {
    success_t rc;
    context_t context = {0};   // the input context

    context.pargc = pargc;
    context.argv = argv;
    context.out = out;
    context.err = err;

    emit_start_parse(&context);
    rc = parse_activity(&context);
    emit_end_parse(&context);

    return rc;
}
