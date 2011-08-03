/*
 * vector.h
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
 * $Id: vector.h,v 1.9 2008/09/19 05:18:17 taisuke Exp $
 */

#ifndef LMN_VECTOR_H
#define LMN_VECTOR_H

#include "lmntal.h"

struct Vector {
  LmnWord* tbl;
  unsigned int num, cap;
};


typedef struct Vector *PVector;
typedef LmnWord vec_data_t;

Vector *vec_init(Vector *vec, unsigned int init_size);
Vector *vec_make(unsigned int init_size);
void vec_push(Vector *vec, LmnWord keyp);
LmnWord vec_pop(Vector *vec);
LmnWord vec_pop_n(Vector *vec, unsigned int n);
LmnWord vec_peek(const Vector *vec);
inline void vec_set(Vector *vec, unsigned int index, LmnWord keyp);
inline LmnWord vec_get(const Vector *vec, unsigned int index);
LmnWord vec_last(Vector *vec);
BOOL vec_contains(const Vector *vec, LmnWord keyp);
void vec_clear(Vector *vec);
void vec_destroy(Vector *vec);
void vec_free(Vector *vec);
Vector *vec_copy(Vector *vec);
void vec_reverse(Vector *vec);
void vec_resize(Vector *vec, unsigned int size, vec_data_t val);
#define vec_cap(V)      ((V)->cap)
#define vec_num(V)      ((V)->num)
#define vec_is_empty(V) ((V)->num == 0)
void vec_sort(const Vector *vec,
              int (*compare)(const void*, const void*));

inline static unsigned long vec_space_inner(Vector *v)
{
  return vec_cap(v) * sizeof(vec_data_t);
}

inline static unsigned long vec_space(Vector *v)
{
  return sizeof(struct Vector) + vec_space_inner(v);
}

#endif /* LMN_VECTOR_H */