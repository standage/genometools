/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "libgtcore/fptr.h"

typedef struct Hashtable Hashtable;

typedef enum {
  HASH_DIRECT,
  HASH_STRING
} HashType;

typedef int (*Hashiteratorfunc)(void *key, void *value, void *data, Env*);

Hashtable* hashtable_new(HashType, FreeFunc keyfree, FreeFunc valuefree, Env*);
void*      hashtable_get(Hashtable*, const void*);
void       hashtable_add(Hashtable*, void*, void*, Env*);
void       hashtable_remove(Hashtable*, void*, Env*);
int        hashtable_foreach(Hashtable*, Hashiteratorfunc, void*, Env*);
/* iterate over the hashtable in alphabetical order. Requires that the hashtable
   has the HashType HASH_STRING. */
int        hashtable_foreach_ao(Hashtable*, Hashiteratorfunc, void*, Env*);
void       hashtable_reset(Hashtable*, Env*);
int        hashtable_unit_test(Env*);
void       hashtable_delete(Hashtable*, Env*);

#endif
