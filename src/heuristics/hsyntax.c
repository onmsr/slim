/* hsyntax.c */


#include "hsyntax.h"
#include "symbol.h"

/* HInstruction */

HInstruction hinstr_make(enum LmnHInstruction id, HArgList args, HDest dest)
{
  HInstruction i = LMN_MALLOC(struct HInstruction);
  LMN_SET_HINSTR_ID(i, id);
  LMN_SET_HINSTR_ARGS(i, args);
  LMN_SET_HINSTR_DEST(i, dest);
  return i;
}

void hinstr_free(HInstruction inst)
{
  /* harg_list_free(hinst_get_args(inst)); */
  LMN_FREE(inst);
}


/* HArgument */


HArgument harg_make(HArg val)
{
  HArgument a = LMN_MALLOC(struct HArgument);
  LMN_SET_HARG(a, val);
  return a;
}

void harg_free(HArgument a)
{
  LMN_FREE(LMN_GET_HARG(a));
  LMN_FREE(a);
}


/* HArgList */


HArgList harg_list_make()
{
  HArgList l = vec_make(4);
  return l;
}

void harg_list_push(HArgList args, HArgument arg)
{
  vec_push(args, (LmnWord) arg);
}


/* HInstrList */

HInstrList hinstr_list_make()
{
  HInstrList l = vec_make(128);
  return l;
}

void hinstr_list_push(HInstrList instrs, HInstruction instr)
{
  vec_push(instrs, (LmnWord) instr);
}


/* HIL */

HIL hil_make(HInstrList instrs, HLabelList labels)
{
  HIL hil = LMN_MALLOC(struct HIL);
  hil->instrs = instrs;
  hil->labels = labels;
  return hil;
}

HIL hil_local_make(HIL ghil)
{
  HIL hil = LMN_MALLOC(struct HIL);
  hil->instrs = ghil->instrs;
  hil->labels = ghil->labels;
  hil->reg.size = 0;
  return hil;
}

void hil_init_symbol(HIL hil)
{
  unsigned int j, m = vec_num(hil->instrs);
  for (j = 0; j < m; j++) {
    HInstruction tmpinstr = (HInstruction) vec_get(hil->instrs, j);
    HArgList tmphargs = LMN_GET_HINSTR_ARGS(tmpinstr);
    /* unsigned int tmpdest = get_hdest(tmpinstr); */
    if (tmpinstr->id == HINSTR_GET) {
      HArgument a1 = (HArgument) vec_get(tmphargs, 0);
      unsigned int id = find_functor_id_by_name((char *) LMN_GET_HARG(a1));
      LMN_SET_HARG(a1, id);
    }
  }
}


/* HRegister */

void hil_init_hregs(HIL hil, unsigned int size)
{
  size += 1; // for register zero
  LMN_SET_HREGISTER_SIZE(hil, size);
  LMN_SET_HREGS(hil, vec_make(round2up(size)));
  unsigned int i;
  for (i = 0; i < size; i++) {
    vec_push(LMN_GET_HREGS(hil), 0);
  }
}


/* HLabel */
HLabel hlabel_make(HLabelStr label, unsigned long pos)
{
  HLabel l = LMN_MALLOC(struct HLabel);
  l->label = label;
  l->pos = pos;
  return l;
}

void hlabel_free(HLabel l)
{
  LMN_FREE(l->label);
  LMN_FREE(l);
}

/* HLabelList */
HLabelList hlabel_list_make()
{
  HLabelList l = vec_make(32);
  return l;
}

void hlabel_list_push(HLabelList labels, HLabel label)
{
  vec_push(labels, (LmnWord) label);
}


/* dumper */

void dump_hil(HIL hil)
{
  dump_hinstr_list(hil->instrs);
}

void dump_hinstr_list(HInstrList l)
{
  HInstruction instr;
  unsigned int i, n = vec_num(l);
  for (i = 0; i < n; i++) {
    instr = (HInstruction) vec_get(l, i);
    dump_hinstr(instr);
  }
}

void dump_hinstr(HInstruction inst)
{
  printf("%s ", get_hinstr_by_id(inst->id));
  HArgument arg;
  unsigned int i, n = vec_num(inst->args);
  printf("[ ");
  for (i = 0; i < n; i++) {
    if (i != 0) printf(" , ");
    arg = (HArgument) vec_get(inst->args, i);
    if (i == 0 && inst->id == HINSTR_GET) { // シンボルテーブルから取得
      printf("%s", get_functor_name(LMN_GET_HARG(arg)));
    } else {
      printf("%s", (char *) LMN_GET_HARG(arg));
    }
  }
  printf(" ]");
  printf(" %s\n", (char *) inst->dest);
}

