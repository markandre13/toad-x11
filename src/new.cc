/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/toadbase.hh>
#include <toad/debug.hh>
#include <toad/stacktrace.hh>

using namespace toad;

#ifdef DEBUG_MEMORY

/**
 * \file new.cc
 *
 * This stuff is a merger of glibc 2.1 `malloc/malloc.c' and gcc 2.95
 * `gcc/cp/new1.cc', `gcc/cp/new2.cc' to produce meaningful output for
 * the `mtrace' tool with c++.
 *
 * Since `new' and `delete' are declare `weak', just to have these
 * methods in the toad library will override 'em.
 *
 * To use it define the MALLOC_CHECK_ environment variable with
 * \li 0: any detected heap corruption is silently ignored
 * \li 1: a diagnostic is printed on `stderr'
 * \li 2: `abort' is called immediately
 */

#define MALLOC_HOOKS

#include <new>
#include <stdio.h>
#include <malloc.h>
#include <mcheck.h>
#include <pthread.h>
#include <set>

using namespace std;

static new_handler std_new_handler;

static pthread_mutex_t mutex;


/*
 * With some compilers we fail to use a 'set' during new so these functions
 * implement a red-black tree in plain C.
 */

/* implementation dependent declarations */
typedef enum {
    STATUS_OK,
    STATUS_MEM_EXHAUSTED,
    STATUS_DUPLICATE_KEY,
    STATUS_KEY_NOT_FOUND
} statusEnum;

typedef void* keyType;            /* type of key */

/* user data stored in tree */
typedef struct {
  TStackTrace *a;
#ifdef LIKE_ELECTRIC_FENCE
  TStackTrace *f;
#endif
} recType;

#define compLT(a,b) (a < b)
#define compEQ(a,b) (a == b)

/* implementation independent declarations */
/* Red-Black tree description */
typedef enum { BLACK, RED } nodeColor;

typedef struct nodeTag {
    struct nodeTag *left;       /* left child */
    struct nodeTag *right;      /* right child */
    struct nodeTag *parent;     /* parent */
    nodeColor color;            /* node color (BLACK, RED) */
    keyType key;                /* key used for searching */
    recType rec;                /* user data */
} nodeType;

statusEnum insert(keyType key, recType *rec);
statusEnum rb_delete(keyType key);
statusEnum find(keyType key, recType **rec);
static void clearTree(nodeType *n);
#define NIL &sentinel           /* all leafs are sentinels */
static nodeType sentinel = { NIL, NIL, 0, BLACK, 0};

static recType rec;
static nodeType *root = NIL;    /* root of Red-Black tree */

static bool debug_mem = false;

void 
toad_memory_check()
{
  /* set your breakpoint here */
}

//----------------------

/**
 * TOAD's malloc
 *
 * \li
 *   Calls GLIBC's internal __malloc_hook, if available, to register the
 *   return address of the 'new' statement instead of the address of the
 *   'malloc' statement.
 * \li
 *   Stores the allocated address in a red-black tree together with the
 *   adress of the new statement when memory debugging is enabled.
 */
void* 
toad_malloc(size_t sz)
{
  pthread_mutex_lock(&mutex);
  void *p;
  if (sz==0)
    sz=1;
  if (__malloc_hook)
    p = (void *) (*__malloc_hook)(sz, __builtin_return_address(1));
  else
    p = malloc(sz);
  while(p==0) {
    new_handler handler = std_new_handler;
    if (!handler)
       throw bad_alloc();
    handler();
    if (__malloc_hook)
      p = (void *) (*__malloc_hook)(sz, __builtin_return_address(1));
    else
      p = malloc(sz);
  }
  if (debug_mem) {
    rec.a = (TStackTrace*)malloc(sizeof(TStackTrace));
    rec.a = new(rec.a)TStackTrace();
#ifdef LIKE_ELECTRIC_FENCE
    rec.f = NULL;
#endif
    insert(p, &rec);
  }
//cerr << "allocated " << p << endl;
  pthread_mutex_unlock(&mutex);
  return p;
}

