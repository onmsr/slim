/*
 * mc_generator.c
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 *                                         <lmntal@ueda.info.waseda.ac.jp>
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

#include "mc.h"
#include "mc_worker.h"
#include "mc_generator.h"
#include "mc_explorer.h"
#include "state.h"
#include "vector.h"
#include "error.h"
#include "lmntal_thread.h"
#include "runtime_status.h"


/* 邪魔なので上に持ってきた */
#ifdef PROFILE
# define pop_stack(List)                                                       \
  do {                                                                         \
    State *pop = (State *)vec_pop(List);                                       \
    if (is_on_stack(pop)) unset_on_stack(pop);                                 \
    if (lmn_env.profile_level >= 3) {                                          \
      profile_remove_space(PROFILE_SPACE__OPEN_LIST, sizeof(LmnWord));         \
    }                                                                          \
  } while (0)
# define put_stack(List, St)                                                   \
  do {                                                                         \
    if (lmn_env.profile_level >= 3) {                                          \
      profile_add_space(PROFILE_SPACE__OPEN_LIST, sizeof(LmnWord));            \
    }                                                                          \
    vec_push((List), (vec_data_t)(St));                                        \
  } while (0)
# define EXECUTE_PROFILE_START()                                               \
  if (lmn_env.profile_level >= 3) {                                            \
    profile_remove_space(PROFILE_SPACE__OPEN_LIST, sizeof(Node));              \
    profile_start_timer(PROFILE_TIME__ACTIVE_FOR_IDLE_PROF);                   \
  }
# define EXECUTE_PROFILE_FINISH()                                              \
  if (lmn_env.profile_level >= 3) {                                            \
    profile_finish_timer(PROFILE_TIME__ACTIVE_FOR_IDLE_PROF);                  \
  }
# define ADD_OPEN_PROFILE(M)                                                   \
  if (lmn_env.profile_level >= 3) {                                            \
    profile_add_space(PROFILE_SPACE__OPEN_LIST, M);                            \
  }
#else
# define EXECUTE_PROFILE_START()
# define EXECUTE_PROFILE_FINISH()
# define ADD_OPEN_PROFILE(M)
# define pop_stack(List)                                                       \
  do {                                                                         \
    State *pop = (State *)vec_pop(List);                                       \
    if (is_on_stack(pop)) unset_on_stack(pop);                                 \
  } while (0)
# define put_stack(List, St)  vec_push((List), (vec_data_t)(St))
#endif



/** ------------------------------------------------------------------
 *  Depth First Search
 *  Parallel Depth First Search (Stack-Slicing Algorithm)
 */
#define DFS_WORKER_OBJ(W)                  ((McExpandDFS *)worker_generator_obj(W))
#define DFS_WORKER_OBJ_SET(W, O)           (worker_generator_obj_set(W, O))
#define DFS_WORKER_QUEUE(W)                (DFS_WORKER_OBJ(W)->q)
#define DFS_CUTOFF_DEPTH(W)                (DFS_WORKER_OBJ(W)->cutoff_depth)
#define DFS_WORKER_STACK(W)                (DFS_WORKER_OBJ(W)->stack)

/* DFS Stackを静的に分割する条件 */
#define DFS_HANDOFF_COND_STATIC(W, Stack)                                      \
  (vec_num(Stack) >= DFS_CUTOFF_DEPTH(W))
/* DFS Stackを動的に分割するためのWork Sharingの条件 */
#define DFS_HANDOFF_COND_DYNAMIC(I, N, W)                                      \
  (worker_on_dynamic_lb(W) && ((I + 1) < (N)) && !worker_is_active(worker_next(W)))
/* 分割条件 */
#define DFS_LOAD_BALANCING(Stack, W, I, N)                                     \
  (DFS_HANDOFF_COND_STATIC(W, Stack) || DFS_HANDOFF_COND_DYNAMIC(I, N, W))


