/*
 * hyperlink.h
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


#ifndef LMN_HYPERLINK_H
#define LMN_HYPERLINK_H

#include "lmntal.h"
#include "functor.h"
#include "internal_hash.h"
#include "vector.h"

/* ----------------------------------------------------------------------- *
 *  hyperlink system                                                       *
 * ----------------------------------------------------------------------- */

/* symbol atom'!'の第0引数目は接続元のシンボルアトム
 *                 第1引数目はhlink属性としてHyperLink構造体
 * が埋め込まれている */

/* HyperLink 構造体
 *   '!'をファンクタに持つシンボルアトムごとに生成される
 *   '!'アトムのポインタをIDとして利用しているが、出力（とuniqの履歴生成）の際には
 *   idの値を見かけ上のIDとして利用している
 */
typedef struct HyperLink{
  LmnSAtom atom;    /* 対応する'!'アトムのポインタ、atomが開放されているときはNULL */
  LmnHlinkRank rank;
  LmnMembrane *mem; /* atom の所属膜（findatomで使用）*/
  unsigned long id; /* 集合を一意に識別するID (主に出力とuniqの履歴生成の際に使用) */
//  long usrid;        /* ユーザがhyperlinkのidを決められるようにするための変数（未実装）*/
  LmnAtom attrAtom;/* ハイパーリンクの属性として扱うアトム rootにのみ持たせる */
  LmnLinkAttr attr; /* 属性アトムの属性(0なら属性はなし) rootにのみ持たせる */  
  /* 木構造による併合関係の表現 */
  struct HyperLink *parent; /* root の場合は自身のポインタ */
  struct HashSet *children; /* 子表 */

} HyperLink;

#define LMN_HL_EMPTY_ATTR (0)

#define LMN_HL_FUNC LMN_EXCLAMATION_FUNCTOR

#define LMN_HL_RANK(HL)     ((HL)->rank)
#define LMN_HL_MEM(HL)      ((HL)->mem)
#define LMN_HL_ID(HL)       ((HL)->id)
#define LMN_SET_HL_ID(HL,ID)       ((HL)->id = (ID))
#define LMN_HL_ATTRATOM(HL)     ((lmn_hyperlink_get_root(HL))->attrAtom)
#define LMN_HL_ATTRATOM_ATTR(HL) ((lmn_hyperlink_get_root(HL))->attr)
#define LMN_HL_HAS_ATTR(HL)     (LMN_HL_ATTRATOM_ATTR(lmn_hyperlink_get_root(HL)) || LMN_HL_ATTRATOM(lmn_hyperlink_get_root(HL)))

#define LMN_HL_ATOM_ROOT_HL(ATOM)   lmn_hyperlink_get_root(lmn_hyperlink_at_to_hl(ATOM))
#define LMN_HL_ATOM_ROOT_ATOM(ATOM) lmn_hyperlink_hl_to_at(lmn_hyperlink_get_root(lmn_hyperlink_at_to_hl(ATOM)))


#define LMN_IS_HL(ATOM)      (LMN_FUNC_IS_HL(LMN_SATOM_GET_FUNCTOR(ATOM)))
#define LMN_FUNC_IS_HL(FUNC) ((FUNC) == LMN_HL_FUNC)
#define LMN_ATTR_IS_HL(ATTR) ((ATTR) == LMN_HL_ATTR)


void lmn_hyperlink_make(LmnSAtom at);
void lmn_hyperlink_put_attr(HyperLink *hl, LmnAtom attrAtom, LmnLinkAttr attr);
void lmn_hyperlink_make_with_attr(LmnSAtom at, LmnAtom attrAtom, LmnLinkAttr attr);
LmnSAtom lmn_hyperlink_new();
LmnSAtom lmn_hyperlink_new_with_attr(LmnAtom attrAtom, LmnLinkAttr attr);
void lmn_hyperlink_delete(LmnSAtom at);
void lmn_hyperlink_copy(LmnSAtom newatom, LmnSAtom oriatom);
HyperLink *lmn_hyperlink_at_to_hl(LmnSAtom at);
LmnSAtom   lmn_hyperlink_hl_to_at(HyperLink *hl);
HyperLink *lmn_hyperlink_get_root(HyperLink *hl);
HyperLink *hyperlink_unify(HyperLink *parent, HyperLink *child, LmnAtom, LmnLinkAttr);
HyperLink *lmn_hyperlink_unify(HyperLink *hl1, HyperLink *hl2, LmnAtom, LmnLinkAttr);
int  lmn_hyperlink_rank(HyperLink *hl);
int  lmn_hyperlink_element_num(HyperLink *hl);
BOOL lmn_hyperlink_eq_hl(HyperLink *hl1, HyperLink *hl2);
BOOL lmn_hyperlink_eq(LmnSAtom atom1, LmnLinkAttr attr1, LmnSAtom atom2, LmnLinkAttr attr2);
void lmn_hyperlink_print(LmnMembrane *gr);


