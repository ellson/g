/* vim:set shiftwidth=4 ts=8 expandtab: */

#ifndef MERGE_H
#define MERGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "frag.h"

elem_t * merge(elem_t * new, elem_t * old);

#ifdef __cplusplus
}
#endif

#endif