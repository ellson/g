/* vim:set shiftwidth=4 ts=8 expandtab: */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "thread.h"
#include "compare.h"

#define MAXNEST 20

typedef struct {
    elem_t *next[MAXNEST];
    unsigned char *cp;
    uint16_t nest;
    uint16_t len;
} iter_t;

static elem_t * next(iter_t *iter, elem_t *elem)
{
    static unsigned char nullstr[] = {'\0'};

    assert(elem);
    assert(iter->nest < MAXNEST);

    switch (elem->type) {
        case FRAGELEM:
            iter->cp = elem->u.f.frag;
            iter->len = elem->len;
            elem = elem->u.f.next;
            break;
        case SHORTSTRELEM:
            iter->cp = elem->u.s.str;
            iter->len = elem->len;
            elem = NULL;
            break;
        case LISTELEM:
            iter->cp = nullstr;
            iter->len = sizeof(nullstr);
            iter->next[(iter->nest)++] = elem->u.l.next;
            elem = elem->u.l.first;
            break;
        default:
            assert(0);
            break;
    }
    if (!elem && iter->nest) {
        iter->cp = nullstr;
        iter->len = sizeof(nullstr);
        elem = iter->next[--(iter->nest)];
    }
    return elem;
}

/**
 * compare string value of elems: a and b
 *
 * - elem may be LISTELEM or SHORSTRELEM or FRAGELEM
 * 
 * - every traversal up or down through nested lists contibutes a single
 *   '\0' to the comparison.   This is to ensure that:  a  != (a)
 *
 * @param a
 * @param b
 * @return result of ASCII comparison: <0, 0, >0
 */
int compare (elem_t *a, elem_t *b)
{
    int rc;
    iter_t ai = { 0 };
    iter_t bi = { 0 };

    a = next(&ai, a);
    b = next(&bi, b);
    do {
        do { 
            ai.len--;
            bi.len--;
            rc = (*ai.cp++) - (*bi.cp++);
        } while (ai.len && bi.len && rc == 0);
        if (rc == 0) {
            rc = ai.len - bi.len;
        }
        if (a && ai.len == 0) {
            a = next(&ai, a);
        }
        if (b && bi.len == 0) {
            b = next(&bi, b);
        }
    } while (a && b && rc == 0);
    if (a || b) {
        return (int)(a - b);
    }
    return rc;
}

/**
 * match string value of elems: a and b
 * b may contain strings with trailing '*' which will
 * match any tail of the corresponding string in a
 *
 * - a and b may be LISTELEM or SHORSTRELEM or FRAGELEM
 * 
 * - every traversal up or down through nested lists contibutes a single
 *   '\0' to the comparison.   This is to ensure that:  a  != (a)
 *
 * @param a
 * @param b (may contain '*')
 * @return result of ASCII comparison: <0, 0, >0
 */
int match (elem_t *a, elem_t *b)
{
    int rc;
    iter_t ai = { 0 };
    iter_t bi = { 0 };

    a = next(&ai, a);
    b = next(&bi, b);
    do {
        do { 
            if (*bi.cp == '*') {
                rc = 0;
                break;
            } 
            ai.len--;
            bi.len--;
            rc = (*ai.cp++) - (*bi.cp++);
        } while (ai.len && bi.len && rc == 0);
        if (rc == 0 && *bi.cp == '*') {
            ai.len=0;
            bi.len=0;
        }
        if (rc == 0) {
            rc = ai.len - bi.len;
        }
        if (a && ai.len == 0) {
            a = next(&ai, a);
        }
        if (b && bi.len == 0) {
            b = next(&bi, b);
        }
    } while (a && b && rc == 0);
    if (a || b) {
        return (int)(a - b);
    }
    return rc;
}