/* ----------------------------------------------------------------------- *
 *  same proccess context (同名型付きプロセス文脈)                         *
 *  hyperlink の接続関係を利用したルールマッチング最適化                   *
 * ----------------------------------------------------------------------- */

/* 同名型付きプロセス文脈を持つ引数ごとに生成される */
typedef struct ProcCxt {
  int atomi;
  int arg;
  HyperLink *start;
  struct ProcCxt *original;
} ProcCxt;

#define LMN_PC_ATOMI(PC)  ((PC)->atomi)
#define LMN_PC_START(PC)  ((PC)->start)
#define LMN_PC_ORI(PC)    ((PC)->original)
#define LMN_PC_IS_ORI(PC) (LMN_PC_ORI(PC) == NULL)

/* 同名型付きプロセス文脈を持つアトムごとに生成される */
typedef struct SameProcCxt {
  int atomi;       /* findatom の結果を格納するwt[atomi] のatomi */
  int length;      /* wt[atomi] に格納するアトムのarity */
  void **proccxts; /* 長さlength のProcCxt 配列 */

  /* findatom 内で使用される一時領域 */
  Vector *tree; /* HyperLink tree */
  LmnLinkAttr start_attr;

} SameProcCxt;

#define LMN_SPC_TREE(SPC)  ((SPC)->tree)
#define LMN_SPC_SATTR(SPC) ((SPC)->start_attr)
#define LMN_SPC_PC(SPC, I) ((SPC)->proccxts[(I)])


void lmn_sameproccxt_init(LmnReactCxt *rc);
void lmn_sameproccxt_clear(LmnReactCxt *rc);
SameProcCxt *lmn_sameproccxt_spc_make(int atomi, int length);
ProcCxt *lmn_sameproccxt_pc_make(int atomi, int arg, ProcCxt *original);
BOOL lmn_sameproccxt_from_clone(SameProcCxt *spc, int n);
HyperLink *lmn_sameproccxt_start(SameProcCxt *spc, int atom_arity);
BOOL lmn_sameproccxt_all_pc_check_original(SameProcCxt *spc, LmnSAtom atom, int atom_arity);
BOOL lmn_sameproccxt_all_pc_check_clone(SameProcCxt *spc, LmnSAtom atom, int atom_arity);
void lmn_hyperlink_get_elements(Vector *tree, HyperLink *start_hl);

// ハイパーリンクハッシュ値関連 @onuma
#define HLHASH_DEPTH 2
unsigned long lmn_hlhash(HyperLink *hl);
unsigned long lmn_hlhash_sub(LmnWord atom);
unsigned long lmn_hlhash_depth(HyperLink *hl,int depth);
void lmn_hlhash_depth_sub(LmnAtom atom, LmnLinkAttr attr, int i_parent, unsigned long *sum, int depth);

// ハイパーリンクから膜への変換 @onuma
// encode
void lmn_convert_hl_to_mem_root(LmnMembrane *gr);
void lmn_convert_hl_to_mem(LmnMembrane *gr,LmnMembrane *mem);
void lmn_convert_hl_to_mem_sub(LmnMembrane *gr,LmnMembrane *mem,HyperLink *hl);
// decode
void lmn_convert_mem_to_hl_root(LmnMembrane *gr);
void lmn_convert_mem_to_hl(LmnMembrane *hlmem);
// others
void lmn_link_at_to_at(LmnAtom *at1,int pos1,LmnMembrane *mem1,LmnAtom *at2,int pos2,LmnMembrane *mem2);
void lmn_unlink_at_to_at(LmnAtom *at1,int pos1,LmnMembrane *mem1,LmnAtom *at2,int pos2,LmnMembrane *mem2);
unsigned int lmn_get_mem_depth(LmnMembrane *mem);
int lmn_get_atom_link_num(LmnSAtom srcAt,LmnSAtom destAt);
void lmn_hyperlink_delete_from_mem(HyperLink *hl);
void lmn_hyperlink_delete_all(HyperLink *hl);
unsigned long get_max_id(LmnMembrane *mem);

#define LMN_HYPERLINK_MEM 444
#define LMN_HLMEM_ATOM_FUNCTOR 16
#define HYPERMEM 1

void lmn_link_test(LmnMembrane *gr);


/* ハイパーリンクhlのハッシュ値を返す. */
static inline unsigned long lmn_hyperlink_hash(HyperLink *hl) {
  if(lmn_env.hlhash){  // ハイパーリンクのハッシュ値計算改良版
    return lmn_hlhash_depth(hl, HLHASH_DEPTH);
    // return lmn_hlhash(hl);
  }
  return lmn_hyperlink_element_num(hl);
}

#endif /* LMN_HYPERLINK_H */