void toad_free(void *ptr)
{
//cout << "free " << ptr << endl;
  pthread_mutex_lock(&mutex);
  if (ptr==0) {
    cerr << "toad_memory_check: free/delete NULL" << endl;
    printStackTrace();
    toad_memory_check();
    pthread_mutex_unlock(&mutex);
    return;
  }

  if (debug_mem) {
    recType *rec;
    if (find(ptr, &rec)!=STATUS_OK) {
      cerr << "toad_memory_check: free/delete illegal address " << (void*)ptr << endl;
      printStackTrace();
      toad_memory_check();
      pthread_mutex_unlock(&mutex);
      abort();
    }
#ifndef LIKE_ELECTRIC_FENCE
    if (rec->a)
      free(rec->a);
    rb_delete(ptr);
#else
    if (rec->f) {
      cerr<< "toad_memory_check: free/delete illegal address " << endl
          << "was allocated at:" << endl;
      rec->a->print();
      cerr<< "was freed at:" << endl;
      rec->f->print();
      cerr<<"we're now at:" << endl;
      printStackTrace();
      toad_memory_check();
      pthread_mutex_unlock(&mutex);
      abort();
    } else {
      rec->f = (TStackTrace*)malloc(sizeof(TStackTrace));
      rec->f = new(rec->f)TStackTrace();
    }
#endif
  }
#ifndef LIKE_ELECTRIC_FENCE
  if (__free_hook)
    (*__free_hook)(ptr, __builtin_return_address(1));
  else
    free(ptr);
#endif
  pthread_mutex_unlock(&mutex);
}

void* operator new(size_t sz) throw (std::bad_alloc)
{
  return toad_malloc(sz);
}

void* operator new[] (size_t sz) throw (std::bad_alloc)
{
  return toad_malloc(sz);
}

void operator delete (void *ptr) throw ()
{
  return toad_free(ptr);
}

void operator delete[] (void *ptr) throw ()
{
  return toad_free(ptr);
}


void
toad::debug_mem_start()
{
#if 0
  if (mcheck(NULL)!=0) {
    printf("toad: enabling memory debugging\n");
    printf("  warning: mcheck called to late\n");
  } else {
    printf("toad: enabling memory debugging\n");
  }
#else
  printf("toad: enabling memory debugging\n");
#endif
  pthread_mutex_init(&mutex, NULL);
  if (!__malloc_hook || !__free_hook) {
    cerr<< "  warning: memory hooks are NULL, wrong adresses for mcheck\n"
        << "           please try to set MALLOC_CHECK_=2 in your environment\n";
  }
  mtrace();
  std_new_handler = set_new_handler(NULL);
  set_new_handler(std_new_handler);
  debug_mem = true;
}

void toad::debug_mem_end()
{
  if (debug_mem) {
    clearTree(root);
    root=NIL;
    debug_mem = false;
  }
}

//---------------------------------------------------------------------------

/*
 * red-black tree implementation from
 * Thomas Niemann <thomasn@epaperpress.com>       
 */


/* last node found, optimizes find/delete operations */
static nodeType *lastFind;


static void rotateLeft(nodeType *x) {

   /**************************
    *  rotate node x to left *
    **************************/

    nodeType *y = x->right;

    /* establish x->right link */
    x->right = y->left;
    if (y->left != NIL) y->left->parent = x;

    /* establish y->parent link */
    if (y != NIL) y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    } else {
        root = y;
    }

    /* link x and y */
    y->left = x;
    if (x != NIL) x->parent = y;
}

static void rotateRight(nodeType *x) {

   /****************************
    *  rotate node x to right  *
    ****************************/

    nodeType *y = x->left;

    /* establish x->left link */
    x->left = y->right;
    if (y->right != NIL) y->right->parent = x;

    /* establish y->parent link */
    if (y != NIL) y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    } else {
        root = y;
    }

    /* link x and y */
    y->right = x;
    if (x != NIL) x->parent = y;
}

