#include "priority_queue.h"
#include "error.h"
#include <pthread.h>
#include <errno.h>

PQueue *linear_pq_make(unsigned int init_size)
{
  PQueue *q = LMN_MALLOC(PQueue);
  q->tbl = hashtbl_make(init_size);
  q->enq_num = 0;
  q->deq_num = 0;
  return q;
}

PQueue *linear_parallel_pq_make(unsigned int init_size)
{
  PQueue *q = LMN_MALLOC(PQueue);
  q->tbl = hashtbl_make(init_size);
  q->enq_num = 0;
  q->deq_num = 0;
  lmn_mutex_init(&(q->mtx));
  return q;
}

void linear_pq_free(PQueue *q)
{
  PNode *n, *m;
  unsigned int i;
  for (i = 0; i < PQ_SIZE(q); i++) {
    if (linear_pq_contains(q, i)) {
      for (n = (PNode *) hashtbl_get(q->tbl, i); n; n = m) {
        m = n->next;
        pnode_free(n);
      }
    }
  }
  hashtbl_free(q->tbl);
  LMN_FREE(q);
}

BOOL linear_pq_remove_node(PQueue *q, LmnWord v, unsigned int priority)
{
  linear_pq_lock(q);
  
  BOOL ret = FALSE;
  PNode *n, *last, *target = NULL;
  unsigned int i;
  for (i = priority+1; i < PQ_SIZE(q); i++) {
    if (linear_pq_contains(q, i)) {
      n = (PNode *) hashtbl_get(q->tbl, i);
      if (n->v == v) {
        // 最初のノードが目的のノードのとき
        if (n->next != NULL) {
          hashtbl_put(q->tbl, i, (HashValueType) n->next);
        } else {
          hashtbl_put(q->tbl, i, (HashValueType) NULL);
        }
        q->deq_num++;
        free(n);
      } else {
        while(n->next) {
          if (n->next->v == v) {
            last = n;
            target = n->next;
            break;
          }
          n = n->next;
        }
        if (target) {
          last->next = target->next;
          free(target);
          q->deq_num++;
          ret = TRUE;
        }
      }
    }
  }
  linear_pq_unlock(q);
  
  return ret;
}

void pnode_free(PNode *node)
{
  LMN_FREE(node);
}