static inline void    dfs_loop(LmnWorker *w,
                               Vector    *stack,
                               Vector    *new_states,
                               Automata  a,
                               Vector    *psyms);
static inline LmnWord dfs_work_stealing(LmnWorker *w);
static inline void    dfs_handoff_all_task(LmnWorker *me, Vector *tasks);
static inline void    dfs_handoff_task(LmnWorker *me,  LmnWord task);

typedef struct McExpandDFS {
  struct Vector stack;
  unsigned int cutoff_depth;
  Queue *q;
} McExpandDFS;


/* LmnWorker wにDFSのためのデータを割り当てる */
void dfs_worker_init(LmnWorker *w)
{
  McExpandDFS *mc = LMN_MALLOC(McExpandDFS);
  mc->cutoff_depth = lmn_env.cutoff_depth;

  if (!worker_on_parallel(w)) {
    vec_init(&mc->stack, 8192);
  }
  else {
    vec_init(&mc->stack, mc->cutoff_depth + 1);
    if (lmn_env.core_num == 1) {
      mc->q = new_queue();
    } else if (worker_on_dynamic_lb(w)) {
      mc->q = make_parallel_queue(LMN_Q_MRSW);
    } else {
      mc->q = make_parallel_queue(LMN_Q_SRSW);
    }
  }

  DFS_WORKER_OBJ_SET(w, mc);
}


/* LmnWorkerのDFS固有データを破棄する */
void dfs_worker_finalize(LmnWorker *w)
{
  if (worker_on_parallel(w)) {
    q_free(DFS_WORKER_QUEUE(w));
  }
  vec_destroy(&DFS_WORKER_STACK(w));
  LMN_FREE(DFS_WORKER_OBJ(w));
}


/* DFS Worker Queueが空の場合に真を返す */
BOOL dfs_worker_check(LmnWorker *w)
{
  return DFS_WORKER_QUEUE(w) ? is_empty_queue(DFS_WORKER_QUEUE(w))
                             : TRUE;
}

/* WorkerにDFSを割り当てる */
void dfs_env_set(LmnWorker *w)
{
  worker_set_mc_dfs(w);
  w->start    = dfs_start;
  w->check    = dfs_worker_check;
  worker_generator_init_f_set(w, dfs_worker_init);
  worker_generator_finalize_f_set(w, dfs_worker_finalize);
}


/* ワーカーwが輪の方向に沿って, 他のワーカーから未展開状態を奪いに巡回する.
 * 未展開状態を発見した場合, そのワーカーのキューからdequeueして返す.
 * 発見できなかった場合, NULLを返す */
static inline LmnWord dfs_work_stealing(LmnWorker *w)
{
  LmnWorker *dst;
  dst = worker_next(w);

  while (w != dst) {
    if (worker_is_active(dst) && !is_empty_queue(DFS_WORKER_QUEUE(dst))) {
      worker_set_active(w);
      worker_set_stealer(w);
      return dequeue(DFS_WORKER_QUEUE(dst));
    }
    else {
      dst = worker_next(dst);
    }
  }
  return (LmnWord)NULL;
}


/* ベクタexpandsに積まれたタスクをワーカーmeの隣接ワーカーに全てハンドオフする */
static inline void dfs_handoff_all_task(LmnWorker *me, Vector *expands)
{
  unsigned long i, n;
  LmnWorker *rn = worker_next(me);
  if (worker_id(me) > worker_id(rn)) {
    worker_set_black(me);
  }

  n = vec_num(expands);
  for (i = 0; i < n; i++) {
    enqueue(DFS_WORKER_QUEUE(rn), vec_get(expands, i));
  }

  ADD_OPEN_PROFILE(sizeof(Node) * n);
}


/* タスクtaskをワーカーmeの隣接ワーカーにハンドオフする */
static inline void dfs_handoff_task(LmnWorker *me, LmnWord task)
{
  LmnWorker *rn = worker_next(me);
  if (worker_id(me) > worker_id(rn)) {
    worker_set_black(me);
  }
  enqueue(DFS_WORKER_QUEUE(rn), task);

  ADD_OPEN_PROFILE(sizeof(Node));
}