static void insertFixup(nodeType *x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after inserting node x           *
    *************************************/

    /* check Red-Black properties */
    while (x != root && x->parent->color == RED) {
        /* we have a violation */
        if (x->parent == x->parent->parent->left) {
            nodeType *y = x->parent->parent->right;
            if (y->color == RED) {

                /* uncle is RED */
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            } else {

                /* uncle is BLACK */
                if (x == x->parent->right) {
                    /* make x a left child */
                    x = x->parent;
                    rotateLeft(x);
                }

                /* recolor and rotate */
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateRight(x->parent->parent);
            }
        } else {

            /* mirror image of above code */
            nodeType *y = x->parent->parent->left;
            if (y->color == RED) {

                /* uncle is RED */
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            } else {

                /* uncle is BLACK */
                if (x == x->parent->left) {
                    x = x->parent;
                    rotateRight(x);
                }
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateLeft(x->parent->parent);
            }
        }
    }
    root->color = BLACK;
}

statusEnum insert(keyType key, recType *rec) {
    nodeType *current, *parent, *x;

   /***********************************************
    *  allocate node for data and insert in tree  *
    ***********************************************/

    /* find future parent */
    current = root;
    parent = 0;
    while (current != NIL) {
        if (compEQ(key, current->key)) 
            return STATUS_DUPLICATE_KEY;
        parent = current;
        current = compLT(key, current->key) ?
            current->left : current->right;
    }

    /* setup new node */
    if ((x = (nodeType*)malloc (sizeof(*x))) == 0)
        return STATUS_MEM_EXHAUSTED;
    x->parent = parent;
    x->left = NIL;
    x->right = NIL;
    x->color = RED;
    x->key = key;
    x->rec = *rec;

    /* insert node in tree */
    if(parent) {
        if(compLT(key, parent->key))
            parent->left = x;
        else
            parent->right = x;
    } else {
        root = x;
    }

    insertFixup(x);
    lastFind = NULL;

    return STATUS_OK;
}

static void deleteFixup(nodeType *x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after deleting node x            *
    *************************************/

    while (x != root && x->color == BLACK) {
        if (x == x->parent->left) {
            nodeType *w = x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotateLeft (x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotateRight (w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotateLeft (x->parent);
                x = root;
            }
        } else {
            nodeType *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotateRight (x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotateLeft (w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotateRight (x->parent);
                x = root;
            }
        }
    }
    x->color = BLACK;
}

statusEnum rb_delete(keyType key) {
    nodeType *x, *y, *z;

   /*****************************
    *  delete node z from tree  *
    *****************************/

    /* find node in tree */
    if (lastFind && compEQ(lastFind->key, key))
        /* if we just found node, use pointer */
        z = lastFind;
    else {
        z = root;
        while(z != NIL) {
            if(compEQ(key, z->key)) 
                break;
            else
                z = compLT(key, z->key) ? z->left : z->right;
        }
        if (z == NIL) return STATUS_KEY_NOT_FOUND;
    }

    if (z->left == NIL || z->right == NIL) {
        /* y has a NIL node as a child */
        y = z;
    } else {
        /* find tree successor with a NIL node as a child */
        y = z->right;
        while (y->left != NIL) y = y->left;
    }

    /* x is y's only child */
    if (y->left != NIL)
        x = y->left;
    else
        x = y->right;

    /* remove y from the parent chain */
    x->parent = y->parent;
    if (y->parent)
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    else
        root = x;

    if (y != z) {
        z->key = y->key;
        z->rec = y->rec;
    }


    if (y->color == BLACK)
        deleteFixup (x);

    free (y);
    lastFind = NULL;

    return STATUS_OK;
}

statusEnum find(keyType key, recType **rec) {

   /*******************************
    *  find node containing data  *
    *******************************/

    nodeType *current = root;
    while(current != NIL) {
        if(compEQ(key, current->key)) {
            *rec = &current->rec;
            lastFind = current;
            return STATUS_OK;
        } else {
            current = compLT (key, current->key) ?
                current->left : current->right;
        }
    }
    return STATUS_KEY_NOT_FOUND;
}

static void
clearTree(nodeType *n)
{
  if (n==NIL || n==NULL)
    return;
  if (n->left!=NIL) {
    clearTree(n->left);
  }
  if (n->right!=NIL) {
    clearTree(n->right);
  }
#ifdef LIKE_ELECTRIC_FENCE
  if (n->rec.f==NULL) {  
    cerr << "unfreed memory, allocated at:\n";
    n->rec.a->print();
  }
#else
  cerr << "unfreed memory, allocated at:\n";
  n->rec.a->print();
#endif

  free(n);
}

#endif
