/*
 * priority_queue.h
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 *                                          <lmntal@ueda.info.waseda.ac.jp>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *    3. Neither the name of the Ueda Laboratory LMNtal Group nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/** @author Masaru Onuma
 *  library for priority queue / parallel priority queue
 */

#ifndef LMN_PQUEUE_H
#define LMN_PQUEUE_H

#include "lmntal.h"
#include "lmntal_thread.h"

#if SIZEOF_LONG == 4
#  define EMPTY_KEY   0xffffffffUL
#  define DELETED_KEY 0xfffffffeUL
#elif SIZEOF_LONG == 8
#  define EMPTY_KEY   0xffffffffffffffffUL
#  define DELETED_KEY 0xfffffffffffffffeUL
#endif

#define PQ_SIZE(Q) (Q)->tbl->cap

typedef struct PNode PNode;
struct PNode {
  unsigned int priority;
  LmnWord v;
  PNode *next;
};

typedef struct LinearPQueue LinearPQueue;
struct LinearPQueue {
  unsigned int enq_num, deq_num;
  SimpleHashtbl *tbl;
  unsigned int cap;
  pthread_mutex_t mtx;
};

typedef LinearPQueue PQueue;

PQueue *linear_pq_make(unsigned int init_size);
PQueue *linear_parallel_pq_make(unsigned int init_size);
static inline void linear_pq_enqueue(PQueue *q, LmnWord v, unsigned int priority);
static inline LmnWord linear_pq_dequeue(PQueue *q);
static inline BOOL linear_pq_contains(PQueue *q, HashKeyType key);
void linear_pq_free(PQueue *q);
static inline BOOL linear_pq_is_empty(PQueue *q);
void dump_pq(PQueue *q);
BOOL linear_pq_remove_node(PQueue *q, LmnWord v, unsigned int priority);

static inline PNode *pnode_make(LmnWord v, unsigned int priority);
void pnode_free(PNode *node);

static inline void linear_pq_lock(PQueue *q);
static inline void linear_pq_unlock(PQueue *q);


static inline void linear_pq_enqueue(PQueue *q, LmnWord v, unsigned int priority)
{
  linear_pq_lock(q);
  PNode *new = pnode_make(v, priority);
  if (linear_pq_contains(q, priority)) {
    new->next = (PNode *) hashtbl_get(q->tbl, priority);
  }
  hashtbl_put(q->tbl, priority, (HashValueType) new);
  q->enq_num++;
  linear_pq_unlock(q);
}

static inline LmnWord linear_pq_dequeue(PQueue *q)
{
  linear_pq_lock(q);
  PNode *n;
  unsigned int i;
  for (i = 0; i < PQ_SIZE(q); i++) {
    if (linear_pq_contains(q, i)) {
      n = (PNode *) hashtbl_get(q->tbl, i);
      LmnWord ret = n->v;
      if (n->next != NULL) {
        hashtbl_put(q->tbl, i, (HashValueType) n->next);
      } else {
        hashtbl_put(q->tbl, i, (HashValueType) NULL);
      }
      q->deq_num++;
      free(n);
      linear_pq_unlock(q);
      return ret;
    }
  }
  linear_pq_unlock(q);
  return EMPTY_KEY;
}


static inline BOOL linear_pq_contains(PQueue *q, HashKeyType key)
{
  return (hashtbl_contains(q->tbl, key) && hashtbl_get(q->tbl, key));
}

static inline BOOL linear_pq_is_empty(PQueue *q)
{
  return (q->enq_num == q->deq_num);
}

static inline PNode *pnode_make(LmnWord v, unsigned int priority)
{
  PNode *n = LMN_MALLOC(PNode);
  n->v = v;
  n->priority = priority;
  n->next = NULL;
  return n;
}

static inline void linear_pq_lock(PQueue *q)
{
  lmn_mutex_lock(&(q->mtx));
}

static inline void linear_pq_unlock(PQueue *q)
{
  lmn_mutex_unlock(&(q->mtx));
}


#endif