void dfs_start(LmnWorker *w)
{
  LmnWorkerGroup *wp;
  struct Vector new_ss;
  State  *s;
  StateSpace ss;

  ss = worker_states(w);
  wp = worker_group(w);
  vec_init(&new_ss, 32);
  if (worker_on_parallel(w) && worker_id(w) > 0) {
    s = NULL;
  } else {
    s = statespace_init_state(ss);
  }

  if (!worker_on_parallel(w)) { /* DFS */
    put_stack(&DFS_WORKER_STACK(w), s);
    dfs_loop(w, &DFS_WORKER_STACK(w), &new_ss, statespace_automata(ss), statespace_propsyms(ss));
  }
  else {                        /* Stack-Slicing */
    while (!wp->mc_exit) {
      if (!s && is_empty_queue(DFS_WORKER_QUEUE(w))) {
        worker_set_idle(w);
        if (lmn_workers_termination_detection_for_rings(w)) {
          /* termination is detected! */
          break;
        } else if (worker_on_dynamic_lb(w)){
          /* 職探しの旅 */
          s = (State *)dfs_work_stealing(w);
        }
      } else {
        worker_set_active(w);
        if (s || (s = (State *)dequeue(DFS_WORKER_QUEUE(w)))) {
          EXECUTE_PROFILE_START();

          put_stack(&DFS_WORKER_STACK(w), s);
          dfs_loop(w, &DFS_WORKER_STACK(w), &new_ss, statespace_automata(ss), statespace_propsyms(ss));
          s = NULL;
          vec_clear(&DFS_WORKER_STACK(w));
          vec_clear(&new_ss);

          EXECUTE_PROFILE_FINISH();
        }
      }
    }

  }

  vec_destroy(&new_ss);
}


static inline void dfs_loop(LmnWorker *w,
                            Vector    *stack,
                            Vector    *new_ss,
                            Automata  a,
                            Vector    *psyms)
{
  while (!vec_is_empty(stack)) {
    State *s;
    AutomataState p_s;
    unsigned int i, n;

    if (workers_are_exit(worker_group(w))) break;

    /** 展開元の状態の取得 */
    s   = (State *)vec_peek(stack);
    p_s = MC_GET_PROPERTY(s, a);
    if (is_expanded(s)) {
      if (NDFS_COND(w, s, p_s)) {
        /** entering second DFS */
        ndfs_start(w, s);
      }
      pop_stack(stack);
      continue;
    } else if (!worker_ltl_none(w) && atmstate_is_end(p_s)) {
      mc_found_invalid_state(worker_group(w), s);
      pop_stack(stack);
      continue;
    }

    /* サクセッサを展開 */
    mc_expand(worker_states(w), s, p_s, &worker_rc(w), new_ss, psyms, worker_flags(w));

    if (MAP_COND(w)) map_start(w, s);

    if (!worker_on_parallel(w)) { /* Nested-DFS: postorder順を求めるDFS(再度到達した未展開状態がStackに積み直される) */
      set_on_stack(s);
      n = state_succ_num(s);
      for (i = 0; i < n; i++) {
        State *succ = state_succ_state(s, i);

        if (!is_expanded(succ)) {
          put_stack(stack, succ);
        }
      }
    }
    else {/* 並列アルゴリズム使用時 */
      if (DFS_HANDOFF_COND_STATIC(w, stack)) {
        dfs_handoff_all_task(w, new_ss);
      } else {
        n = vec_num(new_ss);
        for (i = 0; i < n; i++) {
          State *new_s = (State *)vec_get(new_ss, i);

          if (DFS_LOAD_BALANCING(stack, w, i, n)) {
            dfs_handoff_task(w, (LmnWord)new_s);
          } else {
            put_stack(stack, new_s);
          }
        }
      }
    }

    vec_clear(new_ss);
  }
}



