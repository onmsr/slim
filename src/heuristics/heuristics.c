/* heuristics.c */

#include "heuristics.h"

static int hil_parse(FILE *in, HIL *hil)
{
  int ret;
  extern FILE *hilin;

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

int get_heuristic_h(HIL hil, State s)
{
  return 0;
}


int get_heuristic_g(HIL hil, State s)
{
  return 0;
}


int get_heuristic_f(HIL hil, State s)
{
  int ret = 0;
  HInstruction instr;
  unsigned int i, n = vec_num(hil->instrs);
  for (i = 0; i < n; i++) {
    instr = (HInstruction) vec_get(hil->instrs, i);

    switch (instr->id) {
    case HINSTR_ADD:
    {
      break;
    }
    case HINSTR_SUB:
    {
      break;
    }
    case HINSTR_MUL:
    {
      break;
    }
    case HINSTR_DIV:
    {
      break;
    }
    case HINSTR_STORE:
    {
      break;
    }
    case HINSTR_GET:
    {
      break;
    }
    case HINSTR_ABS:
    {
      break;
    }
    case HINSTR_DUMM:
    {
      break;
    }
    default:
    
    }
  }
  return ret;
}