char *get_hinstr_by_id(enum LmnHInstruction id)
{
  char *LmnHInstructionStr[] = {
    "INIT",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "STORE",
    "GET",
    "ABS",
    "EQ",
    "NOT",
    "AND",
    "OR",
    "BR",
    "BRT",
    "DUMMY"
  };
  return LmnHInstructionStr[id];
}

unsigned int get_hinstr_id(char *s)
{
  char *LmnHInstructionStr[] = {
    "init",
    "add",
    "sub",
    "mul",
    "div",
    "store",
    "get",
    "abs",
    "eq",
    "not",
    "and",
    "or",
    "Br",
    "BrT",
    "dummy"
  };
  unsigned int i, n = sizeof(LmnHInstructionStr) / sizeof(LmnHInstructionStr[0]);
  for (i = 0; i < n; i++) {
    if (strcmp(s, LmnHInstructionStr[i]) == 0) break;
  }
  return i;
}


void dump_hregs(HIL hil)
{
  Vector *vec = LMN_GET_HREGS(hil);
  unsigned int i, n = vec_num(vec);
  printf("hil register size : %d\n\n", n);
  printf("hil register [\n");
  for (i = 0; i < n; i++) {
    LmnWord v = vec_get(vec, i);
    printf("\t%d : %ld\n", i, v);
  }
  printf("]\n");
}

/* funcotrs table */

void init_functors_table()
{
  int i, n = lmn_functor_table.next_id;
  unsigned int size = round2up(n);
  fs_tbl = LMN_MALLOC(struct FunctorsTable);
  fs_tbl->size = size;
  fs_tbl->entries = vec_make(size);
  for (i = 22; i < n; i++) {
    put_functor(i);
  }
  /* print_functors_table(); */
}

FunctorsEntry *make_functors_entry(unsigned int id, lmn_interned_str nameid)
{
  FunctorsEntry *entry = LMN_MALLOC(struct FunctorsEntry);
  entry->id = id;
  entry->nameid = nameid;
  entry->functors = vec_make(2);
  return entry;
}

FunctorsEntry *get_functors_entry(unsigned int id)
{
  FunctorsEntry *entry = (FunctorsEntry *) vec_get(fs_tbl->entries, id);
  return entry;
}

Vector *get_functors(unsigned int id)
{
  FunctorsEntry *entry = get_functors_entry(id);
  return entry->functors;
}

const char *get_functor_name(unsigned int id)
{
  FunctorsEntry *ent = get_functors_entry(id);
  return lmn_id_to_name(ent->nameid);
}

void put_functor(LmnFunctor f)
{
  FunctorsEntry *entry;
  unsigned int entry_id = find_functor_id(f);
  if (entry_id == EMPTY_FUNCTOR_KEY) {
    entry = make_functors_entry(fs_tbl->entries->num, LMN_FUNCTOR_NAME_ID(f));
    vec_push(fs_tbl->entries, (LmnWord) entry);
  } else {
    entry = (FunctorsEntry *) vec_get(fs_tbl->entries, entry_id);
  }
  vec_push(entry->functors, f);
}

unsigned int find_functor_id(LmnFunctor f)
{
  FunctorsEntry *ent;
  lmn_interned_str nameid = LMN_FUNCTOR_NAME_ID(f);
  unsigned int i, n = vec_num(fs_tbl->entries);
  for (i = 0; i < n; i++) {
    ent = (FunctorsEntry *) vec_get(fs_tbl->entries, i);
    if (ent->nameid == nameid) return ent->id;
  }
  return EMPTY_FUNCTOR_KEY;
}

unsigned int find_functor_id_by_name(char *atname)
{
  FunctorsEntry *ent;
  char *tmp;
  unsigned int i, n = vec_num(fs_tbl->entries);
  for (i = 0; i < n; i++) {
    ent = (FunctorsEntry *) vec_get(fs_tbl->entries, i);
    tmp = strstr(lmn_id_to_name(ent->nameid), atname);
    if (tmp != NULL && (strlen(tmp) - strlen(atname)) == 0) return ent->id;
  }
  return EMPTY_FUNCTOR_KEY;
}

// for debug
void print_functors_table()
{
  printf("\n----------------print functors table------------------------\n");
  unsigned int i, j, m, n = vec_num(fs_tbl->entries);
  for (i = 0; i < n; i++) {
    FunctorsEntry *ent = (FunctorsEntry *) vec_get(fs_tbl->entries, i);
    m = vec_num(ent->functors);
    const char *name = lmn_id_to_name(ent->nameid);
    printf("id : %u, name : %s,\tentries = [", ent->id, name);
    for (j = 0; j < m; j++) {
      LmnFunctor f = (LmnFunctor) vec_get(ent->functors, j);
      printf("%u ", f);
    }
    printf("]\n");
  }
  printf("----------------print functors table end------------------------\n");
}
