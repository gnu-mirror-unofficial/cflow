/*  $Id$ */
/*  cflow:
 *  Copyright (C) 1997 Gray
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cflow.h"

int hash_size = 509;
Symbol **symtab;

typedef struct bucket Bucket;
struct bucket {
    Bucket *next; /* Next bucket */
    int free;
    Cons cons[1];
};

int bucket_nodes = 512;
Bucket *root_bucket, *last_bucket;

void
set_hash_size(num)
    int num;
{
    if (num <= 0) {
	error(0, "invalid hash size value: ignored");
	return;
    }
    hash_size = num;
}

void
init_hash()
{
    unsigned i;
    
    symtab = emalloc(hash_size * sizeof(symtab[0]));
    for (i = 0; i < hash_size; i++)
	symtab[i] = NULL;
}

/* Compute hash-code for the given name */
unsigned
hash(name)
    char *name;
{
    unsigned i;

    for (i = 0; *name; name++) {
        i <<= 1;
        i ^= *(unsigned char*)name;
    }
    return i % hash_size;
}

Symbol *
lookup(s)
    char *s;
{
    Symbol *sp;

    for (sp = symtab[hash(s)]; sp != (Symbol *) 0; sp = sp->next)
        if (strcmp(sp->name, s) == 0)
            return sp;
    return 0;
}

Symbol *
install(s)
    char *s;
{
    Symbol *sp;
    unsigned hc;

    sp = (Symbol *) emalloc(sizeof(Symbol));
    sp->name = emalloc(strlen(s) + 1);
    strcpy(sp->name, s);
    sp->type = SymUndefined;
    hc = hash(s);
    sp->next = symtab[hc];
    symtab[hc] = sp;
    return sp;
}


void
alloc_new_bucket()
{
    Bucket *bp;

    bp = malloc(sizeof(*bp) + sizeof(Cons)*(bucket_nodes-1));
    if (!bp)
	return;
    bp->next = NULL;
    bp->free = 0;
    if (!root_bucket) 
	root_bucket = last_bucket = bp;
    else {
	last_bucket->next = bp;
	last_bucket = bp;
    }
}

Consptr
alloc_cons_from_bucket()
{
    if (last_bucket->free == bucket_nodes)
	return NULL;
    return &last_bucket->cons[last_bucket->free++];
}

Consptr
alloc_cons()
{
    Consptr cp;

    cp = alloc_cons_from_bucket();
    if (!cp) {
	alloc_new_bucket();
	if ((cp = alloc_cons_from_bucket()) == NULL) {
	    error(FATAL(2), "not enough core");
	}
    }
    return cp;
}
