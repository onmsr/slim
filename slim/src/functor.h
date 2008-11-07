/*
 * functor.h - functor operations
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
 * $Id: functor.h,v 1.3 2008/09/29 05:23:40 taisuke Exp $
 */

#ifndef LMN_FUNCTOR_H
#define LMN_FUNCTOR_H

/* Functor Information */

typedef struct LmnFunctorTable {
  unsigned int size;
  unsigned int num_entry;
  struct LmnFunctorEntry *entry;
} LmnFunctorTable;

typedef struct LmnFunctorEntry {
  BOOL special;
  lmn_interned_str  module;
  lmn_interned_str  name;
  LmnArity          arity;
} LmnFunctorEntry;

extern struct LmnFunctorTable lmn_functor_table;

/* アクセスを高速にするためにマクロにする */
#define LMN_FUNCTOR_NAME_ID(F)     (lmn_functor_table.entry[(F)].name)
#define LMN_FUNCTOR_ARITY(F)    (lmn_functor_table.entry[(F)].arity)
#define LMN_FUNCTOR_MODULE_ID(F)     (lmn_functor_table.entry[(F)].module)

void lmn_functor_tbl_init(void);
void lmn_functor_tbl_destroy(void);
LmnFunctor lmn_functor_intern(lmn_interned_str module, lmn_interned_str name, int arity);

#endif /* LMN_RULE_H */