/** -----------------------------------------------------------
 *  Breadth First Search
 *  Layer Synchronized Breadth First Search
 */

typedef struct McExpandBFS {
  Queue *cur; /* 現在のFront Layer */
  Queue *nxt; /* 次のLayer */
} McExpandBFS;


#define BFS_WORKER_OBJ(W)             ((McExpandBFS *)worker_generator_obj(W))
#define BFS_WORKER_OBJ_SET(W, O)      (worker_generator_obj_set(W, O))
#define BFS_WORKER_Q_CUR(W)           (BFS_WORKER_OBJ(W)->cur)
#define BFS_WORKER_Q_NXT(W)           (BFS_WORKER_OBJ(W)->nxt)
#define BFS_WORKER_Q_SWAP(W) do {             \
  Queue *_swap = BFS_WORKER_Q_NXT(W);         \
  BFS_WORKER_Q_NXT(W) = BFS_WORKER_Q_CUR(W);  \
  BFS_WORKER_Q_CUR(W) = _swap;                \
} while (0) /* ポインタの付け替えはatomicに処理されないので並列処理の際には注意 */

static inline void bfs_loop(LmnWorker *w, Vector *new_states, Automata a, Vector *psyms);


/* LmnWorker wにBFSのためのデータを割り当てる */
void bfs_worker_init(LmnWorker *w)
{
  McExpandBFS *mc = LMN_MALLOC(McExpandBFS);

  if (!worker_on_parallel(w)) {
    mc->cur = new_queue();
    mc->nxt = new_queue();
  }
  /* MT */
  else if (worker_id(w) == 0) {
    mc->cur = make_parallel_queue(LMN_Q_MRMW);
    mc->nxt = worker_use_lsync(w) ? make_parallel_queue(LMN_Q_MRMW)
                                  : mc->cur;
  } else {
    /* Layer Queueは全スレッドで共有 */
    mc->cur = BFS_WORKER_Q_CUR(workers_get_worker(worker_group(w), 0));
    mc->nxt = BFS_WORKER_Q_NXT(workers_get_worker(worker_group(w), 0));
  }

  BFS_WORKER_OBJ_SET(w, mc);
}

/* LmnWorkerのBFS固有データを破棄する */
void bfs_worker_finalize(LmnWorker *w)
{
  McExpandBFS *mc = (McExpandBFS *)BFS_WORKER_OBJ(w);
  if (!worker_on_parallel(w)) {
    q_free(mc->cur);
    q_free(mc->nxt);
  } else if (worker_id(w) == 0){
    q_free(mc->cur);
    if (worker_use_lsync(w)) q_free(mc->nxt);
  }
  LMN_FREE(mc);
}


/* BFS Queueが空の場合に真を返す */
BOOL bfs_worker_check(LmnWorker *w)
{
  return is_empty_queue(BFS_WORKER_Q_CUR(w))
      && is_empty_queue(BFS_WORKER_Q_NXT(w));
}


/* WorkerにBFSを割り当てる */
void bfs_env_set(LmnWorker *w)
{
  worker_set_mc_bfs(w);
  w->start    = bfs_start;
  w->check    = bfs_worker_check;
  worker_generator_init_f_set(w, bfs_worker_init);
  worker_generator_finalize_f_set(w, bfs_worker_finalize);

  if (lmn_env.bfs_layer_sync) {
    worker_set_lsync(w);
  }
}


