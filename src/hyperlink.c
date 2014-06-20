/*
 * hyperlink.c
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

#include "hyperlink.h"
#include "atom.h"
#include "membrane.h"
#include "react_context.h"

#if SIZEOF_LONG == 4
#  define EMPTY_KEY   0xffffffffUL
#  define DELETED_KEY 0xfffffffeUL
#elif SIZEOF_LONG == 8
#  define EMPTY_KEY   0xffffffffffffffffUL
#  define DELETED_KEY 0xfffffffffffffffeUL
#endif


/* ----------------------------------------------------------------------- *
 *  hyperlink system                                                       *
 * ----------------------------------------------------------------------- */


/*
 * 非決定実行への対応 :
 *   非決定実行に対応させる場合は並列化が必要
 *   - commit命令での作業配列のコピー操作に、newhlinkなどでのシンボルアトム生成を対応させる
 *   - グローバル変数hl_sameproccxt は実行中のルールごとに必要
 */

/* prototype */
void sht_print(SimpleHashtbl *sht);
void hs_print(HashSet *hs);
static inline void sameproccxt_destroy(SimpleHashtbl *sht);


static inline unsigned long hyperlink_new_id()
{
  return env_gen_next_id(); /* nd実行のために変更 */
}


//HyperLink *lmn_hyperlink_make(LmnSAtom sa)
void lmn_hyperlink_make(LmnSAtom at)
{
  HyperLink *hl;

  hl = LMN_MALLOC(HyperLink);
  hl->atom     = at;
  hl->rank     = 0;
  hl->mem      = NULL;
  hl->id       = hyperlink_new_id();
//  hl->usrid = 0;
  hl->parent   = hl;
  hl->children = NULL;
  hl->attrAtom = 0;
  hl->attr     = 0;

  LMN_SATOM_SET_LINK(LMN_SATOM(at), 1, (LmnWord)hl);
  LMN_SATOM_SET_ATTR(LMN_SATOM(at), 1, 0);

//  hashtbl_put(at_hl, (HashKeyType)at, (HashValueType)hl);
//  printf("lmn_hyperlink_make %p -> %p\n", hl, LMN_SATOM(hl->atom));
}

void lmn_hyperlink_put_attr(HyperLink *hl, LmnAtom attrAtom, LmnLinkAttr attr)
{
  hl->attrAtom = attrAtom;
  hl->attr = attr;
}

void lmn_hyperlink_make_with_attr(LmnSAtom at, LmnAtom attrAtom, LmnLinkAttr attr)
{
  lmn_hyperlink_make(at);
  lmn_hyperlink_put_attr(lmn_hyperlink_at_to_hl(at), attrAtom, attr);
}

/* 新しいhyperlinkの生成 */
LmnSAtom lmn_hyperlink_new()
{
  LmnSAtom atom;

  atom = lmn_new_atom(LMN_HL_FUNC);
  LMN_SATOM_SET_ID(atom, hyperlink_new_id());
  lmn_hyperlink_make(atom);

  return atom;
}

LmnSAtom lmn_hyperlink_new_with_attr(LmnAtom attrAtom, LmnLinkAttr attr)
{
  LmnSAtom atom;
  atom = lmn_hyperlink_new();
  lmn_hyperlink_put_attr(lmn_hyperlink_at_to_hl(atom), attrAtom, attr);
  return atom;
}

/* rootまでの全ての親のrankにdの値を加算する */
void hyperlink_rank_calc(HyperLink *hl, int d)
{
  HyperLink *parent, *current;

  hl->rank += d;
  current   = hl;
  parent    = hl->parent;
  while (parent != current) {
//    pro1++;
    parent->rank += d;
    current = parent;
    parent = parent->parent;
  }
}


/* HyperLinkのatom, mem, attrAtom, attrのみを交換する */
void hyperlink_swap_atom(HyperLink *hl1, HyperLink *hl2)
{
  LmnSAtom t_atom;
  LmnMembrane *t_mem;

  t_atom    = hl1->atom;
  hl1->atom = hl2->atom;
  hl2->atom = t_atom;

  LMN_SATOM_SET_LINK(hl1->atom, 1, (LmnWord)hl1);
  LMN_SATOM_SET_LINK(hl2->atom, 1, (LmnWord)hl2);

  t_mem     = hl1->mem;
  hl1->mem  = hl2->mem;
  hl2->mem  = t_mem;
}


/* 子表に格納されている子のうち先頭のものを返す、子表が無ければNULLを返す */
HyperLink *hyperlink_head_child(HyperLink *hl)
{
  HashSet *children = hl->children;
  if (children) {
    HashSetIterator it;

    for (it = hashset_iterator(children); !hashsetiter_isend(&it); hashsetiter_next(&it)) {
      HyperLink *child = (HyperLink *)hashsetiter_entry(&it);
      if ((HashKeyType)child < DELETED_KEY) {
        return child;
      }
    }
  }

  return NULL;
}


/*
 * atom の削除に呼応してHyperLink 構造体を削除する（最適化 ver.）
 *
 * 子をたくさん持つHyperLink を削除してしまうと, 削除後の木構造の再構築に時間がかかる（全ての子に対して親の更新を行なうなど）ため、
 * 削除するHyperLink は木構造の葉であることが望ましい
 *
 * HyperLink 木は変更せずに、HyperLink に対応しているatom を親子間での交換を繰り返して
 * 末端に向かって移動させ、葉に到達した時点でその位置のHyperLink を削除する
 */
void lmn_hyperlink_delete(LmnSAtom at)
{
  HyperLink *hl = lmn_hyperlink_at_to_hl(at);

  if (hl) {
    HyperLink *parent;

    while (hl->children) {
      HyperLink *child = hyperlink_head_child(hl);
      hyperlink_swap_atom(hl, child);
      hl = child;
    }

    parent = hl->parent;
    if (parent != hl) {
      hashset_delete(parent->children, (HashKeyType)hl);
      hyperlink_rank_calc(parent, -1);
      if (parent->rank == 0) {
        hashset_free(parent->children);
        parent->children = NULL;
      }
    }

    LMN_FREE(hl);
  }
}


/* atom の削除に呼応してHyperLink 構造体を削除する
 *
 * 旧式、使用していないが一応残しておく
 * hyperlinkの削除では何をしているかを把握するためにはいいかも
 */
