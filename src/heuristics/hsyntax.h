/* hsyntax.h */

#ifndef LMN_HSYNTAX_H
#define LMN_HSYNTAX_H

#include "lmntal.h"
#include "vector.h"
#include "util.h"
#include "functor.h"


typedef Vector *HArgList;
typedef Vector *HInstrList;
typedef unsigned long HDest;
typedef unsigned long HArg;
typedef unsigned long HLabelStr;
typedef Vector *HRegList;
typedef Vector *HLabelList;

struct HRegister {
  unsigned int size;
  HRegList registers;
};

struct HIL {
  HInstrList instrs;
  struct HRegister reg;
  HLabelList labels;
};

typedef struct HIL *HIL;

enum LmnHInstruction {
  HINSTR_INIT,
  HINSTR_ADD,
  HINSTR_SUB,
  HINSTR_MUL,
  HINSTR_DIV,
  HINSTR_STORE,
  HINSTR_GET,
  HINSTR_ABS,
  HINSTR_EQ,
  HINSTR_NOT,
  HINSTR_AND,
  HINSTR_OR,
  HINSTR_BR,
  HINSTR_BRT,
  HINSTR_DUMMY
};

struct HArgument
{
  // enum HType type;
  HArg val;
};

typedef struct HArgument *HArgument;
  
struct HInstruction {
  enum LmnHInstruction id;
  HArgList args;
  HDest dest;
};

typedef struct HInstruction *HInstruction;

struct HLabel {
  HLabelStr label;
  unsigned int pos;
};
typedef struct HLabel *HLabel;

/* HInstruction */

#define LMN_GET_HINSTR_ID(I) (I)->id
#define LMN_SET_HINSTR_ID(I, ID) (I)->id = (ID)
#define LMN_GET_HINSTR_ARGS(I) (I)->args 
#define LMN_SET_HINSTR_ARGS(I, ARGS) (I)->args = (ARGS)
#define LMN_GET_HINSTR_DEST(I) (I)->dest
#define LMN_SET_HINSTR_DEST(I, DEST) (I)->dest = (DEST)

HInstruction hinstr_make(enum LmnHInstruction id, HArgList args, HDest dest);
void hinstr_free(HInstruction inst);

/* HArgument */
#define LMN_HARG(ARG) ((HArgument) ARG)
#define LMN_GET_HARG(ARG) (ARG)->val
#define LMN_SET_HARG(ARG, VAL) (ARG)->val = (VAL)

HArgument harg_make(HArg val);
void harg_free(HArgument a);

/* HArgList */
HArgList harg_list_make();
void harg_list_push(HArgList args, HArgument arg);

/* HDest */
static inline BOOL is_register_variable(char *s);
static inline unsigned int get_register_index(char *s);
static inline unsigned int get_hdest(HInstruction instr);

/* HInstrList */
HInstrList hinstr_list_make();
void hinstr_list_push(HInstrList instrs, HInstruction instr);

/* HRegister */
#define LMN_GET_HREGLIST_SIZE(REG) (REG).size
#define LMN_SET_HREGLIST_SIZE(REG, SIZE) (REG).size = SIZE
#define LMN_GET_HREGLIST(REG) (REG).registers
#define LMN_SET_HREGLIST(REG, REGS) (REG).registers = REGS

/* HRegList */
static inline LmnWord get_hreg(HIL hil, unsigned int n);
static inline void set_hreg(HIL hil, unsigned int n, LmnWord v);

/* HIL */
#define LMN_GET_HINSTRS(HIL) (HIL)->instrs
#define LMN_GET_HREGISTER(HIL) (HIL)->reg
#define LMN_GET_HREGISTER_SIZE(HIL) (LMN_GET_HREGLIST_SIZE(LMN_GET_HREGISTER(HIL)))
#define LMN_SET_HREGISTER_SIZE(HIL, REGSIZE) (LMN_GET_HREGISTER_SIZE(HIL) = REGSIZE)
#define LMN_IS_HREGISTER_INITED(HIL) (LMN_GET_HREGISTER_SIZE(HIL) != 0)
#define LMN_GET_HREGS(HIL) (LMN_GET_HREGLIST(LMN_GET_HREGISTER(HIL)))
#define LMN_SET_HREGS(HIL, REGS) (LMN_GET_HREGS(HIL) = REGS)

HIL hil_make(HInstrList instrs, HLabelList labels);
HIL hil_local_make(HIL ghil);
void hil_init_hregs(HIL hil, unsigned int size);
void hil_init_symbol();

/* HLabel */
HLabel hlabel_make(HLabelStr label, unsigned long pos);
void hlabel_free(HLabel label);

/* HLabelList */
HLabelList hlabel_list_make();
void hlabel_list_push(HLabelList labels, HLabel label);
static inline unsigned int get_hlabel_pos(HIL hil, char *label);
  
/* dumper */
void dump_hil(HIL hil);
void dump_hinstr_list(HInstrList l);
void dump_hinstr(HInstruction i);
char *get_hinstr_by_id(enum LmnHInstruction id);
unsigned int get_hinstr_id(char *s);
void dump_hregs(HIL hil);


/* functors table */

#define EMPTY_FUNCTOR_KEY   0xffffffffU

typedef struct FunctorsTable FunctorsTable;
typedef struct FunctorsEntry FunctorsEntry;

struct FunctorsTable {
  unsigned int size;
  Vector *entries;
};

struct FunctorsEntry {
  unsigned int id;
  lmn_interned_str nameid;
  Vector *functors;
};

FunctorsTable *fs_tbl;

void init_functors_table();
FunctorsEntry *make_functors_entry();
void put_functor(LmnFunctor f);
unsigned int find_functor_id(LmnFunctor f);
unsigned int find_functor_id_by_name(char *atname);
FunctorsEntry *get_functors_entry(unsigned int id);
Vector *get_functors(unsigned int id);
const char *get_functor_name(unsigned int id);
void print_functors_table();


/* HDest */

static inline BOOL is_register_variable(char *s)
{
  char *ret = strchr(s, 'T');
  return (ret != NULL) ? TRUE : FALSE;
}

static inline unsigned int get_register_index(char *s)
{
  return atoi(++s);
}

static inline unsigned int get_hdest(HInstruction instr)
{
  return get_register_index((char *) LMN_GET_HINSTR_DEST(instr));
}

/* HRegList */

static inline LmnWord get_hreg(HIL hil, unsigned int n)
{
  return vec_get(LMN_GET_HREGS(hil), n);
}

static inline void set_hreg(HIL hil, unsigned int n, LmnWord v)
{
  vec_set(LMN_GET_HREGS(hil), n, v);
}


/* HLabelList */
static inline unsigned int get_hlabel_pos(HIL hil, char *label)
{
  HLabel l;
  unsigned int i, n = vec_num(hil->labels);
  for (i = 0; i < n; i++) {
    l = (HLabel) vec_get(hil->labels, i);
    if (strcmp((char *) l->label, label) == 0) {
      return l->pos;
    }
  }
  // do not reach this line.
  return 0;
}

#endif // LMN_HSYNTAX_H








