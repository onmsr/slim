/* hsyntax.h */

#ifndef LMN_HSYNTAX_H
#define LMN_HSYNTAX_H

#include "vector.h"

typedef Vector *HArgList;
typedef Vector *HInstrList;
typedef unsigned long HDest;
typedef unsigned long HArg;

struct HIL {
  HInstrList instrs;
};

typedef struct HIL *HIL;

enum LmnHInstruction {
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
#define LMN_GET_HARG(ARG) (ARG)->val
#define LMN_SET_HARG(ARG, VAL) (ARG)->val = (VAL)

HArgument harg_make(HArg val);
void harg_free(HArgument a);

/* HArgList */
HArgList harg_list_make();
void harg_list_push(HArgList args, HArgument arg);

/* HInstrList */
HInstrList hinstr_list_make();
void hinstr_list_push(HInstrList instrs, HInstruction instr);

/* HIL */
HIL hil_make(HInstrList instrs);

/* dumper */
void dump_hil(HIL hil);
void dump_hinstr_list(HInstrList l);
void dump_hinstr(HInstruction i);
void dump_harg_list(HArgList al);
void dump_harg(HArgument arg);
char *get_hinstr_by_id(enum LmnHInstruction id);
unsigned int get_hinstr_id(char *s);

#endif // LMN_HSYNTAX_H








