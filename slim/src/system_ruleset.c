/*
 * system_ruleset.c - default System Ruleset
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lmntal@ueda.info.waseda.ac.jp>
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
 * $Id: system_ruleset.c,v 1.8 2008/09/29 05:23:40 taisuke Exp $
 */

#include "lmntal.h"
#include "membrane.h"
#include "task.h"
#include "functor.h"
#include "symbol.h"

/* prototypes */

void init_default_system_ruleset(void);

/* delete out proxies connected each other */
static BOOL delete_redundant_outproxies(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
                                                          LMN_OUT_PROXY_FUNCTOR,
                                                          0);
  LmnAtomPtr o0;

  if (!ent) return FALSE;

  for (o0 = atomlist_head(ent);
       o0 != lmn_atomlist_end(ent);
       o0 = LMN_ATOM_GET_NEXT(o0)) {
    LmnAtomPtr o1;

    if(LMN_ATOM_GET_FUNCTOR(o0)==LMN_RESUME_FUNCTOR) continue;

    if (LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(o0, 1))) return FALSE;
    o1 = LMN_ATOM(LMN_ATOM_GET_LINK(o0, 1));
    if (LMN_ATOM_GET_FUNCTOR(o1) == LMN_OUT_PROXY_FUNCTOR) {
      LmnAtomPtr i0 = LMN_ATOM(LMN_ATOM_GET_LINK(o0, 0));
      LmnAtomPtr i1 = LMN_ATOM(LMN_ATOM_GET_LINK(o1, 0));
      LmnMembrane *m0 = LMN_PROXY_GET_MEM(i0);
      LmnMembrane *m1 = LMN_PROXY_GET_MEM(i1);

      if (m0 == m1) {
        REMOVE_FROM_ATOMLIST(o0); /* for efficiency */
        REMOVE_FROM_ATOMLIST(o1);
        lmn_delete_atom(o0);
        lmn_delete_atom(o1);
        lmn_mem_unify_atom_args(m0, i0, 1, i1, 1);
        REMOVE_FROM_ATOMLIST(i0);
        REMOVE_FROM_ATOMLIST(i1);
        memstack_push(m0);
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* delete in proxies connected each other */
static BOOL delete_redundant_inproxies(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
                                                          LMN_OUT_PROXY_FUNCTOR,
                                                          0);
  LmnAtomPtr o0;

  if (!ent) return FALSE;

  for (o0 = atomlist_head(ent);
       o0 != lmn_atomlist_end(ent);
       o0 = LMN_ATOM_GET_NEXT(o0)) {
    LmnAtomPtr i0, i1;

    if(LMN_ATOM_GET_FUNCTOR(o0)==LMN_RESUME_FUNCTOR) continue;

    i0 = LMN_ATOM(LMN_ATOM_GET_LINK(o0, 0));

    if (LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(i0, 1))) return FALSE;
    i1 =LMN_ATOM( LMN_ATOM_GET_LINK(i0, 1));
    if (LMN_ATOM_GET_FUNCTOR(i1) == LMN_IN_PROXY_FUNCTOR) {
      LmnAtomPtr o1 = LMN_ATOM(LMN_ATOM_GET_LINK(i1, 0));
      REMOVE_FROM_ATOMLIST(o0);
      REMOVE_FROM_ATOMLIST(o1);
      lmn_delete_atom(o0);
      lmn_delete_atom(o1);
      lmn_mem_unify_atom_args(mem, o0, 1, o1, 1);
      REMOVE_FROM_ATOMLIST(i0);
      REMOVE_FROM_ATOMLIST(i1);
      return TRUE;
    }
  }
  return FALSE;
}

/* basic arithmetic operations
 * -------------------------------------------------------------- */
#define REF_CAST(T,X) (*(T*)&(X))