void lmn_hyperlink_delete_old(LmnSAtom at)
{
  HyperLink *hl, *parent;
  HashSet *children;

  hl = lmn_hyperlink_at_to_hl(at);
  if (!hl) return;

  parent   = hl->parent;
  children = hl->children;

  if (parent != hl) {
    /* root でない場合：
     * 親の子表から自身を削除し, 親のrankを-1した後,
     *   子がいる   -> 親の子表に自身の子を移す
     *   子がいない -> そのまま削除
     */

    hashset_delete(parent->children, (HashKeyType)hl);
//    parent->rank--;
    hyperlink_rank_calc(parent, -1);
    if (parent->rank == 0) {
      hashset_free(parent->children);
      parent->children = NULL;
    }

    if (children) { // 子表があるとき
      HashSetIterator it;
      for (it = hashset_iterator(children);
           !hashsetiter_isend(&it);
           hashsetiter_next(&it)) {
        HyperLink *tmp = (HyperLink *)hashsetiter_entry(&it);
        if ((HashKeyType)tmp < DELETED_KEY) {
          hashset_add(parent->children, (HashKeyType)tmp);
          hashset_delete(children, (HashKeyType)tmp);
          tmp->parent = parent;
        }
      }
    }
  }
  else {
    /* root である場合：
     *   子がいる   -> rankが最大の子を新rootにする
     *   子がいない -> そのまま削除
     */
    if (children) { /* 子表があるとき */
      HyperLink *newroot;
      HashSetIterator it;

      newroot = NULL;
      for (it = hashset_iterator(children);
           !hashsetiter_isend(&it);
           hashsetiter_next(&it)) {
        newroot = (HyperLink *)hashsetiter_entry(&it);
        if ((HashKeyType)newroot < DELETED_KEY) {
          break; /* 現状では先頭の子を新しい親にしている */
        }
      }

      /* 新rootが決定 */
      hashset_delete(children, (HashKeyType)newroot); /* rootの子表から新rootを除去 */
      newroot->parent = newroot;

      if (!newroot->children) {
        newroot->children = hashset_make(hashset_num(children));
      }

      for (it = hashset_iterator(children);
           !hashsetiter_isend(&it);
           hashsetiter_next(&it)) {
        HyperLink *tmp = (HyperLink *)hashsetiter_entry(&it);
        if ((HashKeyType)tmp < DELETED_KEY) {
          hashset_add(newroot->children, (HashKeyType)tmp);
          hashset_delete(children, (HashKeyType)tmp);
          tmp->parent = newroot;
        }
      }

      newroot->rank = parent->rank - 1;
      if (newroot->rank == 0) {
        hashset_free(newroot->children);
        newroot->children = NULL;
      }
    }
  }

  if (children) {
    hashset_free(children);
  }
  LMN_FREE(hl);
}



/* hyperlinkのコピー
 * <=> 新しいHyperLink 構造体を生成し、newatomに対応づけた後、oriatomと併合する
 */
void lmn_hyperlink_copy(LmnSAtom newatom, LmnSAtom oriatom)
{
  HyperLink *newhl, *orihl;

  orihl = lmn_hyperlink_at_to_hl(oriatom);
  lmn_hyperlink_make(newatom);
  newhl = lmn_hyperlink_at_to_hl(newatom);

  lmn_hyperlink_unify(lmn_hyperlink_get_root(orihl), newhl, LMN_HL_ATTRATOM(orihl), LMN_HL_ATTRATOM_ATTR(orihl));
}

/* Union-Find algorithm の最適化 (Path Compression)
 *   あるHyperLink からlmn_hyperlink_get_root でroot まで辿ったとき、
 *   その経路上にある全てのHyperLink をroot の直接の子として再設定する
 */
void hyperlink_path_compression(HyperLink *root, Vector *children)
{
  int i, n;

  n = vec_num(children);
  for (i = 0; i < n; i++) {
    HyperLink *hl, *old_parent;

    hl = (HyperLink *)vec_get(children, i);
    old_parent = hl->parent;

    if (old_parent != root) {
      HashSet *old_parent_children;
      int j, sub_rank;

      /* 旧親に対する処理 */
      old_parent_children = old_parent->children;
      hashset_delete(old_parent_children, (HashKeyType)hl);
      sub_rank = hl->rank + 1;
      for (j = i + 1; j < n; j++) {
        ((HyperLink *)vec_get(children, j))->rank -= sub_rank;
      }

      if (hashset_num(old_parent_children) == 0) {
        hashset_free(old_parent_children);
        old_parent->children = NULL;
      }

      /* 新親(root)に対する処理 */
      hl->parent = root;
      hashset_add(root->children, (HashKeyType)hl);
    }
  }
}


/* root を返す */
HyperLink *lmn_hyperlink_get_root(HyperLink *hl)
{
  HyperLink *parent_hl, *current_hl;
  if (hl->parent == hl) return hl;
  current_hl = hl;
  parent_hl  = hl->parent;

  /* hlとrootの間に他のHyperLinkが無ければpath compressionは起こらない
   * ＝ 要素数が2以下であれば、path compressionは起こらない
   */
  if (lmn_hyperlink_element_num(parent_hl) <= 2) {
    while (parent_hl != current_hl) {
      current_hl = parent_hl;
      parent_hl  = current_hl->parent;
    }
  }
  else {
    Vector children;
    vec_init(&children, lmn_hyperlink_element_num(parent_hl));

    while (parent_hl != current_hl) {
      vec_push(&children, (LmnWord)current_hl);
      current_hl = parent_hl;
      parent_hl  = current_hl->parent;
    }

    if (!vec_is_empty(&children)) {
      hyperlink_path_compression(parent_hl, &children); /* parent_hlはrootになっている */
    }

    vec_destroy(&children);
  }

  return parent_hl;
}


/* child をparent の子として併合する（parent, childは共にroot）*/
HyperLink *hyperlink_unify(HyperLink *parent, HyperLink *child, LmnAtom attrAtom, LmnLinkAttr attr)
{
  child->parent = parent;
  if (!parent->children) {
    parent->children = hashset_make(2);
  }
  hashset_add(parent->children, (HashKeyType)child);
  parent->rank = parent->rank + child->rank + 1;
  parent->attrAtom = attrAtom;
  parent->attr = attr;
  child->attrAtom = 0;
  child->attr = 0;

  return parent;
}


/* 2 つのhyperlink を併合し、親となった方を返す
 *   rank のより大きい(子を多く持つ)方を親とする
 *   rankが等しい場合はhl1をhl2の親とする
 *   attrで指定された属性を併合後のハイパーリンクの属性とする。
 * */
HyperLink *lmn_hyperlink_unify(HyperLink *hl1, HyperLink *hl2, LmnAtom attrAtom,  LmnLinkAttr attr)
{
  HyperLink *root1, *root2, *result;
  int rank1, rank2;

  root1 = lmn_hyperlink_get_root(hl1);
  root2 = lmn_hyperlink_get_root(hl2);

  if (root1 == root2) return root1;

  rank1 = hl1->rank;
  rank2 = hl2->rank;
//  printf("rank %p %d %p %d\n", hl1, rank1, hl2, rank2);
  if (rank1 >= rank2) {
    result = hyperlink_unify(root1, root2, attrAtom, attr);
  } else {
    result = hyperlink_unify(root2, root1, attrAtom, attr);
  }

  return result;
}


/* '!'アトムのポインタ --> 対応するHyperLink 構造体のポインタ */
HyperLink *lmn_hyperlink_at_to_hl(LmnSAtom at)
{
  return (HyperLink *)LMN_SATOM_GET_LINK(at, 1);
}


/* HyperLink 構造体のポインタ --> 対応する'!'アトムのポインタ */
LmnSAtom lmn_hyperlink_hl_to_at(HyperLink *hl)
{
//  if (hl->atom) return hl->atom;
//  else return 0;
  return hl->atom;
}


/* rank を返す */
int lmn_hyperlink_rank(HyperLink *hl)
{
  return LMN_HL_RANK(lmn_hyperlink_get_root(hl));
}


/* hyperlink の要素数(rank + 1)を返す */
int lmn_hyperlink_element_num(HyperLink *hl)
{
  return (lmn_hyperlink_rank(hl) + 1);
}


/* hyperlink 同士の比較 */
BOOL lmn_hyperlink_eq_hl(HyperLink *hl1, HyperLink *hl2)
{
  return lmn_hyperlink_hl_to_at(lmn_hyperlink_get_root(hl1)) ==
           lmn_hyperlink_hl_to_at(lmn_hyperlink_get_root(hl2));
}


/* hyperlink 同士の比較（'!'アトムポインタから直接） */
BOOL lmn_hyperlink_eq(LmnSAtom atom1, LmnLinkAttr attr1, LmnSAtom atom2, LmnLinkAttr attr2)
{
  return LMN_ATTR_IS_HL(attr1) &&
         LMN_ATTR_IS_HL(attr2) &&
         lmn_hyperlink_eq_hl(lmn_hyperlink_at_to_hl(atom1),
                             lmn_hyperlink_at_to_hl(atom2));
}


