/* vim:set shiftwidth=4 ts=8 expandtab: */

#include "libje_private.h"

/**
 * Private function to manage the allocation an elem_t
 *
 * elem_t are allocated in blocks and maintained in a free_elem_t list.
 * Freeing an elem_t actually means returning to this list.
 *
 * @param C the top-level context in which all lists are managed
 * @param type one of: LISTELEM, FRAGELEM, HASHELEM
 * @return a new intialized elem_t
 */
static elem_t *new_elem_sub(context_t * C, elemtype_t type)
{
    elem_t *elem, *next;
    int i;

    if (!C->free_elem_list) {    // if no elems in free_elem_list

        C->free_elem_list = malloc(LISTALLOCNUM * size_elem_t);
        if (!C->free_elem_list) {
            perror("Error - malloc(): ");
            exit(EXIT_FAILURE);
        }
        C->stat_elemmalloc++;

        next = C->free_elem_list;    // link the new elems into free_elem_list
        i = LISTALLOCNUM;
        while (i--) {
            elem = next++;
            elem->next = next;
        }
        elem->next = NULL;    // terminate last elem

    }
    elem = C->free_elem_list;    // use first elem from free_elem_list
    C->free_elem_list = elem->next; // update list to point to next available

    elem->type = (char)type;    // init the new elem
    elem->next = NULL;

    C->stat_elemnow++;        // stats
    if (C->stat_elemnow > C->stat_elemmax) {
        C->stat_elemmax = C->stat_elemnow;
    }
    return elem;
}

/**
 * Return a pointer to an elem_t which holds a string fragment
 * (start address and length).
 * The elem_t is memory managed without caller involvement.
 *
 * @param C the top-level context in which all lists are managed
 * @param state a one character value stored with the elem, no internal meaning
 * @param len fragment length
 * @param frag pointer to first character of contiguous fragment on len chars
 * @return a new intialized elem_t
 */
elem_t *new_frag(context_t * C, char state, unsigned int len, unsigned char *frag)
{
    elem_t *elem;

    elem = new_elem_sub(C, FRAGELEM);

    assert(C->inbuf);
    assert(C->inbuf->refs >= 0);
    assert(frag);
    assert(len > 0);
    // complete frag elem initialization
    elem->u.frag.inbuf = C->inbuf;    // record inbuf for ref counting
    elem->u.frag.frag = frag;    // pointer to begging of frag
    elem->v.frag.len = len;    // length of frag
    elem->state = state;    // state_machine state that created this frag

    C->inbuf->refs++;        // increment reference count in inbuf.
    return elem;
}

/**
 * Return a pointer to an elem_t which holds a hashname
 * (suitable for use as a filename)
 * The element is memory managed without caller involvement.
 * The FILE* in the elem_t is initialized to NULL
 *
 * @param C the top-level context in which all lists are managed
 * @param hash a long containing a hash value
 * @return a new intialized elem_t
 */
elem_t *new_hash(context_t * C, unsigned long hash)
{
    elem_t *elem;

    elem = new_elem_sub(C, HASHELEM);

    // complete frag elem initialization
    elem->u.hash.hash = hash;    // the hash value
    elem->u.hash.out = NULL;    // open later
    return elem;
}

/**
 * Private function to clone a list header to a new elem.
 *
 * The old list header is not modified, so could have been be statically or
 * dynamically created.
 *
 * The ref count in the first elem is not updated for this clone, so
 * this function must only be used by move_list() or ref_list(), which
 * make appropriate fixes to the ref counts.
 *
 * @param C the top-level context in which all lists are managed
 * @param list a header to a list to be cloned
 * @return a new intialized elem_t
 */
static elem_t *clone_list(context_t * C, elem_t * list)
{
    elem_t *elem;

    assert(list->type == (char)LISTELEM);

    elem = new_elem_sub(C, LISTELEM);

    elem->u.list.first = list->u.list.first;    // copy details
    elem->u.list.last = list->u.list.last;
    elem->v.list.refs = 0;
    elem->state = list->state;
    return elem;
}

/**
 * Move a list to a new elem.
 * Typically used to move a list from a call stack header into an elem_t header
 * so the list can be in a lists of lists.
 *
 * Implemented using clone_list. Clone_list didn't increase the ref count
 * in the first elem_t, so no need to deref.
 *
 * Clean up the old list header so it no longer references the list elems.
 *
 * @param C the top-level context in which all lists are managed
 * @param list a header to a list to be moved
 * @return a new intialized elem_t which is now the header of the moved list
 */
elem_t *move_list(context_t * C, elem_t * list)
{
    elem_t *elem;

    elem = clone_list(C, list);

    list->u.list.first = NULL;    // reset old header
    list->u.list.last = NULL;
    list->state = 0;

    return elem;
}

/**
 * Reference a list from a new elem_t.
 * Implement as a clone_list with a ref count adjustment
 *
 * If there is a first elem and if it is a LISTELEM, then
 * increment the first elem's ref count.  (NB, not the ref_count in this
 * new elem_t)
 *
 * @param C the top-level context in which all lists are managed
 * @param list a header to a list to be referenced
 * @return a new intialized elem_t which is now also a header of the referenced list
 */
elem_t *ref_list(context_t * C, elem_t * list)
{
    elem_t *elem;

    elem = clone_list(C, list);

    if (list->u.list.first && list->u.list.first->type == LISTELEM) {
        list->u.list.first->v.list.refs++;    // increment ref count
    }
    return elem;
}

/**
 *  Append a list elem_t to the end of the list of lists.
 *
 *  The reference count in the appended element is incremented
 *  to account for the new reference from the old tail elem_t
 *
 * @param list the header of the list to be appended
 * @param elem the element to be appended (must be a LISTELEM)
 */
