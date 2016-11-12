/* vim:set shiftwidth=4 ts=8 expandtab: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "libg_session.h"

// if interrupted we try to gracefully snapshot the current state 
static void intr(int s)
{
    (void) s; // NOTUSED

#if 0
// FIXME - once we know what we  really want from command line interrupts
    interrupt(THREAD);
#endif
    exit (EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt, optnum, needstats = 0;

    signal(SIGINT, intr);

    while ((opt = getopt(argc, argv, "d::s")) != -1) {
        if (optarg)
            optnum = atoi(optarg);
        else
            optnum = 0;
        switch (opt) {
#if 0
        case 'T':
            if (select_emitter(optarg) != 0) {
                fprintf(stderr, "No back-end found for format: -T%s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
#endif
        case 'd':
            switch (optnum) {
            case 0:
                dumpg();
                exit(EXIT_SUCCESS);
                break;
            case 1:
                printg();
                exit(EXIT_SUCCESS);
                break;
            default:
                fprintf(stderr,"%s\n", "-d0 = linear walk, -d1 = recursive walk");
                exit(EXIT_FAILURE);
                break;
            }

            break;
        case 's':
            needstats = 1;
            break;
        default:

// FIXME no -g or -t support any more ??
// FIXME need something like -p  (print primary content)
// FIXME need something like -P "name" (print content of "name")
// FIXME "name" should allow full subject string, or "hashname"
// FIXME need something like -i (print tree index of container "hashname" and "SUBJECT")
// FIXME - add -T options to usage message
// FIXME - do -i processing here so can index restored pre-parsed graphs
// FIXME - allow multiple -p in loop
// FIXME - do -p / -P processing here so can print resored pre-parsed graphs
//
//            fprintf(stderr,"Usage: %s [-d[01] | [-s] [-t[01]] | [-g[01]] [files] [-]  \n", argv[0]);
            fprintf(stderr,"Usage: %s [-d[01] [-s] [files] [-]\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    // create the top-level context for processing the inputs
    session(&argc, argv, optind, needstats);

    exit(EXIT_SUCCESS);
}




/*
 * Usage:  g ARGS?
 *
 *    ARGS       ::= HELP | VERSION | THREADSPEC*
 *         default ARGS is:  -Qg -
 *    HELP       ::= '-?'
 *    VERSION    ::= '-V'
 *    THREADSPEC ::= OUTPUTSPEC* INPUTSPEC+
 *    OUTPUTSPEC ::= QUERYOUT | PARSEROUT | CONTENTOUT
 *    QUERYOUT   ::= '-Qg' | '-Qgv'
 *    PARSEROUT  ::= '-Pg' | '-Pgv'
 *    CONTENTOUT ::= TRANSLATOR LAYOUT?
 *    FORMAT     ::= '-Tgv' | '-Tsvg' | .... any supported by dot
 *    LAYOUT     ::= '-Kdot' | '-Kfdp' | .... any supported by dot
 *    INPUTSPEC  ::= INPARALLEL? FILE
 *    INPARALLEL ::= '+'
 *    FILE       ::= '-' | filename
 *
 *
 *    The default, if no ARGS provided, is equivalent to:
 *        g -Qg -
 *    which runs until stdin is closd, accumulating contents, ands
 *    outputting query results to stdout
 *
 *    Examples:
 *        g -?                -- this help text
 *        g -V                -- version info
 *        g -Pg               -- syntax checker, pretty printer
 *        g -Pgv              -- converter to DOT format
 *        g -Qg               -- accumulate content and output query results in g format to stdout
 *        g -Qgv              -- accumulate content and output query results in DOT format to stdout
 *        g -Qg +:8080 +-     -- accumulate content in parallel from port 8080 and stdin
 *                               and output query results in g format to stdout
 *
 *        g                   -- equivalent to:  g -Qg - -
 */

/* Usage:   g <args>
 *
 * Where <args> is a list of options as NODES in g syntax:
 *
 * args ::= '-?;
 *          '-V'
 *          '-P' [fmt= out=]
 *          '-Q' [fmt= out=]
 *          '-C' [fmt= out= lay=]
 *          '-i' {in in in ...}
 *          '+i' {in in in ...}
 *          '-F' {args}
 *
 * defaults:
 *         args:  -Q[fmt=g out=-]
 *         fmt:   g
 *         lay:   fdp
 *         out:   -    
 *         in:    -
 */