/* 幅優先探索で状態空間を構築する */
void bfs_start(LmnWorker *w) {
  LmnWorkerGroup *wp;
  unsigned long d, d_lim;
  Vector *new_ss;
  StateSpace ss;

  ss = worker_states(w);
  wp = worker_group(w);
  d = 1;
  d_lim = lmn_env.depth_limits; /* ローカル変数に */
  new_ss = vec_make(32);

  if (!worker_on_parallel(w) || worker_id(w) == 0) { /* 重複して初期状態をenqしないようにするための条件 */
    enqueue(BFS_WORKER_Q_CUR(w),
            (LmnWord)statespace_init_state(ss));
  }

  /* start bfs  */
  if (!worker_on_parallel(w)) {
    /** >>>> 逐次 >>>> */
    while (!wp->mc_exit) {
      /* 1step展開 */
      bfs_loop(w, new_ss, statespace_automata(ss), statespace_propsyms(ss));

      if (BLEDGE_COND(w)) bledge_start(w);

      if (d_lim < ++d || is_empty_queue(BFS_WORKER_Q_NXT(w))) {
        /* 次のLayerが空の場合は探索終了 */
        /* 指定した制限の深さに到達した場合も探索を打ち切る */
        break;
      }

      BFS_WORKER_Q_SWAP(w); /* Layer Queueをswap */
    }
  }
  else if (!worker_use_lsync(w)) {
    /** >>>> 並列(Layer非同期) >>>> */
    while (!wp->mc_exit) {
      if (!is_empty_queue(BFS_WORKER_Q_CUR(w))) {
        EXECUTE_PROFILE_START();
        worker_set_active(w);
        bfs_loop(w, new_ss, statespace_automata(ss), statespace_propsyms(ss));
        worker_set_idle(w);

        vec_clear(new_ss);
        EXECUTE_PROFILE_FINISH();
      } else {
        worker_set_idle(w);
        if (lmn_workers_termination_detection_for_rings(w)) {
          /* termination is detected! */
          break;
        }
      }
    }
  } else {
    /** >>>> 並列(Layer同期) >>>> */
    worker_set_idle(w);
    while (TRUE) {
      if (!is_empty_queue(BFS_WORKER_Q_CUR(w))) {
        /**/EXECUTE_PROFILE_START();
        worker_set_active(w);
        bfs_loop(w, new_ss, statespace_automata(ss), statespace_propsyms(ss));
        worker_set_idle(w);
        vec_clear(new_ss);
        /**/EXECUTE_PROFILE_FINISH();
      }

      if (BLEDGE_COND(w)) bledge_start(w);

      BFS_WORKER_Q_SWAP(w);
      lmn_workers_synchronization(w, (void *)lmn_workers_termination_detection_for_rings);
      if (d_lim < ++d || wp->mc_exit || lmn_workers_termination_detection_for_rings(w)) {
        break;
      }
    }
  }

  vec_free(new_ss);
}


static inline void bfs_loop(LmnWorker *w, Vector *new_ss, Automata a, Vector *psyms)
{
  LmnWorkerGroup *wp = worker_group(w);
  while (!is_empty_queue(BFS_WORKER_Q_CUR(w))) { /* # of states@current layer > 0 */
    State *s;
    AutomataState p_s;
    unsigned int i;

    if (workers_are_exit(wp)) return;

    s = (State *)dequeue(BFS_WORKER_Q_CUR(w));

    if (!s) return; /* dequeueはNULLを返すことがある */

    p_s = MC_GET_PROPERTY(s, a);
    if (is_expanded(s)) {
      continue;
    } else if (!worker_ltl_none(w) && atmstate_is_end(p_s)) { /* safety property analysis */
      mc_found_invalid_state(wp, s);
      continue;
    }


    mc_expand(worker_states(w), s, p_s, &worker_rc(w), new_ss, psyms, worker_flags(w));

    if (MAP_COND(w)) map_start(w, s);
    else if (BLEDGE_COND(w)) bledge_store_layer(w, s);

    if (state_succ_num(s) == 0) {
      if (lmn_env.nd_search_end) {
        /* 最終状態探索モードの場合, 発見次第探索を打ち切る */
        workers_set_exit(wp);
        return;
      }
    } else {
      /* 展開した状態をnext layer queueに登録する */
      for (i = 0; i < vec_num(new_ss); i++) {
        enqueue(BFS_WORKER_Q_NXT(w), (LmnWord)vec_get(new_ss, i));
      }
    }

    vec_clear(new_ss);
  }
}