void append_list(elem_t * list, elem_t * elem)
{
    assert(list->type == (char)LISTELEM);

    if (list->u.list.first) {
        list->u.list.last->next = elem;
    } else {
        list->u.list.first = elem;
    }
    list->u.list.last = elem;
    if (elem->type == (char)LISTELEM) {
        elem->v.list.refs++;    // increment ref count in appended elem
        assert(elem->v.list.refs > 0);
    }
}

/**
 * Free the list contents, but not the list header.
 * This function can be used on statically or callstack allocated list headers.
 *  
 * If it is a list of lists, then the refence count in the first elem_t is
 * decremented and the elements are freed only if the references are  zero.
 *
 * If it is a list of fragments, then the reference count to the fragments'
 * inbufs are decremented and the inbuf freed if there are no more fragments
 * in use.
 *
 * Lists of hashes are not allowed to be freed.
 *
 * @param C the top-level context in which all lists are managed
 * @param list a header to the list to be freed.
 */
void free_list(context_t * C, elem_t * list)
{
    elem_t *elem, *next;

    assert(list);
    assert(list->type == (char)LISTELEM);

    // free list of elem, but really just put them back
    // on the elem_freelist (declared at the top of this file)`
    elem = list->u.list.first;
    while (elem) {
        next = elem->next;
        switch ((elemtype_t) (elem->type)) {
        case FRAGELEM:
            assert(elem->u.frag.inbuf->refs > 0);
            if (--(elem->u.frag.inbuf->refs) == 0) {
                free_inbuf(C, elem->u.frag.inbuf);
            }
            break;
        case LISTELEM:
            assert(elem->v.list.refs > 0);
            if (--(elem->v.list.refs) > 0) {
                goto done;    // stop at any point with additional refs
            }
            free_list(C, elem); // recursively free lists that have no references
            break;
        case HASHELEM:
            assert(0);  // should not be here
            break;
        }

        // insert elem at beginning of freelist
        elem->next = C->free_elem_list;
        C->free_elem_list = elem;

        C->stat_elemnow--;    // maintain stats

        elem = next;
    }

 done:
    // clean up emptied list
    list->u.list.first = NULL;
    list->u.list.last = NULL;
    // Note: ref count of the header is not modified.
    // It may be still referenced, even though it is now empty.
}

/**
 * Print a single fragment of len contiguous characters.
 *
 * @param chan output FILE*
 * @param len number of contiguous characters to be output
 * @param frag pointer to the first character in the fragment
 */
static void print_one_frag(FILE * chan, unsigned int len, unsigned char *frag)
{
    while (len--) {
        putc(*frag++, chan);
    }
}

/**
 * Print a len_frag (an 8bit length, followed by that number of characters)
 *
 * @param chan output FILE*
 * @param len_frag an 8bit length, followed by that number of characters
 */
int print_len_frag(FILE * chan, unsigned char *len_frag)
{
    unsigned char len;

    len = *len_frag++;
    print_one_frag(chan, len, len_frag);
    return len;
}

/**
 * Conditionaly print a separator followed by the concatenation of
 * fragments in the list.
 * The composite string is quoted as necessary to comply with g syntax
 * (although not necessarily in the same way as in the original input).
 *
 * @param chan output FILE*
 * @param liststate an indicator if the string is to be quoted
 * @param elem the first frag of the fragllist
 * @param sep if not NULL then a character to be printed first
 */
void print_frags(FILE * chan, state_t liststate, elem_t * elem, char *sep)
{
    unsigned char *frag;
    int len;

    if (*sep) {
        putc(*sep, chan);
    }
    if (liststate == DQT) {
        putc('"', chan);
    }
    while (elem) {
        frag = elem->u.frag.frag;
        len = elem->v.frag.len;
        assert(len > 0);
        if ((state_t) elem->state == BSL) {
            putc('\\', chan);
        }
        if ((state_t) elem->state == AST) {
            if (liststate == DQT) {
                putc('"', chan);
                putc('*', chan);
                putc('"', chan);
            } else {
                putc('*', chan);
            }
        }
        else {
            print_one_frag(chan, len, frag);
        }
        elem = elem->next;
    }
    if (liststate == DQT) {
        putc('"', chan);
    }
    *sep = ' ';
}

/**
 * Print a simple fragment list (a string)
 * or print a list of strings (recursively), with appropriate separators
 * and indentation
 *
 * If non-negative initial indent, each nested list is printed at an incremented indent
 *
 * @param chan output FILE*
 * @param list the header of a fraglist, or a list (maybe nested)  of fraglists
 * @param indent if not -ve, then the initial indent
 * @param sep if not NULL then a character to be printed first
 */
void print_list(FILE * chan, elem_t * list, int indent, char *sep)
{
    elem_t *elem;
    elemtype_t type;
    int ind, cnt, width;

    assert(list->type == (char)LISTELEM);
    elem = list->u.list.first;
    if (!elem)
        return;
    type = (elemtype_t) (elem->type);
    switch (type) {
    case FRAGELEM:
        print_frags(chan, list->state, elem, sep);
        break;
    case LISTELEM:
        cnt = 0;
        width = 0;
        while (elem) {
            assert(elem->type == (char)type);    // check all the same type
            if (cnt++) {
                putc('\n', chan);
                putc(' ', chan);
                if (indent >= 0) {
                    ind = indent;
                    while (ind--)
                        putc(' ', chan);
                }
            } else {
                putc(' ', chan);
            }
            width = print_len_frag(chan, NAMEP(elem->state));
            ind = indent + width + 1;;
            print_list(chan, elem, ind, sep);    // recurse
            elem = elem->next;
        }
        break;
    case HASHELEM:
        assert(0);  // should not be here
        break;
    }
}
