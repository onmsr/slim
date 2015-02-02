/* heuristics.c */

#include "heuristics.h"
#include "hil_lexer.h"
#include "hil_parser.h"

#ifdef PROFILE
#  include "runtime_status.h"
#endif

static int hil_parse(FILE *in, HIL *hil)
{
  int ret;
  extern FILE *hilin;
  extern int hilparse(HIL *);

  hilin = in;
  if ((ret = hilparse(hil))) {
    fprintf(stderr, "Error !\n");
    exit(1);
  }
  hillex_destroy();

  return ret;
}

int load_hil_file(char *file_name, HIL *hil)
{
  FILE *fp_hil = fopen(file_name, "r");
  if (!fp_hil) fprintf(stderr, "hil file open error.\n");
  int ret = hil_parse(fp_hil, hil);
  fclose(fp_hil);
  return ret;
}

int get_heuristic_g(HIL hil, State *s)
{
  return s->g;
}


int get_heuristic_h(HIL hil, State *s, LmnMembrane *mem)
{
#ifdef PROFILE
  if (lmn_env.profile_level >= 3) {
    profile_start_timer(PROFILE_TIME__CALC_HEURISTIC_FUNC);
  }
#endif

  if (mem == NULL) {
    mem = state_restore_mem(s); 
  }

  int i;
  unsigned int n = vec_num(hil->instrs);
  for (i = 0; i < n; i++) {
    HInstruction instr = (HInstruction) vec_get(hil->instrs, i);
    HArgList hargs = LMN_GET_HINSTR_ARGS(instr);
    unsigned int dest = get_hdest(instr);

    switch (instr->id) {
    case HINSTR_INIT:
    {
      // init hil registers
      if (!LMN_IS_HREGISTER_INITED(hil)) {
        HArgument arg1 = (HArgument) vec_get(hargs, 0);
        unsigned int reg_size = (unsigned int) atoi((char *) LMN_GET_HARG(arg1));
        hil_init_hregs(hil, reg_size);
        set_hreg(hil, dest, reg_size);
      }
      break;
    }
    case HINSTR_ADD:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      unsigned int regn1 = get_register_index((char *) LMN_GET_HARG(arg1));
      unsigned int regn2 = get_register_index((char *) LMN_GET_HARG(arg2));
      LmnWord res = get_hreg(hil, regn1) + get_hreg(hil, regn2);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_SUB:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      unsigned int regn1 = get_register_index((char *) LMN_GET_HARG(arg1));
      unsigned int regn2 = get_register_index((char *) LMN_GET_HARG(arg2));
      LmnWord res = (LmnWord) (((long) get_hreg(hil, regn1)) - ((long) get_hreg(hil, regn2))); 
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_MUL:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      unsigned int regn1 = get_register_index((char *) LMN_GET_HARG(arg1));
      unsigned int regn2 = get_register_index((char *) LMN_GET_HARG(arg2));
      LmnWord res = get_hreg(hil, regn1) * get_hreg(hil, regn2);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_DIV:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      unsigned int regn1 = get_register_index((char *) LMN_GET_HARG(arg1));
      unsigned int regn2 = get_register_index((char *) LMN_GET_HARG(arg2));
      LmnWord res = get_hreg(hil, regn1) / get_hreg(hil, regn2);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_STORE:
    {
      HArgument arg = (HArgument) vec_get(hargs, 0);
      char *val_or_regvar = (char *) LMN_GET_HARG(arg);
      LmnWord res = (is_register_variable(val_or_regvar)) ? get_hreg(hil, get_register_index(val_or_regvar)) : atoi(val_or_regvar);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_GET:
    {
      // get instruction arguments
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);

      unsigned int id = (unsigned int) LMN_GET_HARG(arg1);
      char *val_or_regvar = (char *) LMN_GET_HARG(arg2);
      int pos = (is_register_variable(val_or_regvar)) ?
        get_hreg(hil, get_register_index(val_or_regvar))-1 : atoi(val_or_regvar)-1;
      if (pos < 0) {
        fprintf(stderr, "heuristic function [GET] : get link number is ilegal.");
        exit(1);
      }

      // functors
      Vector *fs = get_functors(id);

      unsigned int j, n = vec_num(fs);
      for (j = 0; j < n; j++) {
        LmnFunctor f = (LmnFunctor) vec_get(fs, j);
        
        LmnSAtom atom;
        EACH_FUNC_ATOM(mem, f, atom, ({
              if (atom != NULL) {
                LmnLinkAttr attr = LMN_SATOM_GET_ATTR(atom, pos);
                if (LMN_ATTR_IS_DATA(attr)) {
                  unsigned int val = LMN_SATOM_GET_LINK(atom, pos);
                  set_hreg(hil, dest, val);
                } else {
                  fprintf(stderr, "heuristic function [GET] : atom attr error.");
                  exit(1);
                }
                break;
              }
            }));
      }
      
      break;
    }
    case HINSTR_ABS:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      unsigned int regn1 = get_register_index((char *) LMN_GET_HARG(arg1));
      LmnWord res = abs(get_hreg(hil, regn1));
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_EQ:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      char *val_or_regvar1 = (char *) LMN_GET_HARG(arg1);
      char *val_or_regvar2 = (char *) LMN_GET_HARG(arg2);
      int val1 = (is_register_variable(val_or_regvar1)) ? get_hreg(hil, get_register_index(val_or_regvar1)) : atoi(val_or_regvar1);
      int val2 = (is_register_variable(val_or_regvar2)) ? get_hreg(hil, get_register_index(val_or_regvar2)) : atoi(val_or_regvar2);
      LmnWord res = (val1 == val2);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_NOT:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      char *val_or_regvar1 = (char *) LMN_GET_HARG(arg1);
      int val1 = (is_register_variable(val_or_regvar1)) ? get_hreg(hil, get_register_index(val_or_regvar1)) : atoi(val_or_regvar1);
      LmnWord res = (!val1);
      set_hreg(hil, dest, res);
      break;
    }
    case HINSTR_AND:
    {
      // @not implemented
      break;
    }
    case HINSTR_OR:
    {
      // @not implemented
      break;
    }
    case HINSTR_BR:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      char *label = (char *) LMN_GET_HARG(arg1);
      i = get_hlabel_pos(hil, label)-1;
      set_hreg(hil, dest, 1);
      break;
    }
    case HINSTR_BRT:
    {
      HArgument arg1 = (HArgument) vec_get(hargs, 0);
      HArgument arg2 = (HArgument) vec_get(hargs, 1);
      char *label = (char *) LMN_GET_HARG(arg1);
      char *val_or_regvar = (char *) LMN_GET_HARG(arg2);
      int cond = (is_register_variable(val_or_regvar)) ? get_hreg(hil, get_register_index(val_or_regvar)) : atoi(val_or_regvar);
      if (cond != 0) {
        i = get_hlabel_pos(hil, label)-1;
      }
      set_hreg(hil, dest, (cond != 0));
      break;
    }
    case HINSTR_DUMMY:
    {
      break;
    }
    default:
      fprintf(stderr, "heuristic function: Unknown heuristic instruction %d\n", instr->id);
      exit(1);
    }
  }
  
#ifdef PROFILE
  if (lmn_env.profile_level >= 3) {
    profile_finish_timer(PROFILE_TIME__CALC_HEURISTIC_FUNC);
  }
#endif

  return (LMN_IS_HREGISTER_INITED(hil)) ? get_hreg(hil, get_hreg(hil, 0)) : 0;
}

Vector *get_functors_by_atname(const char *atname)
{
  Vector *functors = vec_make(2);
  int i, n = lmn_functor_table.next_id;
  char *tmp, *tmpatname;
  for (i = 22; i < n; i++) {
    tmpatname = (char *) lmn_id_to_name(LMN_FUNCTOR_NAME_ID(i));
    tmp = strstr(tmpatname, atname);
    if (tmp != NULL && (tmp - tmpatname) == 0) {
      vec_push(functors, (LmnWord) i);
    }
  }
  return functors;
}