/* hyperlink を1 つ出力
 *   hyperlink が1 つでも出力されるとTRUE を返す */
BOOL hyperlink_print(LmnMembrane *mem, BOOL *flag, int *group, int *element)
{
  AtomListEntry *atomlist;
  LmnMembrane *m;
  LmnSAtom atom;
  HyperLink *hl, *parent;
  HashSet *children;
  int WIDTH;
  BOOL result;
  FILE *f; /*  出力先は呼び出し側から指定させたい */

  f = stdout;
  result = FALSE;
  WIDTH  = 22;
  if ((atomlist = lmn_mem_get_atomlist(mem, LMN_HL_FUNC))) {
    EACH_ATOM(atom, atomlist, ({
      result = TRUE;

      if (!(*flag)) {
        fprintf(f, "%9s %9s %13s %5s %5s\n", "[hl_ID]", "[parent]", "[linked with]", "[num]", "[direct children ( inside info )]");
        (*flag) = TRUE;
      }
      hl = lmn_hyperlink_at_to_hl(atom);

      /* hl_ID */
//      fprintf(f, "%9lx", LMN_ATOM(atom));
      fprintf(f, "%9lx", LMN_HL_ID(lmn_hyperlink_at_to_hl(atom)));
//      fprintf(f, "%9lx", LMN_HL_ID(lmn_hyperlink_get_root(lmn_hyperlink_at_to_hl(atom))));

      /* parent */
      if ((parent = hl->parent) != hl) {
//        fprintf(f, " %9lx", LMN_ATOM(parent->atom));
        fprintf(f, " %9lx", LMN_HL_ID(parent));
      } else {
        (*group)++;
        fprintf(f, " %9s", "root");
      }

      /* linked with */
      if (!LMN_ATTR_IS_DATA(LMN_SATOM_GET_ATTR(atom, 0)) && LMN_SATOM_GET_LINK(atom, 0)) {
        fprintf(f, " %13s", LMN_SATOM_STR(LMN_SATOM_GET_LINK(atom, 0)));
      } else {
        fprintf(f, " %13s", "---");
      }

      /* element num */
      fprintf(f, " %5d  ", lmn_hyperlink_element_num(hl));

      /* (direct children) */
      if ((children = hl->children)) {
        HashSetIterator hsit;
        HyperLink *ch_hl;
        int i, n, width;
        BOOL comma;

        width = 0;
        comma = FALSE;
        i = 1;
        for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
          if ((HashKeyType)(ch_hl = (HyperLink *)hashsetiter_entry(&hsit)) < DELETED_KEY) {
            if (!comma) {
              comma = TRUE;
            } else {
              if (width > WIDTH - 4) {
                fprintf(f, ",\n%41s", "");
                width = 0;
              } else {
                fprintf(f, ",");
              }
            }

//            fprintf(f, "%lx", LMN_ATOM(ch_hl->atom));
            fprintf(f, "%lx%n", LMN_HL_ID(ch_hl), &n);
            width += n;
            i++;
          }
        }
        fprintf(f, ".");
      }

      (*element)++;
      fprintf(f, "\n");
    }));
  }
//  else result = FALSE;

  for (m = mem->child_head; m; m = m->next) {
    result = (hyperlink_print(m, flag, group, element) || result);
  }

  return result;
}



/* num >= 0 */
int hyperlink_print_get_place(int num) {
  int place, tmp;

  place = 1;
  tmp = num;
  while(tmp >= 10) {
    tmp = tmp / 10;
    place++;
  }

  return place;
}


/* グローバルルート膜から順に辿って、存在する全てのhyperlink を出力する */
void lmn_hyperlink_print(LmnMembrane *gr)
{
  FILE *f;
  int WIDTH, group, element, place_g, place_e;
  char tail_g[8], tail_e[14];
  BOOL flag;

  f = stdout;
  element = 0;
  group = 0;
  flag = FALSE;
  fprintf(f, "== HyperLink =============================================================%n\n", &WIDTH);
  if (!hyperlink_print(gr, &flag, &group, &element)) fprintf(f, "There is no hyperlink.\n");

  place_g = hyperlink_print_get_place(group);
  place_e = hyperlink_print_get_place(element);

  if (group < 2)   sprintf(tail_g, "group, ");
  else             sprintf(tail_g, "groups,");

  if (element < 2) sprintf(tail_e, "element =====");
  else             sprintf(tail_e, "elements ====");

  place_e = WIDTH - sizeof(tail_g) - sizeof(tail_e) - (place_g + 1) - (place_e + 1);
  while (place_e > 0) {
    fprintf(f, "=");
    place_e--;
  }
  fprintf(f, " %d %s %d %s\n", group, tail_g, element, tail_e);
//  fprintf(f, "\n");

}


/* 旧式
 * [hl_ID]  [parent] [linked with] [num] [direct children (inside info)]
 */
void hyperlink_print_old()
{
//  BOOL item;
//
//  item = FALSE;
//  printf("== HyperLink =============================================================\n");
//  if (at_hl) {
//    HashIterator it;
//    HyperLink *hl, *parent;
//    LmnSAtom atom;
//    HashSet *children;
//
//    for (it = hashtbl_iterator(at_hl); !hashtbliter_isend(&it); hashtbliter_next(&it)) {
//      if ((HashValueType)(hl = (HyperLink *)(hashtbliter_entry(&it)->data)) < DELETED_KEY) {
//        if (!item) {
//          printf("%9s %9s %13s %5s %5s\n", "[hl_ID]", "[parent]", "[linked with]", "[num]", "[direct children ( inside info )]");
//          item = TRUE;
//        }
//
//        /* hl_ID */
//        printf("%9lx", (atom = hl->atom));
//
//        /* parent */
//        if ((parent = hl->parent) != hl)
//          printf(" %9lx", parent->atom);
//        else
//          printf(" %9s", "root");
//
//        /* linked with */
//        if (!LMN_ATTR_IS_DATA(LMN_SATOM_GET_ATTR(atom, 0)) && LMN_SATOM_GET_LINK(atom, 0))
//          printf(" %13s", LMN_SATOM_STR(LMN_SATOM_GET_LINK(atom, 0)));
//        else
//          printf(" %13s", "---");
//
//        /* num, rank */
////        if (hl->parent == hl)
////          printf(" %5d  ", hl->rank+1);
////        else
////          printf(" %5s  ", "---");
//        printf(" %5d  ", lmn_hyperlink_element_num(hl));
////        printf(" %5d  ", hl->rank);
//
//        /* (direct children) */
//        if ((children = hl->children)) {
//          HashSetIterator hsit;
//          HyperLink *ch_hl;
//          BOOL comma;
//          int i;
//
//          comma = FALSE;
//          i = 1;
//          for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
//            if ((HashKeyType)(ch_hl = (HyperLink *)hashsetiter_entry(&hsit)) < DELETED_KEY) {
//              if (comma) {
//                if (i % 4 == 1) printf(",\n%41s", "");
//                else printf(",");
//              }
//              else comma = TRUE;
//              printf("%lx", ch_hl->atom);
//              i++;
//            }
//          }
//          printf(".");
//        }
//
//        printf("\n");
//      }
//
//    }
//
//  }
//  if (!item) printf("There is no hyperlink.\n");
//  printf("==========================================================================\n");
}

void lmn_hyperlink_print_old()
{
  hyperlink_print_old();
}


