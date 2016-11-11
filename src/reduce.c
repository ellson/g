/* vim:set shiftwidth=4 ts=8 expandtab: */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "thread.h"
#include "merge.h"
#include "reduce.h"

void reduce(CONTAINER_t * CONTAINER, elem_t *list)
{
    THREAD_t * THREAD = CONTAINER->THREAD;
    state_t si, subjtype = 0;
    elem_t *elem, *subject, *attributes = NULL, *disambig = NULL;

    assert(list);

//E();
P(list);

    elem = list->u.l.first;
    assert(elem); // must always be a subject
    si = (state_t) elem->state;
    switch (si) {
    case NODE:
    case SIS:
        subject = elem;
        subjtype = si; // NODE or SIS
        break;
    case EDGE:
        subject = elem->u.l.first;  // ENDPOINTS (legs)
        disambig = subject->u.l.next; // DISAMBIG (may be NULL)
        subjtype = si; // EDGE
        break;
    default:
        S(si);
        assert(0); // SUBJECT must be NODE,SIS, or EDGE
        break;
    }
    elem = elem->u.l.next;
    if (elem) {
        si = (state_t) elem->state;
        switch (si) {
        case ATTRIBUTES:
            attributes = elem;
            break;
        default:
            S(si);
            assert(0); // that should be all
            break;
        }
    }
    assert(elem->u.l.next == NULL);

    switch (subjtype) {
    case NODE:
        CONTAINER->nodes = insert_item(LIST(),
                CONTAINER->nodes,
                &(subject->u.l.first), // skip NODEID
                merge_key); 
        break;
    case SIS:
        CONTAINER->nodes = insert_item(LIST(),
                CONTAINER->nodes,
                &(subject->u.l.first->u.l.first), // skip NODEREF NODEID
                merge_key); 
        break;
    case EDGE:
        CONTAINER->edges = insert_item(LIST(),
                CONTAINER->edges,
                &(subject),
                merge_key); 
        break;
    default:
        S(subjtype);
        assert(0); // that should be all
        break;
    }
    if (disambig) {
        //FIXME - what to do with this?
    }
    if (attributes) {
        //FIXME - what to do with this?
    }

//E();
}
