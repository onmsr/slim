/*
 * lmntal.h - global header file
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lang@ueda.info.waseda.ac.jp>
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
 * $Id: lmntal.h,v 1.13 2008/10/17 07:36:32 riki Exp $
 */

#ifndef LMNTAL_H
#define LMNTAL_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#endif

#define LMN_EXTERN extern

/* Some useful macros */
#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

/* This defines several auxiliary routines that are useful for debugging */
#ifndef LMN_DEBUG_HELPER
#define LMN_DEBUG_HELPER      TRUE
#endif

#ifndef LMN_DECL_BEGIN
#ifdef __cplusplus
# define LMN_DECL_BEGIN  extern "C" {
# define LMN_DECL_END    }
#else
# define LMN_DECL_BEGIN
# define LMN_DECL_END
#endif
#endif /*!defined(LMN_DECL_BEGIN)*/

LMN_DECL_BEGIN

/*----------------------------------------------------------------------
 * data types
 */

#if SIZEOF_LONG < SIZEOF_VOIDP
#error sizeof(long) < sizeof(void*)
#endif

typedef unsigned long LmnWord;
typedef unsigned char BYTE;
typedef BYTE LmnByte;

#define LMN_WORD_BYTES  SIZEOF_LONG
#define LMN_WORD_BITS   (SIZEOF_LONG*8)
#define LMN_WORD(X)     ((LmnWord)(X))

/* uint16_t is not defined if there is no 2Byte data type */
typedef uint16_t LmnFunctor;
#define LMN_FUNCTOR_BYTES (sizeof(LmnFunctor))
#define LMN_FUNCTOR_BITS (LMN_FUNCTOR_BYTES*8)
#define LMN_FUNCTOR(X) ((LmnFunctor)((X)))

/* this type must be enough to represent arity */
typedef uint8_t LmnArity;

typedef unsigned int lmn_interned_str;

typedef BYTE* LmnRuleInstr;
typedef uint16_t LmnInstrOp;
typedef uint16_t LmnInstrVar;
typedef uint16_t LmnJumpOffset;
typedef uint32_t LmnLineNum;
typedef int16_t LmnRulesetId;
typedef uint32_t LmnSubInstrSize;

typedef struct LmnMembrane LmnMembrane;

#if LMN_WORD_BYTES == 4
#define LMN_WORD_SHIFT 2
#elif LMN_WORD_BYTES == 8
#define LMN_WORD_SHIFT 3
#else
#error Word size is not 2^N
#endif

/*----------------------------------------------------------------------
 * Utility
 */

/*  Memory */

LMN_EXTERN void *lmn_calloc(size_t num, size_t size);
LMN_EXTERN void *lmn_malloc(size_t num);
LMN_EXTERN void *lmn_realloc(void *p, size_t num);
LMN_EXTERN void lmn_free (void *p);

#define LMN_NALLOC(TYPE, NUM)          ((TYPE *)lmn_malloc(sizeof(TYPE)*(NUM)))
#define LMN_CALLOC(TYPE, NUM)          ((TYPE *)lmn_calloc((NUM), sizeof(TYPE)))
#define LMN_MALLOC(TYPE)               ((TYPE *)lmn_malloc(sizeof(TYPE)))
#define LMN_REALLOC(TYPE, P, NUM)      ((TYPE *)lmn_realloc((P), (NUM) * sizeof(TYPE)))
#define LMN_FREE(P)                    (lmn_free((void*)(P)))

/* Error */
LMN_EXTERN void lmn_fatal(const char *msg, ...);

/* Assertion */

#include <assert.h>
#ifdef DEBUG
#define LMN_ASSERT(expr)   assert(expr)
#else
#define LMN_ASSERT(expr)   ((void)0)/* nothing */
#endif

/*----------------------------------------------------------------------
 * Global data
 */

/* dumpの形式 */
enum OutputFormat { DEFAULT, DOT, DEV };

struct LmnEnv {
  BOOL trace;
  BOOL show_proxy;
  BOOL show_ruleset;
  BOOL nd;
  BOOL nd_result;
  BOOL ltl;
  BOOL ltl_all;
  enum OutputFormat output_format;
  int optimization_level;
  char *load_path[256];
  int load_path_num;
};

extern struct LmnEnv  lmn_env;


/*----------------------------------------------------------------------
 * Others
 */

LMN_DECL_END

#endif /* LMNTAL_H */