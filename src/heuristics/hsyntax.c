/* hsyntax.c */


#include "hsyntax.h"

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

/* HDest */

BOOL is_register_variable(char *s)
{
  char *ret = strchr(s, 'T');
  return (ret != NULL) ? TRUE : FALSE;
}

unsigned int get_register_index(char *s)
{
  return atoi(++s);
}

unsigned int get_hdest(HInstruction instr)
{
  return get_register_index((char *) LMN_GET_HINSTR_DEST(instr));
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

HIL hil_make(HInstrList instrs)
{
  HIL hil = LMN_MALLOC(struct HIL);
  hil->instrs = instrs;
  return hil;
}

/* HRegister */

void hil_init_hregs(HIL hil, unsigned int size)
{
  size += 1; // for register zero
  LMN_SET_HREGISTER_SIZE(hil, size);
  LMN_SET_HREGS(hil, vec_make(round2up(size)));
  int i;
  for (i = 0; i < size; i++) {
    vec_push(LMN_GET_HREGS(hil), 0);
  }
}

/* HRegList */

LmnWord get_hreg(HIL hil, unsigned int n)
{
  return vec_get(LMN_GET_HREGS(hil), n);
}

void set_hreg(HIL hil, unsigned int n, LmnWord v)
{
  vec_set(LMN_GET_HREGS(hil), n, v);
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

void dump_hinstr(HInstruction i)
{
  printf("%s ", get_hinstr_by_id(i->id));
  dump_harg_list(i->args);
  printf(" %s\n", (char *) i->dest);
}

void dump_harg_list(HArgList al)
{
  HArgument arg;
  unsigned int i, n = vec_num(al);
  printf("[ ");
  for (i = 0; i < n; i++) {
    if (i != 0) printf(" , ");
    arg = (HArgument) vec_get(al, i);
    dump_harg(arg);
  }
  printf(" ]");
}

void dump_harg(HArgument arg)
{
  printf("%s", (char *) LMN_GET_HARG(arg));
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