/* for debug @seiji */
void sht_print(SimpleHashtbl *sht)
{
  if (!sht) {
    printf(">>>> NULL\n");
  } else {
    int n = hashtbl_num(sht);
    int i;
    printf(">>>> sht %p num %d cap %d\n", sht, n, sht->cap);
    for (i = 0; i < sht->cap; i++) {
      if (sht->tbl[i].key == EMPTY_KEY) printf("%3d: key: %p data: %p\n", i, (void *)sht->tbl[i].key, (HyperLink *)sht->tbl[i].data);
      else {
        if (sht->tbl[i].data < DELETED_KEY){
          printf("%3d: key: %p data: %p->%p\n", i, (void *)sht->tbl[i].key, (HyperLink *)sht->tbl[i].data, (void*)((HyperLink *)sht->tbl[i].data)->atom);
        } else {
          printf("%3d: key: %p data: %p\n", i, (void *)sht->tbl[i].key, (HyperLink *)sht->tbl[i].data);
        }
      }
    }
  }
}


/* for debug @seiji */
void hs_print(HashSet *hs)
{
  if (!hs) {
    printf(">>>> NULL\n");
  } else {
    int n = hashtbl_num(hs);
    int i;
    printf(">>>>  hs %p num %d cap %d\n", hs, n, hs->cap);
    for (i = 0; i < hs->cap; i++) {
//      if (sht->tbl[i].key == EMPTY_KEY) printf("%3d: %s\n", i, "empty");
      if (hs->tbl[i] < DELETED_KEY) printf("%3d: key: %p\n", i, (void *)hs->tbl[i]);
      else {
        if (hs->tbl[i] < DELETED_KEY){
          printf("%3d: key: %p\n", i, (void *)hs->tbl[i]);
        } else {
          printf("%3d: key: %p\n", i, (void *)hs->tbl[i]);
        }
      }
    }
  }
}


/* ----------------------------------------------------------------------- *
 *  same proccess context (同名型付きプロセス文脈)                         *
 *  hyperlink の接続関係を利用したルールマッチング最適化                   *
 * ----------------------------------------------------------------------- */

//FindProcCxt *findproccxt;
//
//void lmn_sameproccxt_init2()
//{
//  findproccxt = LMN_MALLOC(FindProcCxt);
//  findproccxt = NULL;
//  commit      = FALSE;
//}

void lmn_sameproccxt_init(LmnReactCxt *rc)
{
  RC_SET_HLINK_SPC(rc, hashtbl_make(2));
}


static inline void sameproccxt_destroy(SimpleHashtbl *hl_sameproccxt)
{
  HashIterator it;

  if (!hl_sameproccxt) return;

  for (it = hashtbl_iterator(hl_sameproccxt);
       !hashtbliter_isend(&it);
       hashtbliter_next(&it)) {
    SameProcCxt *spc = (SameProcCxt *)(hashtbliter_entry(&it)->data);

    if (!spc) {
      continue;
    }
    else if (spc->proccxts) {
      Vector *tree;
      unsigned int i;

      for (i = 0; i < spc->length; i++) {
        ProcCxt *pc = LMN_SPC_PC(spc, i);
        if (pc) LMN_FREE(pc);
      }

      tree = LMN_SPC_TREE(spc);
      if (tree) vec_free(tree);

      LMN_FREE(spc->proccxts);
    }

    LMN_FREE(spc);
  }

  hashtbl_free(hl_sameproccxt);
}



void lmn_sameproccxt_clear(LmnReactCxt *rc)
{
  sameproccxt_destroy(RC_HLINK_SPC(rc));
  RC_SET_HLINK_SPC(rc, NULL);
}


ProcCxt *lmn_sameproccxt_pc_make(int atomi, int arg, ProcCxt *original)
{
  ProcCxt *pc;

  pc = LMN_MALLOC(ProcCxt);
  pc->atomi    = atomi;
  pc->arg      = arg;
  pc->start    = NULL;
  pc->original = original;

  return pc;
}

SameProcCxt *lmn_sameproccxt_spc_make(int atomi, int length)
{
  SameProcCxt *spc;
  int i;
  spc = LMN_MALLOC(SameProcCxt);
  spc->atomi      = atomi;
  spc->length     = length;
  spc->tree       = NULL;
  spc->start_attr = 0;
  spc->proccxts   = LMN_NALLOC(void *, length);
  for (i = 0; i < length; i++)
    spc->proccxts[i] = NULL;

  return spc;
}

BOOL lmn_sameproccxt_from_clone(SameProcCxt *spc, int n)
{
  ProcCxt *pc;
  int i;

  for (i = 0; i < n; i++) {
    pc = LMN_SPC_PC(spc, i);
    if (pc
        && !LMN_PC_IS_ORI(pc)
        && (LMN_PC_ATOMI(LMN_PC_ORI(pc)) != LMN_PC_ATOMI(pc))) { /* clone proccxtを持つ */
      return TRUE; /* hyperlinkからfindatomを行なう */
    }
  }

  return FALSE;
}

/* 探索の始点となる引数を決定する
 * 候補が複数ある場合は、もっとも選択肢の少ない(element_numが小さい)hyperlinkがマッチする引数を探索の始点とする
 */
HyperLink *lmn_sameproccxt_start(SameProcCxt *spc, int atom_arity)
{
  HyperLink *start_hl;
  int i, element_num, start_arity;

  start_hl = NULL;
  element_num = -1;
  start_arity = 0;

  /* バックトラックしてきた場合はspc->treeの中身は初期化されていないため、ここで初期化 */
  if (LMN_SPC_TREE(spc)) {
    vec_free(LMN_SPC_TREE(spc));
    LMN_SPC_TREE(spc) = NULL;
  }

  for (i = 0; i < atom_arity; i++) {
    ProcCxt *pc = LMN_SPC_PC(spc, i);
    if (pc && !LMN_PC_IS_ORI(pc)) {
      HyperLink *hl;
      int tmp_num;

      hl = LMN_PC_START(LMN_PC_ORI(pc));
      LMN_PC_START(pc) = hl;
//      /* オリジナル側で探索始点のハイパーリンクが指定されていない
//       * または探索始点のハイパーリンクの要素数が0（どちらも起こり得ないはず）*/
//      if (!hl) return FALSE;
//      if (!(element_num = lmn_hyperlink_element_num(hl))) return FALSE;

      tmp_num = lmn_hyperlink_element_num(hl);
      if (element_num < 0 || element_num > tmp_num) {
        element_num = tmp_num;
        start_hl = hl;
        start_arity = i;
      }
    }
  }

  LMN_SPC_TREE(spc) = vec_make(element_num <= 0 ? 1 : element_num);
  LMN_SPC_SATTR(spc) = start_arity;
  return start_hl;
}

/* オリジナル側のatom が持つ全ての引数に対して以下の処理を行なう
 * a. 通常の引数
 *   何もしない
 * b. 同名型付きプロセス文脈を持つ引数
 *   clone側での探索の始点となるhyperlinkをspcに保持させる
 */
