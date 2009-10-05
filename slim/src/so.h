/*
 * so.h
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
 *    3. Neither the name of the Ueda Laboratory LMNtal Groupy LMNtal
 *       Group nor the names of its contributors may be used to
 *       endorse or promote products derived from this software
 *       without specific prior written permission.
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
 * $Id: so.h,v 1.00 2009/10/01 18:18:00 riki Exp $
 */

#include "lmntal.h"
#include "rule.h"
#include "functor.h"

struct trans_ruleset
{
  int size;
  LmnTranslated *rules;
};

struct trans_maindata
{
  int count_of_symbol;
  const char **symbol_table;
  int count_of_functor;
  LmnFunctorEntry *functor_table;
  int count_of_ruleset;
  struct trans_ruleset *ruleset_table;
  int *symbol_exchange;
  int *functor_exchange;
  int *ruleset_exchange;
};

/* 今の瞬間の仕様：slimで扱うsoファイルは, hogehoge.soという名前のとき,
   void init_hogehoge(void)を持ち、
   struct trans_maindata trans_hogehoge_maindataを持つこと */

