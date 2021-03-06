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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "fatal.h"
#include "inbuf.h"
#include "list.h"
#include "io.h"

#define IO() ((IO_t*)LIST)
#define INBUF() ((INBUF_t*)LIST)

/**
 * Private function to manage the allocation an elem_t
 *
 * elem_t are allocated in blocks and maintained in a free_elem_t list.
 * Freeing an elem_t actually means returning to this list.
 *
 * @param LIST the top-level context in which all lists are managed
 * @return a new initialized elem_t
 */
static elem_t *new_elem_sub(LIST_t * LIST)
{
    PROC_LIST_t * PROC_LIST = LIST->PROC_LIST;
    elem_t *elem, *nextelem;

    DUMMY_LOCK();  // FIXME

    if (!PROC_LIST->free_elem_list) {    // if no elems in free_elem_list

        PROC_LIST->free_elem_list = malloc(LISTALLOCNUM * sizeof(elem_t));
        if (!PROC_LIST->free_elem_list)
            FATAL("malloc()");
        PROC_LIST->stat_elemmalloc++;

        nextelem = PROC_LIST->free_elem_list;    // link the new elems into free_elem_list
        int i = LISTALLOCNUM;
        while (i--) {
            elem = nextelem++;
            elem->u.l.next = nextelem;
        }
        elem->u.l.next = NULL;    // terminate last elem

    }
    elem = PROC_LIST->free_elem_list;    // use first elem from free_elem_list
    PROC_LIST->free_elem_list = elem->u.l.next; // update list to point to next available

    PROC_LIST->stat_elemnow++;        // stats
    if (PROC_LIST->stat_elemnow > PROC_LIST->stat_elemmax) {
        PROC_LIST->stat_elemmax = PROC_LIST->stat_elemnow;
    }

    DUMMY_UNLOCK();  // FIXME

    // N.B. elem is uninitialized
    return elem;
}

/**
 * Private function to free an elem
 *
 *   "free" means insert elem at beginning of freelist
 *
 * @param LIST the top-level context in which all lists are managed
 * @param elem the elem being "freed"
 */
static void free_elem(LIST_t *LIST, elem_t *elem)
{
    PROC_LIST_t * PROC_LIST = LIST->PROC_LIST;

    DUMMY_LOCK();   // FIXME

    elem->u.l.next = PROC_LIST->free_elem_list;
    PROC_LIST->free_elem_list = elem;
    PROC_LIST->stat_elemnow--;    // maintain stats
    assert(PROC_LIST->stat_elemnow >= 0);

    DUMMY_UNLOCK(); // FIXME
}

/**
 * Return a pointer to an elem_t which is an empty list
 * The elem_t is memory managed without caller involvement.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param state a one character value stored with the elem, no internal meaning
 * @return a new initialized elem_t
 */
elem_t *new_list(LIST_t * LIST, char state)
{
    elem_t *elem = new_elem_sub(LIST);

    // complete elem initialization
    elem->type = LISTELEM;
    elem->state = state;    // state_machine state that created this frag
    elem->u.l.next = NULL;  // clear next
    elem->u.l.first = NULL; // new list is empty
    elem->u.l.last = NULL;
    elem->len = 0;
    elem->refs = 1;
    elem->height = 0;       // notused

    return elem;
}

/**
 * Return a pointer to an elem_t which refers to a string fragment in an inbuf
 * (start address and length).
 * The elem_t, and inbuf_t, are memory managed without caller involvement.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param state a one character value stored with the elem, no internal meaning
 * @param len fragment length
 * @param frag pointer to first character of contiguous fragment of len chars
 * @return a new initialized elem_t
 */
