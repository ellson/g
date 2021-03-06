/* vim:set shiftwidth=4 ts=8 expandtab: */

/*************************************************************************
 * Copyright (c) 2017 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: John Ellson <john.ellson@gmail.com>
 *************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "thread.h"
#include "container.h"
#include "info.h"
#include "process.h"

/**
 * @param THREAD context   
 * @return SUCCESS or FAIL
 */
success_t container(THREAD_t * THREAD)
{
    CONTAINER_t container = { 0 };
    elem_t *root;
    success_t rc;

    root = new_list(LIST(), ACTIVITY);

    container.THREAD = THREAD;

    container.previous = new_list(LIST(), SUBJECT);  // for sameas

    THREAD->stat_containdepth++;      // containment nesting level
    if (THREAD->stat_containdepth > THREAD->stat_containdepthmax) {
        THREAD->stat_containdepthmax = THREAD->stat_containdepth;
    }
    container.stat_containercount++;    // number of containers in this container

    if ((rc = parse(&container, root, 0, REP, 0, 0, END)) == FAIL) {
        if (TOKEN()->insi == END) {    // EOF is OK
            rc = SUCCESS;
        } else {
            token_error(TOKEN(), "Parse error near token:", TOKEN()->insi);
        }
    }

    // preserve in ikea storage
    if (container.nodes) {
        IO()->out_disc = &ikea_disc;
        IO()->out_chan = IO()->out_disc->out_open_fn( THREAD->PROCESS->ikea_store, NULL);
        printt(IO(), container.nodes);
        if (container.edges) {
            printt(IO(), container.edges);
        }
        IO()->out_disc->out_flush_fn(IO());
        IO()->out_disc->out_close_fn(IO());
    }

    THREAD->stat_containdepth--;

    free_list(LIST(), root);
    free_list(LIST(), container.nodes);
    free_list(LIST(), container.edges);
    free_list(LIST(), container.previous);

// Some elem's are retained by the attrid tree
//E();

    // FIXME - move to Aunt Sally query
    if (THREAD->PROCESS->flags & 4) {
        // in alpha-sorted order
        info_container(&container);
        info_process(THREAD);
        info_thread(THREAD);
    }

    return rc;
}

/**
 * @param THREAD context   
 * @param node with kid
 * @return node with kid in playpen
 */
elem_t * playpen(THREAD_t * THREAD, elem_t *node)
{
    elem_t * sis = node->u.l.first;
    elem_t * kid = sis->u.l.next; // might be a port

    if (kid) {
        elem_t * newnode = new_list(LIST(), NODE);
        elem_t * newsis = new_list(LIST(), SIS);
        append_transfer(newnode, newsis);
        switch ((state_t)kid->state) {
            case KID:   // drop KID from NODE but put into a playpen
                append_addref(newsis, sis->u.l.first);
                // FIXME playpen recursion
                break;
            case PORT:  // drop PORT from NODE, whether induced or not
                append_addref(newsis, sis->u.l.first);
                break;
            default:
                assert(0);
        }
        free_list(LIST(), node);
        return newnode;
    }
    return node;
}

/**
 * @param THREAD context   
 * @param node with kid
 * @return node with kid in playpen
 */
elem_t * playpen2(THREAD_t * THREAD, elem_t *subj)
{
//P(subj);
#if 0
    elem_t *node = subj->u.l.first->u.l.first;
    elem_t *sis = node->u.l.first;
    elem_t *kid = sis->u.l.next;
    if (kid) {
        elem_t * newsubj = new_list(LIST(), SUBJECT);
        elem_t * newnode = new_list(LIST(), NODE);
        append_transfer(newsubj, newnode);
        elem_t * newendpoint = new_list(LIST(), ENDPOINT);
        append_transfer(newnode, newendpoint);
        elem_t * newsis = new_list(LIST(), SIS);
        append_transfer(newendpoint, newsis);
        switch ((state_t)kid->state) {
            case KID:   // drop KID from NODE but put into a playpen
                append_transfer(newsis, sis->u.l.first);
                // FIXME playpen recursion
                break;
            case PORT:  // drop PORT from NODE, whether induced or not
                append_transfer(newsis, sis->u.l.first);
                break;
            default:
                assert(0);
        }
        return newsubj;
//P(newsubf);
    }
#endif
    return subj;
}
