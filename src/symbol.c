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

int hash_size = 509;  /* Size of hash table */
Symbol **symtab;      /* Symbol table */
Symbol *statsym;      /* Static symbols are held here */

typedef struct bucket Bucket;
struct bucket {
    Bucket *next; /* Next bucket */
    int free;
    Cons cons[1];
};

int bucket_nodes = 512;
Bucket *root_bucket, *last_bucket;
int symbol_count;

/* Set new hashtable size
 */
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

/* Init symbol table
 */
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

/* Find a Symbol corresponding to the given name
 */
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

/* Install a new symbol into the symbol table
 */
Symbol *
install(s)
    char *s;
{
    Symbol *sp;
    unsigned hc;

    sp = (Symbol *) emalloc(sizeof(Symbol));
    memset(sp, 0, sizeof(*sp));
    sp->name = emalloc(strlen(s) + 1);
    strcpy(sp->name, s);
    sp->type = SymUndefined;
    hc = hash(s);
    sp->next = symtab[hc];
    symtab[hc] = sp;
    symbol_count++;
    return sp;
}

/* Delete from the symbol table all static symbols defined in the current
 * source.
 * NOTE: This takes advantage of the fact that install() uses LIFO strategy,
 * so we have not to check the source name where the symbol was defined. If it
 * is static, we simply remove it.
 */
void
delete_statics()
{
    int i;
    Symbol *sp;
    
    for (i = 0; i < hash_size; i++) {
	if (symtab[i] &&
	    symtab[i]->type == SymFunction &&
	    symtab[i]->v.func.storage == StaticStorage) {
	    /* Delete the entry from the main symbol table */
	    sp = symtab[i];
	    symtab[i] = sp->next;
	    /* Add it to the static symbol list */
	    sp->next = statsym;
	    statsym = sp;
	}
    }
}

/* Delete from the symbol table all auto variables with given nesting
 * level.
 * TODO: The memory is not reclaimed.
 */
void
delete_autos(level)
    int level;
{
    int i;
    
    for (i = 0; i < hash_size; i++) {
	if (symtab[i] &&
	    symtab[i]->type == SymFunction &&
	    symtab[i]->v.func.level == level) {
	    symtab[i] = symtab[i]->next;
	}
    }
}

/* Make all list pointers of the SYM ready for final processing.
 * This means for each list replace its entry point with its CAR
 * and throw away the first cons. The first cons holds pointers
 * to the head and tail of the list and is used to speed up appends.
 *
 * TODO: The memory is not reclaimed
 */
static void
cleanup_symbol(sym)
    Symbol *sym;
{
    if (sym->type == SymFunction) {
	if (sym->v.func.ref_line)
	    sym->v.func.ref_line = CAR(sym->v.func.ref_line);
	if (sym->v.func.caller)
	    sym->v.func.caller = CAR(sym->v.func.caller);
	if (sym->v.func.callee)
	    sym->v.func.callee = CAR(sym->v.func.callee);
    }
}
    

/* Clean up all symbols from the auxiliary information.
 * See the comment for cleanup_symbol() above
 */
void
cleanup()
{
    int i;
    Symbol *sptr;

    for (i = 0; i < hash_size; i++) 
	for (sptr = symtab[i]; sptr; sptr = sptr->next) 
	    cleanup_symbol(sptr);

    for (sptr = statsym; sptr; sptr = sptr->next)
	cleanup_symbol(sptr);
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
    if (!last_bucket || last_bucket->free == bucket_nodes)
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
    CAR(cp) = CDR(cp) = NULL;
    return cp;
}

/* Append a new cons to the tail of the list
 * ROOT_PTR points to a `root cons'. 
 * CAR is the car value of the cons to be created.
 *
 * Note: Car of the root cons points to the head of the list,
 * cdr of root cons points to  the tail of the list.
 */
Consptr
append_to_list(root_ptr, car)
    Consptr *root_ptr;
    void *car;
{
    Consptr root, cons;

    if (!*root_ptr) {
	*root_ptr = alloc_cons();
	/* both car and cdr are guaranteed to be NULL */ 
    }
    root = *root_ptr;
    
    cons = alloc_cons();
    if (!CAR(root))
	CAR(root) = cons;

    /* Maintain linked list */
    if (CDR(root))
	CDR(CDR(root)) = cons;
    CDR(root) = cons;
    CAR(cons) = car;
    return cons;
}

int
collect_symbols(return_sym, sel)
    Symbol ***return_sym;
    int (*sel)();
{
    Symbol **sym, *st_ptr;
    int i, num=0;

    sym = calloc(symbol_count, sizeof(*sym));
    if (!sym)
	error(FATAL(2), "not enough core to sort symbols.");

    /* collect usable sybols */
    for (i = 0; i < hash_size; i++) {
	for (st_ptr = symtab[i]; st_ptr; st_ptr = st_ptr->next)
	    if (sel(st_ptr))
		sym[num++] = st_ptr;
    }
    if (!globals_only) {
	for (st_ptr = statsym; st_ptr; st_ptr = st_ptr->next)
	    if (sel(st_ptr))
		sym[num++] = st_ptr;
    }
    *return_sym = sym;
    return num;
}