elem_t *new_frag(LIST_t * LIST, char state, uint16_t len, unsigned char *frag)
{
    assert(INBUF()->inbuf);
    assert(INBUF()->inbuf->u.refs >= 0);
    assert(frag);
    assert(len > 0);

    elem_t *elem = new_elem_sub(LIST);

    // complete frag elem initialization
    elem->type = FRAGELEM;  // type
    elem->state = state;    // state_machine state that created this frag
    elem->u.f.next = NULL;  // clear next
    elem->u.f.inbuf = INBUF()->inbuf; // record inbuf for ref counting
    elem->u.f.frag = frag;  // pointer to start of frag in inbuf
    elem->len = len;        // length of frag
    elem->refs = 1;         // initial ref count
    elem->height = 0;       // notused

    INBUF()->inbuf->u.refs++;   // increment reference count in inbuf.

    PROC_LIST_t * PROC_LIST = LIST->PROC_LIST;
    PROC_LIST->stat_fragnow++;        // stats
    if (PROC_LIST->stat_fragnow > PROC_LIST->stat_fragmax) {
        PROC_LIST->stat_fragmax = PROC_LIST->stat_fragnow;
    }
    return elem;
}

/**
 * Return a pointer to an elem_t which holds a shortstr
 * The string is stored in the struct.  Maximum length sizeof(void*)*3
 * The stored string is *not* NUL terminated;
 *
 * @param LIST the top-level context in which all lists are managed
 * @param state a one character value stored with the elem, no internal meaning
 * @param str string to be stored in elem (nul terminated) max 12 chars on 32 bit machines
 * @return a new initialized elem_t
 */
elem_t * new_shortstr(LIST_t * LIST, char state, char * str)
{
    elem_t *elem = new_elem_sub(LIST);
    int len = 0;

    // complete shortstr elem initialization
    elem->type = SHORTSTRELEM; // type
    elem->state = state;       // ABC or DQT for strings
    if (str) {  // copy string into internal buffer
        char c;
        while (len < sizeof(((elem_t*)0)->u.s.str) && (c = str[len]) != '\0') {
            elem->u.s.str[len++] = c;
        }
    }
    elem->len = len;           // length of the stored string
    elem->refs = 1;            // initial ref count
    elem->height = 0;          // notused
    return elem;
}

/**
 * Return a pointer to an elem_t which references a static string
 *    The referenced static string is not freed when the elem is freed.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param state a one character value stored with the elem, no internal meaning
 * @param str a static string to be referenced by elem.
 * @return a new initialized elem_t
 */
elem_t * new_referstr(LIST_t * LIST, char state, char * str)
{
    elem_t *elem = new_elem_sub(LIST);

    // complete shortstr elem initialization
    elem->type = REFERSTRELEM; // type
    elem->state = state;       // ABC or DQT for strings
    elem->u.r.str = (unsigned char*)str;  // reference to static string
    elem->len = strlen(str);   // length of the stored string
    elem->refs = 1;            // initial ref count
    elem->height = 0;          // notused
    return elem;
}

/**
 * Return a pointer to an elem_t which is a tree node with a key (reference to a list)
 * The elem_t is memory managed without caller involvement.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param key a list containing (as some point) some frags wihich are the key
 * @return a new initialized elem_t
 */
elem_t *new_tree(LIST_t * LIST, elem_t *key)
{
    assert(key);

    key->refs++;
    assert(key->refs > 0);

    elem_t *elem = new_elem_sub(LIST);

    // complete elem initialization
    elem->type = TREEELEM;
    elem->u.t.key = key;
    elem->u.t.left = NULL; // new tree is empty so far
    elem->u.t.right = NULL;
    elem->height = 1;
    elem->refs = 1;        // init, but don't use since tree root changes during balancing
    elem->state = key->state;
    elem->len = 0;         // notused

    return elem;
}

/**
 * Free the contents of a list, but not the list itself
 *
 * @param LIST the top-level context in which all lists are managed
 * @param list elem whose contents are to be freed
 */
