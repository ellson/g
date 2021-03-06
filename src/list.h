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

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LISTELEM = 0,           // must be 0 for static or calloc allocation of list headers
    FRAGELEM = 1,
    SHORTSTRELEM = 2,
    REFERSTRELEM = 3,
    TREEELEM = 4
} elemtype_t;

// elem struct, with unions to reduce size.
// Size is 32bytes on 64bit machines, and 20bytes on 32bit.
struct elem_s { 
    union {
        struct {
            elem_t *next;        // next elem in parent's list (next sibling)
            elem_t *first;       // first elem in this list (kid) (or NULL)
            elem_t *last;        // last elem in this list (last kid) (or NULL)
        } l;
        struct {
            elem_t *next;        // next frag in fraglist list
            inbufelem_t *inbuf;  // inbuf containing frag - for memory management
            unsigned char *frag; // pointer to beginning of frag
        } f;
        struct {
            unsigned char str[sizeof(void*)*3]; // short string (24char on 64bit, 12char on 32bit machines)
        } s;
        struct {
            unsigned char *str;  // reference to static string, any length
        } r;
        struct {
            elem_t *key;         // a list providing key and value
            elem_t *left;        // left elem of tree
            elem_t *right;       // left elem of tree
        } t;
    } u;
    // N.B. 1) Just type *must* be outside of union
    //    but moving the rest inside would increase the size of the struct.
    // N.B. 2) height and len could be a union, but that would
    //    not reduce the size of the struct on either 32 or 64 bit machines
    //    because of the rounding up of size to n* sizeof(void*).
    uint16_t height;        // TREEELEM: height of elem in tree
                            // LISTELEM: not used
                            // FRAGELEM: not used
                            // SHORTSTR: not used
                            // REFERSTR: not used
    uint16_t len;           // LISTELEM: number of elems from u.l.first to u.l.last
                            // FRAGELEM: number of characters in u.f.frag
                            // SHORTSTR: number of characters in u.s.str
                            // REFERSTR: number of characters in u.r.str
                            // TREEELEM: not used
    int16_t refs;           // don't free elem until refs == 0
    char state;             // state_machine state that generated this elem
    char type;              // as in elemtype_t
    // N.B. 3) Using elemtype_t (int) would increase the size of the struct
};

struct proc_list_s {        // LIST shared data context
    elem_t *free_elem_list; // linked list of unused list elems
    long stat_elemmax;      // list stats
    long stat_elemnow;
    long stat_elemmalloc;
    long stat_fragnow;
    long stat_fragmax;
};

struct list_s {             // LIST context
    INBUF_t INBUF;          // INBUF context, may be cast from LIST
    PROC_LIST_t *PROC_LIST;
};

#define LISTALLOCNUM 512

elem_t *new_list(LIST_t * LIST, char state);
elem_t *new_tree(LIST_t * LIST, elem_t *key);
elem_t *new_frag(LIST_t * LIST, char state, uint16_t len, unsigned char *frag);
elem_t *new_shortstr(LIST_t * LIST, char state, char *str);
elem_t *new_referstr(LIST_t * LIST, char state, char *str);
elem_t *ref_list(LIST_t * LIST, elem_t * list);
void append_addref(elem_t * list, elem_t * elem);
void append_transfer(elem_t * list, elem_t * elem);
void remove_next_from_list(LIST_t * LIST, elem_t * list, elem_t *elem);
void free_list(LIST_t * LIST, elem_t * elem);
void free_tree(LIST_t *LIST, elem_t * p);
void free_tree_item(LIST_t *LIST, elem_t * p);
void fraglist2shortstr(LIST_t * LIST, int slen, elem_t * string);

#ifdef __cplusplus
}
#endif

#endif
