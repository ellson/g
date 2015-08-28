#include "libje_private.h"

// This parser recurses at two levels:
//
//     main() -----> je_parse(C) ----> je_parse_r(CC) -| -|  
//                     ^                 ^             |  |
//                     |                 |             |  |
//                     |                 -------<-------  |
//                     |                                  |
//                     ---------------<--------------------
//
// The inner recursions are through the grammar state_machine at a single
// level of containment - maintained in container_context (CC)
//
// The outer recursionsi are through nested containment.
// The top-level context (C) is available to both and maintains the input state.

static success_t more_rep(context_t * C, unsigned char prop)
{
	state_t ei, bi;

	if (!(prop & (REP | SREP)))
		return FAIL;

	ei = C->ei;
	if (ei == RPN || ei == RAN || ei == RBR || ei == RBE) {
		return FAIL;	// no more repetitions
	}
	bi = C->bi;
	if (bi == RPN || bi == RAN || bi == RBR || bi == RBE
	    || (ei != ABC && ei != AST && ei != DQT)) {
		return SUCCESS;	// more repetitions, but additional WS sep is optional
	}
	if (prop & SREP) {
		emit_sep(C);	// sep is non-optional, emit the minimal sep
	}
	return SUCCESS;		// more repetitions
}

static success_t
je_parse_r(container_context_t * CC, elem_t * root,
	state_t si, unsigned char prop, int nest, int repc)
{
	unsigned char nprop;
	char so;		// offset to next state, signed
	state_t ti, ni;
	success_t rc;
	elem_t *elem;
	elem_t branch = { 0 };
	context_t *C = CC->context;
    unsigned long hash;
	static unsigned char nullstring[] = { '\0' };

	rc = SUCCESS;
	emit_start_state(CC, si, prop, nest, repc);
	branch.state = si;

	nest++;
	assert(nest >= 0);	// catch overflows

	if (!C->inbuf) {	// state_machine just started
		C->bi = WS;	    // pretend preceeded by WS to satisfy toplevel SREP or REP
		                // (Note, first REP of a sequence *can*
		                // be preceeded by WS, just not the
		                // rest of the REPs. )
		C->in = nullstring;	// fake it;
		C->insi = NLL;	// pretend last input was the EOF of
		                // a prior file.
	}

	// Entering state
	C->state = si;		// record of last state entered, for error messages.

	// deal with "terminal" states: Whitespace, Tokens, and Contained activity, Strings

	C->ei = C->insi;	// the char class that ended the last token

	// Whitespace
	if ((rc = je_parse_whitespace(C)) == FAIL) {
		goto done;	// EOF during whitespace
	}

	// Special character tokens
	if (si == C->insi) {	// single character terminals matching state_machine expectation
		C->bi = C->insi;
		rc = je_parse_token(C);
		C->ei = C->insi;
		goto done;
	}
	switch (si) {
	case ACTIVITY:          // Recursion into Contained activity
		if (C->bi == LBE) {	// if not top-level of containment
			C->bi = NLL;
			rc = je_parse(CC->context, &CC->subject);	// recursively process contained ACTIVITY in to its own root
			C->bi = C->insi;	// The char class that terminates the ACTIVITY
			goto done;
		}
		break;

	case STRING:            // Strings
		rc = je_parse_string(C, &branch);
		C->bi = C->insi;	// the char class that terminates the STRING
		goto done;
		break;

	case VSTRING:            // Value Strings
		rc = je_parse_vstring(C, &branch);
		C->bi = C->insi;	// the char class that terminates the VSTRING
		goto done;
		break;

		// the remainder of the switch() is just state initialization and emit hooks;
	case SUBJECT:
		// This is a bit ugly.
		//
		// Because the grammar has no mandatory terminal token for ACTs, the 
		// only time we can be sure that the old ACT is finished is when there
		// is enough input stream to determine that a new ACT has started.
		//
		// This is not a problem for patterns, since they are not used until they are
		// matched to a later SUBJECT anyway.  Patterns in the last ACT of input just aren't
		// useful.

		if (CC->is_pattern) {   // flag was set by SUBJECT in previous ACT
			                    //  save entire previous ACT in a list of pattern_acts
			C->stat_patterncount++;
			elem = ref_list(C, root);

			if (CC->subject_type == NODE) {
				append_list(&(CC->node_pattern_acts), elem);
			} else {
				assert(CC->subject_type == EDGE);
				append_list(&(CC->edge_pattern_acts), elem);
			}
		} else {
			C->stat_actcount++;
		}

        // dispatch events for the ACT just finished
        je_dispatch(CC, root);

		free_list(C, root);	// now we're done with the last ACT
		                    // and we can really start on the new ACT
        C->verb = 0;        // initialize verb to default "add"   // FIXME - haven't we passed VERB already?
		C->has_ast = 0;     // maintain a flag for an '*' found anywhere in the subject
		break;
	default:
		break;
	}

	// If it wasn't a terminal state, then use the state_machine to
	// iterate through ALTs or sequences, and then recursively process next the state

	rc = FAIL;		// init rc to FAIL in case no ALT is satisfied
	ti = si;
	while ((so = state_machine[ti])) {	// iterate over ALTs or sequences
		nprop = state_props[ti];    	// get the props for the transition
                                        // from the current state (OPT, ALT, REP etc)

		                                // at this point, ni is a signed, non-zero
                                        // offset to the next state
		ni = ti + so;               	// we get to the next state by adding the
                                        // offset from the current state.

		if (nprop & ALT) {              // look for ALT
			if ((rc = je_parse_r(CC, &branch, ni, nprop, nest, 0)) == SUCCESS) {
				break;                  // ALT satisfied
			}

			                            // we failed an ALT so continue iteration to try next ALT
		} else {                    	// else it is a sequence (or the last ALT, same thing)
			repc = 0;
			if (nprop & OPT) {          // OPTional
				if ((je_parse_r(CC, &branch, ni, nprop, nest, repc++)) == SUCCESS) {
					while (more_rep(C, nprop) == SUCCESS) {
						if (je_parse_r(CC, &branch, ni, nprop, nest, repc++) == FAIL) {
							break;
						}
					}
				}
			} else {                	// else not OPTional
				if ((rc = je_parse_r(CC, &branch, ni, nprop, nest, repc++)) == FAIL) {
					break;
				}
				while (more_rep(C, nprop) == SUCCESS) {
					if ((rc = je_parse_r(CC, &branch, ni, nprop, nest, repc++)) == FAIL) {
						break;
					}
				}
			}
		}
		ti++;		// next ALT (if not yet satisfied), or next sequence item
	}

 done: // State exit processing
	if (rc == SUCCESS) {
		switch (si) {
		case TLD:
		case QRY:
            C->verb = si;  // record verb prefix, if not default
            break;
		case HAT:
            je_persist_snapshot(C);
            break;
		case SUBJECT: // subject rewrites before adding branch to root
            branch.state = si;

			// Perform EQL "same as in subject of previous ACT" substitutions
			// Also classifies ACT as NODE or EDGE based on SUBJECT
			je_sameas(CC, &branch);

            je_hash_list(&hash, &(CC->subject));   // generate name hash
            elem = je_hash_bucket(C, hash);    // save in bucket list 

			// If this subject is not itself a pattern, then
            // perform pattern matching and insertion if matched
			if (!(CC->is_pattern = C->has_ast)) {
				je_pattern(CC, root, &branch);
			}

			emit_subject(CC, &branch);      // emit hook for rewritten subject
			break;
		case ATTRIBUTES:
			emit_attributes(CC, &branch);   // emit hook for attributes
			break;
        default:
            break;
        }
		if (branch.u.list.first != NULL || si == EQL) {	// mostly ignore empty lists
            branch.state = si;
			elem = move_list(C, &branch);
			append_list(root, elem);
		}
	}
	nest--;
	assert(nest >= 0);
	emit_end_state(CC, si, rc, nest, repc);

	return rc;
}