static void free_list_r(LIST_t * LIST, elem_t * list)
{
    assert(list);
    assert(list->type == (char)LISTELEM);
    assert(list->refs >= 0);

    // free list of elem, but really just put them back
    // on the elem_freelist (declared at the top of this file)`
    free_list(LIST, list->u.l.first);

    // clean up emptied list
    list->u.l.first = NULL;
    list->u.l.last = NULL;
    list->len = 0;
    // Note: ref count of the empty list is not modified.
    // It may be still referenced, even though it is now empty.
}

/**
 * Free list - for each elem in an elem, elem->u.l.next chain:
 * decrement its ref count, and if zero, free its contents,
 * free the rest of the chain, and free the elem itself.
 *
 * If it is a list of lists, then the refence count in the first elem_t is
 * decremented and the elements are freed only if the references are  zero.
 *
 * If it is a list of fragments, then the reference count to the fragments'
 * inbufs are decremented and the inbuf freed if there are no more fragments
 * in use.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param elem - a list elem
 */
void free_list(LIST_t * LIST, elem_t * elem)
{
    elem_t *nextelem;

    while (elem) {
        assert(LIST->PROC_LIST->stat_elemnow > 0);
        assert(elem->refs > 0);
        nextelem = elem->u.l.next;
        switch ((elemtype_t)elem->type) {
        case LISTELEM:
            if (--(elem->refs)) {
                return;    // stop at any point with additional refs
            }
            free_list_r(LIST, elem); // recursively free content lists that have no references
            break;
        case FRAGELEM:
            if (--(elem->refs)) {
                return;    // stop at any point with additional refs
            }
            assert(elem->u.f.inbuf->u.refs > 0);
            if (--(elem->u.f.inbuf->u.refs) == 0) {
                free_inbuf(INBUF(), elem->u.f.inbuf);
            }
            LIST->PROC_LIST->stat_fragnow--;    // maintain stats
            break;
        case SHORTSTRELEM:
        case REFERSTRELEM:
            // These are self contained singletons,  nothing else to clean up
            // (The str in shortstring is in the elem, and so freed with the elem)
            // (The str in referstring is external and assumed static, it is not freed.)
            if (--(elem->refs)) {
                return;    // stop at any point with additional refs
            }
            nextelem = NULL;
            break;
        case TREEELEM:
            // trees are not ref count, must be used exactly once.
            // (They are not ref counted because the root elem of the tree can
            //  change with balancing,  and so writing refs in root doesn't mean it will be
            //  available in another.)
            free_tree(LIST, elem);
            return;
        }
        free_elem(LIST, elem);

        elem = nextelem;
    }
}

/**
 * After accounting for u.t.left and u.t.right, free a single
 * tree element and the u.t.key list that it holds.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param p the tree elem to be free
 */
void free_tree_item(LIST_t *LIST, elem_t * p)
{
    elem_t *k = p->u.t.key;

    assert(k);
    switch ((elemtype_t)k->type) {
        case LISTELEM:
           free_list(LIST, k);
           break;
        case SHORTSTRELEM:
           k->refs--;
           free_elem(LIST, k);
           break;
        default:
           assert(0);
           break;
    }
    free_elem(LIST, p);
}

/**
 * Free a tree elems left branh, recursively, then its right branch, recursively, then
 * the elem's key, and the elem itself.
 *
 * Trees are not ref counted, so they must belong in a single list.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param p the tree elem to be freed
 */
void free_tree(LIST_t *LIST, elem_t * p)
{
    if ( !p ) {
        return;
    }
    assert((elemtype_t)p->type == TREEELEM);
    free_tree(LIST, p->u.t.left);
    free_tree(LIST, p->u.t.right);
    free_tree_item(LIST, p);
}

/**
 * Create a new list with the same content, by reference, as the input list.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param list a header to a list to be referenced
 * @return a new initialized elem_t which is now also a header of the referenced list
 */