BOOL lmn_sameproccxt_all_pc_check_original(SameProcCxt *spc, LmnSAtom atom, int atom_arity)
{
  int i;
  BOOL all_pc_check;

  all_pc_check = TRUE;
  for (i = 0; i < atom_arity; i++) {
    ProcCxt *pc;
    LmnSAtom linked_atom;
    LmnLinkAttr linked_attr;

    pc = LMN_SPC_PC(spc, i);
    if (!pc) continue; /* atom(spc)の第i引数が同名プロセス文脈 */

    linked_atom = LMN_SATOM(LMN_SATOM_GET_LINK(atom, i));
    linked_attr = LMN_SATOM_GET_ATTR(atom, i);

    if (!LMN_ATTR_IS_HL(linked_attr)) {
      all_pc_check = FALSE;
    }
    else if (!LMN_PC_IS_ORI(pc) &&
             !lmn_hyperlink_eq_hl(LMN_PC_START(LMN_PC_ORI(pc)),
                                  lmn_hyperlink_at_to_hl(linked_atom))) {
      /* atomの第i引数がハイパーリンク */
      all_pc_check = FALSE;
    }
    else {
      HyperLink *hl = lmn_hyperlink_at_to_hl(linked_atom);
      if (lmn_hyperlink_element_num(hl) <= 1) {
        all_pc_check = FALSE;
      }
      LMN_PC_START(pc) = lmn_hyperlink_at_to_hl(linked_atom);
    }
    if (!all_pc_check) break;
  }


  return all_pc_check;
}


/* clone側のatomが持つ全ての引数に対して以下の処理を行なう
 * a. 通常の引数
 *   何もしない
 * b. 同名型付きプロセス文脈を持つ引数
 *   b-1. original側の引数である場合
 *     clone側での探索の始点となるhyperlinkをspcに保持させる
 *   b-2. clone側の引数である場合
 *     original側で探索の始点として指定されていたhyperlinkに対応していることを確認する
 */
BOOL lmn_sameproccxt_all_pc_check_clone(SameProcCxt *spc, LmnSAtom atom, int atom_arity)
{
  int i;
  BOOL all_pc_check;

  all_pc_check = TRUE;
  for (i = 0; i < atom_arity; i++) {
    ProcCxt *pc;
    LmnSAtom linked_atom;
    LmnLinkAttr linked_attr;

    pc = LMN_SPC_PC(spc, i);
    if (!pc) continue;

    linked_atom = LMN_SATOM(LMN_SATOM_GET_LINK(atom, i));
    linked_attr = LMN_SATOM_GET_ATTR(atom, i);

    if (!LMN_ATTR_IS_HL(linked_attr)) { /* atomの第i引数がハイパーリンク */
      all_pc_check = FALSE;
    }
    else {
      HyperLink *linked_hl = lmn_hyperlink_at_to_hl(linked_atom);

      if (LMN_PC_IS_ORI(pc)) { /* 第i引数がオリジナルであれば、クローン側での探索の始点を保持 */
        LMN_PC_START(pc) = linked_hl;
      } else if (!lmn_hyperlink_eq_hl(linked_hl, LMN_PC_START(pc))) { /* 第i引数がクローンであれば、それの成否を確かめる */
        all_pc_check = FALSE;
      }
    }

    if (!all_pc_check) break;
  }

  return all_pc_check;
}


/* rootの子を全てtreeに格納する(withoutは除く) */
void hyperlink_get_children_without(Vector *tree, HyperLink *root, HyperLink *without)
{
  HashSetIterator it;
  for (it = hashset_iterator(root->children); !hashsetiter_isend(&it); hashsetiter_next(&it)) {
//    printf("%p\n", (void *)((HyperLink *)hashsetiter_entry(&it))->atom);
//    pro1++;
    HyperLink *hl = (HyperLink *)hashsetiter_entry(&it);
    if (hl->children && hl->rank > 0) { /* hlが子を持つならば */
      hyperlink_get_children_without(tree, hl, without);
    }

    if (hl != without) {
      vec_push(tree, (LmnWord)hl);
    }
  }
}


/* start_hlと同じ集合に属するhyperlinkを全てVectorに格納して返す.
 * fidnproccxtで使用する関係上,
 * start_hlは探索対象外のHyperLinkであるため, treeの最後に追加する  */
void lmn_hyperlink_get_elements(Vector *tree, HyperLink *start_hl)
{
  HyperLink *root = lmn_hyperlink_get_root(start_hl);
  if (root->rank > 0)   hyperlink_get_children_without(tree, root, start_hl);
  if (root != start_hl) vec_push(tree, (LmnWord)root);
  vec_push(tree, (LmnWord)start_hl);
}


/* --------------------------------------------------------------- *
 *  ハイパーリンクハッシュ値計算                                     *
 *  '!'アトムに接続されたアトムを深さDまでたどり計算する               *
 * -------------------------------------------------------------- */

/* ハイパーリンクhlのハッシュ値を計算する */
unsigned long lmn_hlhash(HyperLink *hl) {

  unsigned long sum = 0;
  HashSetIterator hsit;
  HyperLink *ch_hl;
  HashSet *children;
  HyperLink *hl_root = lmn_hyperlink_get_root(hl);
  LmnAtom at = (LmnAtom) lmn_hyperlink_hl_to_at(hl_root);
  if (!LMN_ATTR_IS_DATA(LMN_SATOM_GET_ATTR(at, 0)) && LMN_SATOM_GET_LINK(at, 0)) {
    sum +=  lmn_hlhash_sub(LMN_SATOM_GET_LINK(at, 0));
  }
  if((children = hl_root->children)){
    for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
      if ((HashKeyType)(ch_hl = (HyperLink *)hashsetiter_entry(&hsit)) < DELETED_KEY) {
        at = (LmnAtom) lmn_hyperlink_hl_to_at(ch_hl);
        if (!LMN_ATTR_IS_DATA(LMN_SATOM_GET_ATTR(at, 0)) && LMN_SATOM_GET_LINK(at, 0)) {
          sum +=  lmn_hlhash_sub(LMN_SATOM_GET_LINK(at, 0));
        }
      }
    }
  }
  return sum;
}

unsigned long lmn_hlhash_sub(LmnWord atom) {

  unsigned long sum = 0;

  if (LMN_IS_SYMBOL_FUNCTOR(LMN_SATOM_GET_FUNCTOR(atom))) {
    const int arity = LMN_SATOM_GET_ARITY(atom);
    int i_arg;
    LmnLinkAttr attr;
    for (i_arg = 0; i_arg < arity; i_arg++) {
      
      attr = LMN_SATOM_GET_ATTR(atom,i_arg);
      if (!LMN_ATTR_IS_DATA(attr)) {
        LmnAtom at = LMN_SATOM_GET_LINK(atom, i_arg);
        if (at) {
          sum += LMN_SATOM_GET_FUNCTOR(at);
        }
      }else{
        switch (attr) {
        case  LMN_INT_ATTR:
          sum += LMN_SATOM_GET_LINK(atom,i_arg);
          break;
        case  LMN_DBL_ATTR:
          sum += (int) (*(double*)LMN_SATOM_GET_LINK(atom,i_arg));
          break;
        case  LMN_HL_ATTR:
          sum += lmn_hyperlink_element_num(lmn_hyperlink_at_to_hl((LmnSAtom) LMN_SATOM_GET_LINK(atom, i_arg)));	   
          break;
        default:
          break;
        }
      }
      
    }
  }

  return sum;
}

