/* This file is part of GNU cflow
   Copyright (C) 1997, 2005, 2006, 2007 Sergey Poznyakoff

   GNU cflow is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GNU cflow is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with GNU cflow; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <cflow.h>
#include <parser.h>
#include <hash.h>

static Hash_table *symbol_table;

struct table_entry {
     Symbol *sym;
};

/* Calculate the hash of a string.  */
static size_t
hash_symbol_hasher(void const *data, unsigned n_buckets)
{
     struct table_entry const *t = data;
     if (!t->sym)
	  return ((size_t) data) % n_buckets;
     return hash_string(t->sym->name, n_buckets);
}

/* Compare two strings for equality.  */
static bool
hash_symbol_compare(void const *data1, void const *data2)
{
     struct table_entry const *t1 = data1;
     struct table_entry const *t2 = data2;
     return t1->sym && t2->sym && strcmp(t1->sym->name, t2->sym->name) == 0;
}

Symbol *
lookup(char *name)
{
     Symbol s;
     struct table_entry t, *tp;
     
     if (!symbol_table)
	  return NULL;
     s.name = name;
     t.sym = &s;
     tp = hash_lookup(symbol_table, &t);
     return tp ? tp->sym : NULL;
}

Symbol *
install(char *name)
{
     Symbol *sym;
     struct table_entry *tp, *ret;
     
     sym = xmalloc(sizeof(*sym));
     memset(sym, 0, sizeof(*sym));
     sym->type = SymUndefined;
     sym->name = name;

     tp = xmalloc(sizeof(*tp));
     tp->sym = sym;
     
     if (canonical_filename && strcmp(filename, canonical_filename))
	  sym->flag = symbol_temp;
     else
	  sym->flag = symbol_none;
     
     if (! ((symbol_table
	     || (symbol_table = hash_initialize (0, 0, 
						 hash_symbol_hasher,
						 hash_symbol_compare, 0)))
	    && (ret = hash_insert (symbol_table, tp))))
	  xalloc_die ();

     if (ret != tp) {
	  if (ret->sym->type != SymUndefined) 
	       sym->next = ret->sym;
	  ret->sym = sym;
     }
     return sym;
}

/* Unlink the first symbol from the table entry */
static Symbol *
unlink_symbol(struct table_entry *tp)
{
     Symbol *s = tp->sym;
     if (s) 
	  tp->sym = s->next;
     return s;
}     

/* Unlink and free the first symbol from the table entry */
static void
delete_symbol(struct table_entry *tp)
{
     Symbol *s = unlink_symbol(tp);
     /* The symbol could have been referenced even if it is static
	in -i^s mode. See tests/static.at for details. */
     if (s->ref_line == NULL) 
	  free(s);
}     

static void cleanup_symbol_refs(Symbol *sym);

/* Delete from the symbol table all static symbols defined in the current
   source.
   If the user requested static symbols in the listing, the symbol is
   not deleted, as it may have been referenced by other symbols. Instead,
   it is unlinked from its table entry and cleanup_symbol_refs() is
   called, so that its reference tables become usable.
   NOTE: This takes advantage of the fact that install() uses LIFO strategy,
   so we don't have to check the name of the source where the symbol was
   defined. */

static bool
static_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;

     if (t->sym
	 && t->sym->type == SymIdentifier
	 && t->sym->storage == StaticStorage) {
	  if (globals_only()) 
	       delete_symbol(t);
	  else
	       cleanup_symbol_refs(unlink_symbol(t));
     }
     return true;
}

static bool
temp_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     
     if (t->sym && t->sym->flag == symbol_temp) 
	  delete_symbol(t);
     return true;
}

void
delete_statics()
{
     hash_do_for_each (symbol_table, static_processor, NULL);
     hash_do_for_each (symbol_table, temp_processor, NULL);
}

/* See NOTE above */
bool
auto_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     Symbol *s = t->sym;
     if (s) {
	  int *level = proc_data;
	  if (s->type == SymIdentifier && s->level == *level) {
	       switch (s->storage) {
	       case AutoStorage:
		    delete_symbol(t);
		    break;
		    
	       case StaticStorage:
		    cleanup_symbol_refs(unlink_symbol(t));
		    break;
		    
	       default:
		    break;
	       }
	  }
     }
     return true;
}