success_t je_parse(context_t * C, elem_t * name)
{
	container_context_t container_context = { 0 };
	container_context_t *CC = &container_context;
	elem_t root = { 0 };	// the output parse tree
    elem_t *elem;
	success_t rc;
    unsigned long hash;
    char *hashname, *filename;

	CC->context = C;
    C->sep = ' ';

    je_hash_list(&hash, name);         // hash name (subject "names" can be very long)
    elem = je_hash_bucket(C, hash);    // save in bucket list 
    if (! elem->u.hash.out) {          // open file, if not already open
        hashname = je_long_to_base64(&hash);
        if (! (filename = malloc(strlen(C->tempdir) + 1 + strlen(hashname) + 1))) {
            perror("Error - malloc(): ");
            exit(EXIT_FAILURE);
        }
        strcpy(filename, C->tempdir);
        strcat(filename, "/");
        strcat(filename, hashname);
        elem->u.hash.out = fopen(filename,"a+b"); //open for binary append writes, + read.
        if (! elem->u.hash.out) {
            perror("Error - fopen(): ");
            exit(EXIT_FAILURE);
        }
        free(filename);
    }
    CC->out = elem->u.hash.out;

	C->stat_containercount++;

	emit_start_activity(CC);
	if ((rc = je_parse_r(CC, &root, ACTIVITY, SREP, 0, 0)) != SUCCESS) {
		if (C->insi == NLL) {	// EOF is OK
			rc = SUCCESS;
		} else {
			emit_error(C, C->state, "Parse error. Last good state was:");
		}
	}
	emit_end_activity(CC);

	free_list(C, &root);
	free_list(C, &(CC->subject));
	free_list(C, &(CC->node_pattern_acts));
	free_list(C, &(CC->edge_pattern_acts));

	return rc;
}