/* ハイパーリンクhlのハッシュ値を計算する(深さ指定版) */
unsigned long lmn_hlhash_depth(HyperLink *hl, int depth) {

  unsigned long sum = 0;
  HashSetIterator hsit;
  HyperLink *ch_hl;
  HashSet *children;
  LmnLinkAttr attr;
  HyperLink *hl_root = lmn_hyperlink_get_root(hl);
  LmnAtom at = (LmnAtom) lmn_hyperlink_hl_to_at(hl_root);
  attr = LMN_SATOM_GET_ATTR(at, 0);
  if (!LMN_ATTR_IS_DATA(attr) && LMN_SATOM_GET_LINK(at, 0)) {
    lmn_hlhash_depth_sub(LMN_SATOM_GET_LINK(at, 0), attr, -1 , &sum, depth);
  }
  if((children = hl_root->children)){
    for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
      if ((HashKeyType)(ch_hl = (HyperLink *)hashsetiter_entry(&hsit)) < DELETED_KEY) {
        at = (LmnAtom) lmn_hyperlink_hl_to_at(ch_hl);
        attr = LMN_SATOM_GET_ATTR(at, 0);
        if (!LMN_ATTR_IS_DATA(attr) && LMN_SATOM_GET_LINK(at, 0)) {
          lmn_hlhash_depth_sub(LMN_SATOM_GET_LINK(at, 0), attr, -1 , &sum, depth);
        }
      }
    }
  }
  return sum;
}

void lmn_hlhash_depth_sub(LmnAtom atom, LmnLinkAttr attr, int i_parent, unsigned long *sum, int depth)
{
  unsigned long unit = 0;
  if (!LMN_ATTR_IS_DATA(attr)) { // symbol
    LmnFunctor f = LMN_SATOM_GET_FUNCTOR(atom);
    unit = (unsigned long) f;

    if (depth > 1) {
      if (LMN_IS_SYMBOL_FUNCTOR(f)) {
        const int arity = LMN_SATOM_GET_ARITY(atom);
        int i_arg;

        for (i_arg = 0; i_arg < arity; i_arg++) {
          if (i_arg == i_parent) continue;
          
          LmnLinkAttr to_attr = LMN_SATOM_GET_ATTR(atom, i_arg);
          LmnAtom to_atom = LMN_SATOM_GET_LINK(atom, i_arg);
          lmn_hlhash_depth_sub(to_atom, to_attr, LMN_ATTR_GET_VALUE(to_attr), sum, depth-1);
        }
      }
    }

  }else{ // data    
    switch (attr) {
    case  LMN_INT_ATTR: // int
      unit = ((unsigned long) atom) + 1;
      break;
    case  LMN_DBL_ATTR: // double
      unit += (unsigned long) (*(double*) atom);
      /* unit = (unsigned long) lmn_byte_hash((unsigned char *)atom, sizeof(double) / sizeof(unsigned char)); */
      break;
    case  LMN_HL_ATTR: // hyperlink
      unit = lmn_hyperlink_element_num(lmn_hyperlink_at_to_hl((LmnSAtom) atom));
      break;
    default:
      break;
    }    
  }
  (*sum) += unit;
}


/* --------------------------------------------------------------- *
 *  ハイパーリンクから膜への変換                                     *
 * -------------------------------------------------------------- */

/* ハイパーリンクを膜に変換する関数 (ハイパーリンク->膜) */
void lmn_convert_hl_to_mem_root(LmnMembrane *gr)
{
  // 引数のグローバルルート膜を始点にして、全膜内のハイパーリンクアトム探索
  // ハイパーリンクに出会ったときに変換
  AtomListEntry *ent;
  LmnSAtom atom;
  do {
    ent = lmn_mem_get_atomlist(gr, LMN_HL_FUNC);
    if(!ent || ( atomlist_head(ent) == lmn_atomlist_end(ent) )) break;

    EACH_ATOM(atom, ent, ({
          lmn_convert_hl_to_mem_sub(gr, gr, lmn_hyperlink_at_to_hl(atom));
          break;
        }));
  } while (ent);

  // 子の膜について再帰する
  LmnMembrane *m;
  for(m = gr->child_head; m; m = m->next){
    if(LMN_MEM_ATTR(m) != LMN_HYPERLINK_MEM){    // ハイパーリンク膜以外
      lmn_convert_hl_to_mem(gr, m);
    }
  }
}

/* ハイパーリンクを膜に変換する関数 */
void lmn_convert_hl_to_mem(LmnMembrane *gr, LmnMembrane *mem)
{
  AtomListEntry *ent;
  LmnSAtom atom;
  do {
    ent = lmn_mem_get_atomlist(mem, LMN_HL_FUNC);
    if(!ent || ( atomlist_head(ent) == lmn_atomlist_end(ent) )) break;
    EACH_ATOM(atom, ent, ({
          lmn_convert_hl_to_mem_sub(gr, mem, lmn_hyperlink_at_to_hl(atom));
          break;
        }));
  } while (ent);

  LmnMembrane *m;
  for(m = mem->child_head; m; m = m->next){
    if(LMN_MEM_ATTR(m) != LMN_HYPERLINK_MEM){    // ハイパーリンク膜以外
      lmn_convert_hl_to_mem(gr, m);
    }
  }
}


/* ハイパーリンクを膜に変換する関数。ひとつのハイパーリンクについてその集合を変換する */
void lmn_convert_hl_to_mem_sub(LmnMembrane *gr, LmnMembrane *mem, HyperLink *hl)
{
  // 膜の作成・初期化,ハイパーリンクという属性を表すようなものを付加
  // parentにグローバルルート膜
  LmnMembrane *hlmem = lmn_mem_make();
  lmn_mem_set_attr(hlmem, LMN_HYPERLINK_MEM);
  lmn_mem_add_child_mem(gr, hlmem);
  // lmn_mem_set_active(hlmem, TRUE);

  // ハイパーリンクの集合のIDを取得し、膜のatom_data_numに退避する
  unsigned long hid = LMN_HL_ID(lmn_hyperlink_get_root(hl));
  lmn_mem_data_atom_set(hlmem, hid);

  // ハイパーリンクを全部取得
  HyperLink *tmp_hl = lmn_hyperlink_get_root(hl);

  int len = lmn_hyperlink_element_num(hl);
  Vector *hls = vec_make(len);
  vec_push(hls, (LmnWord) tmp_hl);

  HashSet *children = tmp_hl->children;
  if(children){
    HashSetIterator hsit;
    for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
      if ((tmp_hl = (HyperLink *) hashsetiter_entry(&hsit))) {
        if ((HashKeyType) tmp_hl < DELETED_KEY) {
          vec_push(hls, (LmnWord) tmp_hl);
        }
      }
    }
  }

  // ハイパーリンクをルートからたどる
  int i, linknum;
  LmnSAtom hlatom, atom;
  LmnSAtom newatom;
  for (i = 0; i < vec_num(hls); i++) {
    tmp_hl = (HyperLink *) vec_get(hls, i);

    // ハイパーリンクに対応するアトムを生成して、もともと接続されていたアトムと接続し膜へ追加
    hlatom = lmn_hyperlink_hl_to_at(tmp_hl);
    atom = LMN_SATOM(LMN_SATOM_GET_LINK(hlatom, 0));
    newatom = lmn_new_atom(LMN_HLMEM_ATOM_FUNCTOR); // '+'アトム。ハイパーリンクアトムに対応。
    lmn_mem_push_atom(hlmem, LMN_ATOM(newatom), 0);

    linknum = lmn_get_atom_link_num(hlatom, atom);
    if(linknum != -1) {
      lmn_link_at_to_at(newatom, 0, hlmem, atom, linknum, mem);
    }
  }

  // ハイパーリンク削除
  lmn_hyperlink_delete_all(hl);

  vec_free(hls);
}

/* ハイパーリンクから変換した膜をハイパーリンクにデコードする関数(膜->ハイパーリンク) */
void lmn_convert_mem_to_hl_root(LmnMembrane *gr)
{
  LmnMembrane *m;
  for(m = gr->child_head; m; m = m->next){
    if(LMN_MEM_ATTR(m) == LMN_HYPERLINK_MEM){
      lmn_convert_mem_to_hl(m);
    }
  }
}