/* Delete from the symbol table all auto variables with given nesting
   level. */
void
delete_autos(int level)
{
     hash_do_for_each (symbol_table, auto_processor, &level);
}


/* Make all list pointers of the SYM ready for final processing.
 * This means for each list replace its entry point with its CAR
 * and throw away the first cons. The first cons holds pointers
 * to the head and tail of the list and is used to speed up appends.
 *  
 * TODO: The memory is not reclaimed
 */
static void
cleanup_symbol_refs(Symbol *sym)
{
     if (sym && sym->type == SymIdentifier) {
	  if (sym->ref_line)
	       sym->ref_line = CAR(sym->ref_line);
	  if (sym->caller)
	       sym->caller = CAR(sym->caller);
	  if (sym->callee)
	       sym->callee = CAR(sym->callee);
     }
}

static bool
cleanup_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     Symbol *sym;
     for (sym = t->sym; sym; sym = sym->next) {
	  cleanup_symbol_refs(sym);
     }
     return true;
}


/* Clean up all symbols from the auxiliary information.
 * See the comment for cleanup_symbol() above
 */
void
cleanup()
{
     hash_do_for_each (symbol_table, cleanup_processor, NULL);
}

struct collect_data {
     Symbol **sym;
     int (*sel)(Symbol *p);
     size_t index;
};

static bool
collect_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     struct collect_data *cd = proc_data;
     Symbol *s;
     for (s = t->sym; s; s = s->next) {
	  if (cd->sel(s)) {
	       if (cd->sym)
		    cd->sym[cd->index] = s;
	       cd->index++;
	  }
     }
     return true;
}

int
collect_symbols(Symbol ***return_sym, int (*sel)(Symbol *p))
{
     struct collect_data cdata;

     cdata.sym = NULL;
     cdata.index = 0;
     cdata.sel = sel;
     hash_do_for_each (symbol_table, collect_processor, &cdata);
     cdata.sym = calloc(cdata.index, sizeof(*cdata.sym));
     if (!cdata.sym)
	  xalloc_die();
     cdata.index = 0;
     hash_do_for_each (symbol_table, collect_processor, &cdata);
     *return_sym = cdata.sym;
     return cdata.index;
}


/* Special handling for function parameters */

static bool
delete_parm_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     Symbol *s = t->sym;
     if (s) {
	  int *level = proc_data;
	  if (s->type == SymIdentifier && s->storage == AutoStorage
	      && s->flag == symbol_parm && s->level > *level)
	       delete_symbol(t);
     }
     return true;
}

/* Delete all parameters with parameter nesting level greater than LEVEL */
void
delete_parms(int level)
{
     hash_do_for_each (symbol_table, delete_parm_processor, &level);
}

static bool
move_parm_processor(void *data, void *proc_data)
{
     struct table_entry *t = data;
     Symbol *s = t->sym;
     if (s) {
	  int level = *(int*)proc_data;
	  if (s->type == SymIdentifier && s->storage == AutoStorage
	      && s->flag == symbol_parm) {
	       s->level = level;
	       s->flag = symbol_none;
	  }
     }
     return true;
}

/* Redeclare all saved parameters as automatic variables with the
   given nesting level */
void
move_parms(int level)
{
     hash_do_for_each (symbol_table, move_parm_processor, &level);
}


typedef struct bucket Bucket;
struct bucket {
     Bucket *next; /* Next bucket */
     int free;
     Cons cons[1];
};

static int bucket_nodes = 512;
static Bucket *root_bucket, *last_bucket;

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
	       error(2, 0, _("not enough core"));
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
append_to_list(Consptr *root_ptr, void *car)
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
symbol_in_list(Symbol *sym, Consptr list)
{
     Consptr cons;
     
     if (!list)
	  return 0;
     for (cons = CAR(list); cons; cons = CDR(cons))
	  if ((Symbol*)CAR(cons) == sym)
	       return 1;
     return 0;
}