elem_t *ref_list(LIST_t * LIST, elem_t * list)
{
    elem_t *elem;

    assert(list->type == (char)LISTELEM);

    elem = new_elem_sub(LIST);

    elem->type = LISTELEM;       // type
    elem->refs = 1;
    elem->state = list->state;
    elem->u.l.next = NULL;       // clear next
    elem->u.l.first = list->u.l.first; // copy details
    elem->u.l.last = list->u.l.last;
    elem->len = list->len;
    if (list->u.l.first) {
        // can't add refs to trees because the root element
        // changes on rebalancing
        assert(list->u.l.first->type != TREEELEM);
        list->u.l.first->refs++;    // increment ref count
    }
    return elem;
}

/**
 *  Append a list elem_t to the end of the list of lists, transferring ownership
 *
 *  The reference count in the appended element not changed.
 *
 * @param list the header of the list to be appended
 * @param elem the element to be appended (must be a LISTELEM)
 */
void append_transfer(elem_t * list, elem_t * elem)
{
    assert(list->type == (char)LISTELEM);
    assert(elem->refs > 0);
    if (list->u.l.first) {
        assert((elemtype_t)list->u.l.first->type != TREEELEM);  // TREE elems have no next pointer
        list->u.l.last->u.l.next = elem;
    } else {
        list->u.l.first = elem;
    }
    list->len++;
    list->u.l.last = elem;
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
void append_addref(elem_t * list, elem_t * elem)
{
    append_transfer(list, elem);
    elem->refs++;
    assert(elem->refs > 0);
}

/**
 *  Remove the next element from a list.
 *
 *  The removed element is freed.
 *
 * @param LIST the top-level context in which all lists are managed
 * @param list header of the list to be shortened
 * @param elem the elem preceding the elem to be removed (or NULL to remove 1st elem)
 */
void remove_next_from_list(LIST_t * LIST, elem_t * list, elem_t *elem)
{
    elem_t *old;

    assert(list);
    assert(list->type == (char)LISTELEM);
    assert(list->u.l.last);    // must be at least one elem in the list
    assert(list->len > 0);     // must be at least one elem in the list

    if (! elem) {                            // if removing the first elem
        old = list->u.l.first;
        list->u.l.first = list->u.l.first->u.l.next; // skip the elem being removed
    }
    else {
        old = elem->u.l.next;
        elem->u.l.next = elem->u.l.next->u.l.next;   // skip the elem being removed
    }
    if (list->u.l.last == old) {             // if removing the last element
        list->u.l.last = elem;               // then elem is the new last (or NULL)
    }
    list->len--;                             // list has one less elem

    assert(old->refs > 0);
    free_list(LIST, old);                    // free the removed elem
}

/**
 * Convert a string LISTELEM to a SHORTSTR
 *  - assumes the caller has prechecked slen and any other inhibitors
 *
 * @param LIST the top-level context in which all lists are managed
 * @param slen the length of the string
 * @param string
 */
void fraglist2shortstr(LIST_t * LIST, int slen, elem_t * string)
{
    PROC_LIST_t * PROC_LIST = LIST->PROC_LIST;
    elem_t *frag, *nextfrag;
    unsigned char *src, *dst;
    int i;

    assert(string);
    assert(string->type == (char)LISTELEM);
    assert(slen <= sizeof(((elem_t*)0)->u.s.str));

    frag = string->u.l.first;
    dst = string->u.s.str;
    while (frag) {
        assert(frag->type == (char)FRAGELEM);
        assert(frag->refs == 1);
        nextfrag = frag->u.f.next;
        (frag->u.f.inbuf->u.refs)--;
        for (i = frag->len, src = frag->u.f.frag; i; --i) {
            *dst++ = *src++;
        }
        free_elem(LIST, frag);
        PROC_LIST->stat_fragnow--;    // maintain stats
        assert(PROC_LIST->stat_fragnow >= 0);
        frag = nextfrag;
    }
    string->type = SHORTSTRELEM; // frag is now shortstr
    string->len = slen; // save length of string
}