void lmn_convert_mem_to_hl(LmnMembrane *hlmem)
{
  AtomListEntry *ent = lmn_mem_get_atomlist(hlmem, LMN_HLMEM_ATOM_FUNCTOR);
  LmnSAtom hlmem_atom;
  LmnSAtom tmp_atom, tmp_old_atom;
  LmnSAtom tmp_atom0, tmp_atom1;
  LmnMembrane *mem;

  LmnSAtom tmp_hlatom;
  LmnSAtom hl_atom = lmn_hyperlink_new();

  if (ent) {
    EACH_ATOM(hlmem_atom, ent, ({
          // リンクをたどり接続されているアトムを取得する
          tmp_old_atom = hlmem_atom;
          tmp_atom = LMN_SATOM(LMN_SATOM_GET_LINK(hlmem_atom, 0));
          while( LMN_SATOM_IS_PROXY(tmp_atom) ){
            mem = (LmnMembrane *) LMN_SATOM_GET_LINK(tmp_atom, 2);
            tmp_atom0 = LMN_SATOM(LMN_SATOM_GET_LINK(tmp_atom, 0));
            tmp_atom1 = LMN_SATOM(LMN_SATOM_GET_LINK(tmp_atom, 1));
            if(tmp_atom0 != tmp_old_atom){
              tmp_old_atom = tmp_atom;
              tmp_atom = tmp_atom0;
            }else{
              tmp_old_atom = tmp_atom;
              tmp_atom = tmp_atom1;
            }
          }
          int linknum = lmn_get_atom_link_num(tmp_old_atom, tmp_atom);

          // unlink
          lmn_unlink_at_to_at(hlmem_atom, 0, hlmem, tmp_atom, linknum,mem);
          //remove hlmem_atom
          mem_remove_symbol_atom(hlmem, LMN_SATOM(hlmem_atom));
          lmn_delete_atom(LMN_SATOM(hlmem_atom));
          // make hyperlink atom
          tmp_hlatom = LMN_SATOM(lmn_copy_atom(LMN_ATOM(hl_atom), LMN_HL_ATTR));
          lmn_mem_push_atom(mem, LMN_ATOM(tmp_hlatom), LMN_HL_ATTR);
          lmn_mem_newlink(mem, LMN_ATOM(tmp_atom), 0, linknum, LMN_ATOM(tmp_hlatom), LMN_HL_ATTR, 0);
        }));

    // hl_atomを開放
    lmn_hyperlink_delete(LMN_SATOM(hl_atom));
    lmn_delete_atom(LMN_SATOM(hl_atom));

    // remove mem
    lmn_mem_remove_mem(lmn_mem_parent(hlmem), hlmem);
    lmn_mem_free(hlmem);

    // hyperlink idをルートに設定する
    unsigned long hid = lmn_mem_data_atom_num(hlmem);
    LMN_SET_HL_ID(LMN_HL_ATOM_ROOT_HL(tmp_hlatom), hid);
  }

}

/* 一つのハイパーリンクを削除する関数。ハイパーリンクアトムを膜から取り除き、ハイパーリンク構造体も削除する。 */
void lmn_hyperlink_delete_from_mem(HyperLink *hl)
{
  if(hl){
    LmnSAtom hlatom = lmn_hyperlink_hl_to_at(hl);
    mem_remove_symbol_atom(LMN_HL_MEM(hl), LMN_SATOM(hlatom));
    /* lmn_hyperlink_delete(hlatom); */
    lmn_hyperlink_delete_old(hlatom);
    lmn_delete_atom(LMN_SATOM(hlatom));
  }
}

/* 一つのハイパーリンクの集合を削除する関数 */
void lmn_hyperlink_delete_all(HyperLink *hl)
{
  if(hl){
    // ハイパーリンクを全部取得
    HyperLink *tmp_hl = lmn_hyperlink_get_root(hl);
    int len = lmn_hyperlink_element_num(hl);
    Vector *hls = vec_make(len);
    vec_push(hls, (LmnWord) tmp_hl);

    HashSet *children = tmp_hl->children;
    if(children){
      HashSetIterator hsit;
      for (hsit = hashset_iterator(children); !hashsetiter_isend(&hsit); hashsetiter_next(&hsit)) {
        if ((tmp_hl = (HyperLink *) hashsetiter_entry(&hsit))) {
          if ((HashKeyType) tmp_hl < DELETED_KEY) {
            vec_push(hls, (LmnWord) tmp_hl);
          }
        }
      }
    }

    // ハイパーリンクを削除
    int i;
    for (i = 0; i < vec_num(hls); i++) {
      tmp_hl = (HyperLink *) vec_get(hls, i);
      lmn_hyperlink_delete_from_mem(tmp_hl);
    }

    vec_free(hls);
  }
}

/* 膜の深さを取得する関数 */
/* グローバルルート膜が深さ1 */
unsigned int lmn_get_mem_depth(LmnMembrane *mem)
{
  unsigned int depth = 0;
  LmnMembrane *tmpcmem, *tmppmem = mem;
  do{
    depth++;
    tmpcmem = tmppmem;
    tmppmem = lmn_mem_parent(tmpcmem);
  }while(tmppmem != tmpcmem && tmppmem != NULL);
  return depth;
}

/* 接続元のアトムが接続先のアトムの何番目のリンクに接続されているか取得する関数 */
int lmn_get_atom_link_num(LmnSAtom srcAt, LmnSAtom destAt)
{
  int i, srcId = LMN_SATOM_ID(srcAt);
  LmnArity arity = LMN_SATOM_GET_ARITY(destAt);
  for(i = 0; i < arity; i++){
    if(!LMN_ATTR_IS_DATA_WITHOUT_EX(LMN_SATOM_GET_ATTR(destAt,i))
       && LMN_SATOM_ID(LMN_SATOM_GET_LINK(destAt,i)) == srcId)
      return i;
  }
  LMN_ASSERT(0);
  return -1;
}

