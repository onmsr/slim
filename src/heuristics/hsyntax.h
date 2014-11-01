/* hsyntax.h */

#ifndef LMN_HSYNTAX_H
#define LMN_HSYNTAX_H

#include "vector.h"
#include "util.h"

typedef Vector *HArgList;
typedef Vector *HInstrList;
typedef unsigned long HDest;
typedef unsigned long HArg;
typedef Vector *HRegList;

struct HRegister {
  unsigned int size;
  HRegList registers;
};

struct HIL {
  HInstrList instrs;
  struct HRegister reg;
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
BOOL is_register_variable(char *s);
unsigned int get_register_index(char *s);
unsigned int get_hdest(HInstruction instr);

/* HInstrList */
HInstrList hinstr_list_make();
void hinstr_list_push(HInstrList instrs, HInstruction instr);

/* HRegister */
#define LMN_GET_HREGLIST_SIZE(REG) (REG).size
#define LMN_SET_HREGLIST_SIZE(REG, SIZE) (REG).size = SIZE
#define LMN_GET_HREGLIST(REG) (REG).registers
#define LMN_SET_HREGLIST(REG, REGS) (REG).registers = REGS

/* HRegList */
LmnWord get_hreg(HIL hil, unsigned int n);
void set_hreg(HIL hil, unsigned int n, LmnWord v);

/* HIL */
#define LMN_GET_HINSTRS(HIL) (HIL)->instrs
#define LMN_GET_HREGISTER(HIL) (HIL)->reg
#define LMN_GET_HREGISTER_SIZE(HIL) (LMN_GET_HREGLIST_SIZE(LMN_GET_HREGISTER(HIL)))
#define LMN_SET_HREGISTER_SIZE(HIL, REGSIZE) (LMN_GET_HREGISTER_SIZE(HIL) = REGSIZE)
#define LMN_IS_HREGISTER_INITED(HIL) (LMN_GET_HREGISTER_SIZE(HIL) != 0)
#define LMN_GET_HREGS(HIL) (LMN_GET_HREGLIST(LMN_GET_HREGISTER(HIL)))
#define LMN_SET_HREGS(HIL, REGS) (LMN_GET_HREGS(HIL) = REGS)

HIL hil_make(HInstrList instrs);
void hil_init_hregs(HIL hil, unsigned int size);


/* dumper */
void dump_hil(HIL hil);
void dump_hinstr_list(HInstrList l);
void dump_hinstr(HInstruction i);
void dump_harg_list(HArgList al);
void dump_harg(HArgument arg);
char *get_hinstr_by_id(enum LmnHInstruction id);
unsigned int get_hinstr_id(char *s);
void dump_hregs(HIL hil);

#endif // LMN_HSYNTAX_H