static BOOL exec_iadd_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_IADD_FUNCTOR, 0);
  LmnAtomPtr op, ret; /* let op be an operation atom such as '+'/3, '-'/3, 'mod'/3, and so on */
                      /* let ret be the atom pointer for returned value (= y) */
  LmnLinkAttr ret_attr, x0_attr, x1_attr;
  int x0, x1, y;

  /* when '+'/3 operation atom does not exist, do nothing */
  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent);  op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      if (x0_attr == LMN_INT_ATTR && x1_attr == LMN_INT_ATTR) {
        x0 = (int)LMN_ATOM_GET_LINK(op, 0);
        x1 = (int)LMN_ATOM_GET_LINK(op, 1);
        y = x0 + x1;

        ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
        LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_INT_ATTR);
        LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

        mem->atom_num--; /* x1, x2 are deleted and y is produced, so atom_num is decreased by 1 */
        REMOVE_FROM_ATOMLIST(op);
        lmn_delete_atom(op);

        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL exec_isub_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_ISUB_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;;
  int x0, x1, y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent);  op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      if (x0_attr == LMN_INT_ATTR && x1_attr == LMN_INT_ATTR) {
        x0 = (int)LMN_ATOM_GET_LINK(op, 0);
        x1 = (int)LMN_ATOM_GET_LINK(op, 1);
        y = x0 - x1;

        ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
        LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_INT_ATTR);
        LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

        mem->atom_num--;
        REMOVE_FROM_ATOMLIST(op);
        lmn_delete_atom(op);

        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL exec_imul_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_IMUL_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;;
  int x0, x1, y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      if (x0_attr == LMN_INT_ATTR && x1_attr == LMN_INT_ATTR) {
        x0 = (int)LMN_ATOM_GET_LINK(op, 0);
        x1 = (int)LMN_ATOM_GET_LINK(op, 1);
        y = x0 * x1;

        ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
        LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_INT_ATTR);
        LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

        mem->atom_num--;
        REMOVE_FROM_ATOMLIST(op);
        lmn_delete_atom(op);

        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL exec_idiv_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_IDIV_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;;
  int x0, x1, y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      if (x0_attr == LMN_INT_ATTR && x1_attr == LMN_INT_ATTR) {
        x0 = (int)LMN_ATOM_GET_LINK(op, 0);
        x1 = (int)LMN_ATOM_GET_LINK(op, 1);
        if (x1 != 0) {
          y = x0 / x1;
        } else {
          continue;
        }

        ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
        LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_INT_ATTR);
        LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

        mem->atom_num--;
        REMOVE_FROM_ATOMLIST(op);
        lmn_delete_atom(op);

        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL exec_mod_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_MOD_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;;
  int x0, x1, y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      if (x0_attr == LMN_INT_ATTR && x1_attr == LMN_INT_ATTR) {
        x0 = (int)LMN_ATOM_GET_LINK(op, 0);
        x1 = (int)LMN_ATOM_GET_LINK(op, 1);
        if (x1 != 0) {
          y = x0 % x1;
        } else {
          continue;
        }

        ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
        LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_INT_ATTR);
        LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

        mem->atom_num--;
        REMOVE_FROM_ATOMLIST(op);
        lmn_delete_atom(op);

        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL exec_fadd_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_FADD_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;
  double *x0, *x1, *y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      x0  = LMN_MALLOC(double); x1  = LMN_MALLOC(double);
      y   = LMN_MALLOC(double);

      if (x0_attr == LMN_INT_ATTR) {
        *x0 = (double)LMN_ATOM_GET_LINK(op, 0);
      } else {
        LMN_FREE(x0); /* when x0 is a floating point variable,
                           the memory allocation for x0 is needless
                             because it has been already allocated */
        REF_CAST(LmnWord, x0) = LMN_ATOM_GET_LINK(op, 0);
      }
      if (x1_attr == LMN_INT_ATTR) {
        *x1 = (double)LMN_ATOM_GET_LINK(op, 1);
      } else {
        LMN_FREE(x1); /* when x1 is a floating point variable,
                           the memory allocation for x1 is needless */
        REF_CAST(LmnWord, x1) = LMN_ATOM_GET_LINK(op, 1);
      }
      *y = *x0 + *x1;

      ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
      LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_DBL_ATTR);
      LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

      mem->atom_num--;
      REMOVE_FROM_ATOMLIST(op);
      lmn_delete_atom(op);

      LMN_FREE(x0); LMN_FREE(x1); /* deallocation to prevent the memory leak error */
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL exec_fsub_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_FSUB_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;
  double *x0, *x1, *y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      x0 = LMN_MALLOC(double); x1 = LMN_MALLOC(double);
      y  = LMN_MALLOC(double);

      if (x0_attr == LMN_INT_ATTR) {
        *x0 = (double)LMN_ATOM_GET_LINK(op, 0);
      } else {
        LMN_FREE(x0);
        REF_CAST(LmnWord, x0) = LMN_ATOM_GET_LINK(op, 0);
      }
      if (x1_attr == LMN_INT_ATTR) {
        *x1 = (double)LMN_ATOM_GET_LINK(op, 1);
      } else {
        LMN_FREE(x1);
        REF_CAST(LmnWord, x1) = LMN_ATOM_GET_LINK(op, 1);
      }
      *y = *x0 - *x1;

      ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
      LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_DBL_ATTR);
      LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

      mem->atom_num--;
      REMOVE_FROM_ATOMLIST(op);
      lmn_delete_atom(op);

      LMN_FREE(x0); LMN_FREE(x1);
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL exec_fmul_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_FMUL_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;
  double *x0, *x1, *y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      x0  = LMN_MALLOC(double); x1  = LMN_MALLOC(double);
      y   = LMN_MALLOC(double);

      if (x0_attr == LMN_INT_ATTR) {
        *x0 = (double)LMN_ATOM_GET_LINK(op, 0);
      } else {
        LMN_FREE(x0);
        REF_CAST(LmnWord, x0) = LMN_ATOM_GET_LINK(op, 0);
      }
      if (x1_attr == LMN_INT_ATTR) {
        *x1 = (double)LMN_ATOM_GET_LINK(op, 1);
      } else {
        LMN_FREE(x1);
        REF_CAST(LmnWord, x1) = LMN_ATOM_GET_LINK(op, 1);
      }
      *y = *x0 * *x1;

      ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
      LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_DBL_ATTR);
      LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

      mem->atom_num--;
      REMOVE_FROM_ATOMLIST(op);
      lmn_delete_atom(op);

      LMN_FREE(x0); LMN_FREE(x1);
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL exec_fdiv_operation_on_body(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_ARITHMETIC_FDIV_FUNCTOR, 0);
  LmnAtomPtr op, ret;
  LmnLinkAttr ret_attr, x0_attr, x1_attr;
  double *x0, *x1, *y;

  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    x0_attr = LMN_ATOM_GET_ATTR(op, 0);
    x1_attr = LMN_ATOM_GET_ATTR(op, 1);
    ret_attr = LMN_ATOM_GET_ATTR(op, 2);
    if (LMN_ATTR_IS_DATA(x0_attr) && LMN_ATTR_IS_DATA(x1_attr)) {
      x0  = LMN_MALLOC(double); x1  = LMN_MALLOC(double);
      y   = LMN_MALLOC(double);

      if (x0_attr == LMN_INT_ATTR) {
        *x0 = (double)LMN_ATOM_GET_LINK(op, 0);
      } else {
        LMN_FREE(x0);
        REF_CAST(LmnWord, x0) = LMN_ATOM_GET_LINK(op, 0);
      }
      if (x1_attr == LMN_INT_ATTR) {
        *x1 = (double)LMN_ATOM_GET_LINK(op, 1);
      } else {
        LMN_FREE(x1);
        REF_CAST(LmnWord, x1) = LMN_ATOM_GET_LINK(op, 1);
      }

      if (*x1 != 0) {
        *y = *x0 / *x1;
      } else {
        LMN_FREE(x0); LMN_FREE(x1); LMN_FREE(y);
        continue;
      }

      ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 2));
      LMN_ATOM_SET_ATTR(ret, ret_attr, LMN_DBL_ATTR);
      LMN_ATOM_SET_LINK(ret, ret_attr, (LmnWord)y);

      mem->atom_num--;
      REMOVE_FROM_ATOMLIST(op);
      lmn_delete_atom(op);

      LMN_FREE(x0); LMN_FREE(x1);
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL mem_eq(LmnMembrane *mem)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset, LMN_MEM_EQ_FUNCTOR, 0);
  LmnMembrane *mem0, *mem1;
  LmnAtomPtr op, ret, out0, out1, in0, in1, result_atom;
  LmnLinkAttr out_attr0, out_attr1, ret_attr;
  BOOL result;
  if (!ent) return FALSE;

  for (op = atomlist_head(ent); op != lmn_atomlist_end(ent); op = LMN_ATOM_GET_NEXT(op)) {
    out_attr0 = LMN_ATOM_GET_ATTR(op, 0);
    if (LMN_ATTR_IS_DATA(out_attr0)) return FALSE;
    out0 = LMN_ATOM(LMN_ATOM_GET_LINK(op, 0));
    if (LMN_ATOM_GET_FUNCTOR(out0) != LMN_OUT_PROXY_FUNCTOR) {
      return FALSE;
    }

    in0 = LMN_ATOM(LMN_ATOM_GET_LINK(out0, 0));
    out_attr1 = LMN_ATOM_GET_ATTR(op, 1);
    if (LMN_ATTR_IS_DATA(out_attr1)) return FALSE;
    out1 = LMN_ATOM(LMN_ATOM_GET_LINK(op, 1));
    if (LMN_ATOM_GET_FUNCTOR(out1) != LMN_OUT_PROXY_FUNCTOR) {
          return FALSE;
    }

    in1 = LMN_ATOM(LMN_ATOM_GET_LINK(out1, 0));

    mem0 = LMN_PROXY_GET_MEM(in0);
    mem1 = LMN_PROXY_GET_MEM(in1);

    result = lmn_mem_equals(mem0, mem1);

    if(result){
      result_atom = lmn_mem_newatom(mem, LMN_TRUE_FUNCTOR);
    }else{
      result_atom = lmn_mem_newatom(mem, LMN_FALSE_FUNCTOR);
    }
    lmn_mem_unify_atom_args(mem, op, 0, op, 2);
    lmn_mem_unify_atom_args(mem, op, 1, op, 3);

    ret = LMN_ATOM(LMN_ATOM_GET_LINK(op, 4));
    ret_attr = LMN_ATOM_GET_ATTR(op, 4);

    if (LMN_ATTR_IS_DATA(ret_attr)) {
      LMN_ATOM_SET_LINK(result_atom, 0, ret);
      LMN_ATOM_SET_ATTR(result_atom, 0, ret_attr);
    }
    else {
      LMN_ATOM_SET_LINK(result_atom, 0, ret);
      LMN_ATOM_SET_ATTR(result_atom, 0, ret_attr);
      LMN_ATOM_SET_LINK(ret, LMN_ATTR_GET_VALUE(ret_attr), result_atom);
      LMN_ATOM_SET_ATTR(ret, LMN_ATTR_GET_VALUE(ret_attr), LMN_ATTR_MAKE_LINK(0));
    }

    mem->atom_num--;
    REMOVE_FROM_ATOMLIST(op);
    lmn_delete_atom(op);

    return TRUE;

  }
  return FALSE;
}


/* -------------------------------------------------------------- */

void init_default_system_ruleset()
{
  lmn_add_system_rule(lmn_rule_make_translated(delete_redundant_outproxies, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(delete_redundant_inproxies, ANONYMOUS));

  lmn_add_system_rule(lmn_rule_make_translated(exec_iadd_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_isub_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_imul_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_idiv_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_mod_operation_on_body,  ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_fadd_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_fsub_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_fmul_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(exec_fdiv_operation_on_body, ANONYMOUS));
  lmn_add_system_rule(lmn_rule_make_translated(mem_eq, ANONYMOUS));
}