/* 2つのアトム間をリンクでつなぐ関数。途中に膜がある場合にはプロキシを設置していく。 */
void lmn_link_at_to_at(LmnAtom *at1, int pos1, LmnMembrane *mem1, LmnAtom *at2, int pos2, LmnMembrane *mem2)
{
  //膜の深さを調べる
  int depth1 = lmn_get_mem_depth(mem1);
  int depth2 = lmn_get_mem_depth(mem2);

  // 親膜をたどっていく。同時にin, outプロキシ作成する。
  // まずは深さの深い方の膜の親膜をたどっていき、2つの膜の深さを合わせる。そのあとは同じ膜に行きつくまで、同時に親膜をたどっていく。
  LmnMembrane *tmpcmem1, *tmppmem1;
  LmnMembrane *tmpcmem2, *tmppmem2;
  LmnSAtom in1, out1, old_out1 = NULL;
  LmnSAtom in2, out2, old_out2 = NULL;
  int tmpdepth1 = depth1;
  int tmpdepth2 = depth2;
  tmppmem1 = mem1; tmppmem2 = mem2;
  while(1){
    if(depth1 == depth2 && (tmppmem1 == tmppmem2 || depth1 == 1 || depth2 == 1))
      break;

    // mem1がmem2よりも階層が深い場合。または深さが等しくなったとき。
    if(depth1 >= depth2){
      tmpcmem1 = tmppmem1;
      tmppmem1 = lmn_mem_parent(tmpcmem1);
      in1  = lmn_mem_newatom(tmpcmem1, LMN_IN_PROXY_FUNCTOR);
      out1 = lmn_mem_newatom(tmppmem1, LMN_OUT_PROXY_FUNCTOR);
      lmn_newlink_in_symbols(in1, 0, out1, 0);
      if(old_out1 != NULL){
        lmn_newlink_in_symbols(old_out1, 1, in1, 1);
      }else{
        lmn_newlink_in_symbols(in1, 1, at1, pos1);
      }
      old_out1 = out1;
      tmpdepth1--;
    }

    // mem2がmem1よりも階層が深い場合。または深さが等しくなったとき。
    if(depth1 <= depth2){
      tmpcmem2 = tmppmem2;
      tmppmem2 = lmn_mem_parent(tmpcmem2);
      in2  = lmn_mem_newatom(tmpcmem2, LMN_IN_PROXY_FUNCTOR);
      out2 = lmn_mem_newatom(tmppmem2, LMN_OUT_PROXY_FUNCTOR);
      lmn_newlink_in_symbols(in2, 0, out2, 0);
      if(old_out2 != NULL){
        lmn_newlink_in_symbols(old_out2, 1, in2, 1);
      }else{
        lmn_newlink_in_symbols(in2, 1, at2, pos2);
      }
      old_out2 = out2;
      tmpdepth2--;
    }

    depth1 = tmpdepth1;
    depth2 = tmpdepth2;

  }

  // 最後に同階層の$out-$outリンクをつなぐ。$outがないときはもとのアトムと接続する
  if(old_out1 != NULL && old_out2 != NULL){
    lmn_newlink_in_symbols(old_out1, 1, old_out2, 1);
  }else if(old_out1 != NULL && old_out2 == NULL){
    lmn_newlink_in_symbols(old_out1, 1, at2, pos2);
  }else if(old_out1 == NULL && old_out2 != NULL){
    lmn_newlink_in_symbols(at1, pos1, old_out2, 1);
  }else{
    lmn_newlink_in_symbols(at1, pos1, at2, pos2);
  }

}

/* 2つのアトム間をアンリンクする関数。2つのアトム間で途中に膜を通過しプロキシがあるときにはそのプロキシアトムを削除する。*/
void lmn_unlink_at_to_at(LmnAtom *at1, int pos1, LmnMembrane *mem1, LmnAtom *at2, int pos2, LmnMembrane *mem2)
{
  LmnSAtom tmp_atom, tmp_old_atom;
  LmnSAtom tmp_atom0, tmp_atom1;
  LmnMembrane *mem, *old_mem;

  tmp_old_atom = at1;
  tmp_atom = LMN_SATOM(LMN_SATOM_GET_LINK(at1, pos1));
  while( LMN_SATOM_IS_PROXY(tmp_atom) ){
    mem = (LmnMembrane *) LMN_SATOM_GET_LINK(tmp_atom, 2);
    tmp_atom0 = LMN_SATOM(LMN_SATOM_GET_LINK(tmp_atom, 0));
    tmp_atom1 = LMN_SATOM(LMN_SATOM_GET_LINK(tmp_atom, 1));
    if(tmp_atom0 != tmp_old_atom){
      if(LMN_SATOM_IS_PROXY(tmp_old_atom)){
        // プロキシアトム削除
        mem_remove_symbol_atom(old_mem, LMN_SATOM(tmp_old_atom));
        lmn_delete_atom(LMN_SATOM(tmp_old_atom));
      }
      tmp_old_atom = tmp_atom;
      old_mem = mem;
      tmp_atom = tmp_atom0;
    }else{
      if(LMN_SATOM_IS_PROXY(tmp_old_atom)){
        // プロキシアトム削除
        mem_remove_symbol_atom(old_mem, LMN_SATOM(tmp_old_atom));
        lmn_delete_atom(LMN_SATOM(tmp_old_atom));
      }
      tmp_old_atom = tmp_atom;
      old_mem = mem;
      tmp_atom = tmp_atom1;
    }
  }
  if(LMN_SATOM_IS_PROXY(tmp_old_atom)){
    // プロキシアトム削除
    mem_remove_symbol_atom(old_mem, LMN_SATOM(tmp_old_atom));
    lmn_delete_atom(LMN_SATOM(tmp_old_atom));
  }
}

/* 膜内のアトムで最大のIDを返す */
unsigned long get_max_id(LmnMembrane *mem)
{
  AtomListEntry *ent;
  unsigned long maxid = 0;
  if (!mem) return 0;

  EACH_ATOMLIST(mem, ent, ({
    LmnSAtom atom;
    EACH_ATOM(atom, ent, ({
	  if(LMN_SATOM_ID(atom) > maxid) maxid = LMN_SATOM_ID(atom);
    }));
  }));

  LmnMembrane *m;
  unsigned long cmaxid;
  for(m = mem->child_head; m; m = m->next){
    cmaxid = get_max_id(m);
    if(cmaxid > maxid) maxid = cmaxid;
  }
  return maxid;
}


/* リンクテスト for debug @onuma */
/* 以下LMNtalプログラムの構造に適用 */
/* a(X). b(X). { { c(Y). d(Y). } }. */
void lmn_link_test(LmnMembrane *gr)
{
  printf("before-----------------\n");
  lmn_dump_mem_dev(gr);
  printf("before end-----------------\n\n");

  AtomListEntry *ent;
  LmnSAtom at1 = NULL;
  LmnSAtom at2 = NULL;
  LmnSAtom at3 = NULL;
  LmnSAtom at4 = NULL;
  LmnSAtom at5 = NULL;
  LmnSAtom at6 = NULL;

  LmnSAtom atom;
  EACH_ATOMLIST(gr, ent, ({
        EACH_ATOM(atom, ent, ({
              if(at1 == NULL){ at1 = LMN_SATOM(atom); }else if(at2 == NULL){ at2 = LMN_SATOM(atom); }
              else if(at3 == NULL){ at3 = LMN_SATOM(atom); }else if(at4 == NULL){ at4 = LMN_SATOM(atom); }
              else if(at5 == NULL){ at5 = LMN_SATOM(atom); }else if(at6 == NULL){ at6 = LMN_SATOM(atom); }
            }));
      }));

  LmnMembrane *m = gr->child_head->child_head;
  EACH_ATOMLIST(m, ent, ({
        EACH_ATOM(atom, ent, ({
              if(at1 == NULL){ at1 = LMN_SATOM(atom); }else if(at2 == NULL){ at2 = LMN_SATOM(atom); }
              else if(at3 == NULL){ at3 = LMN_SATOM(atom); }else if(at4 == NULL){ at4 = LMN_SATOM(atom); }
              else if(at5 == NULL){ at5 = LMN_SATOM(atom); }else if(at6 == NULL){ at6 = LMN_SATOM(atom); }
            }));
      }));

  {
    /* dump_atom_dev(at1); */
    /* dump_atom_dev(at2); */
    /* dump_atom_dev(at3); */
    /* dump_atom_dev(at4); */

    // link (a-b, c-d -> a-c, b-d)
    lmn_link_at_to_at(at1, 0, gr, at3, 0, m);
    lmn_link_at_to_at(at2, 0, gr, at4, 0, m);

    // unlink (a-c, b-d -> a, c, b, d)
    lmn_unlink_at_to_at(at1, 0, gr, at3, 0, m);
    lmn_unlink_at_to_at(at2, 0, gr, at4, 0, m);

    // link (a, b, c, d -> a-b, c-d)
    lmn_link_at_to_at(at1, 0, gr, at2, 0, gr);
    lmn_link_at_to_at(at3, 0, m, at4, 0, m);
  }

  printf("\nafter-----------------\n");
  lmn_dump_mem_dev(gr);
  printf("after end-----------------\n");

}
