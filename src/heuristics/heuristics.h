/* heuristics.h */

#include <stdio.h>
#include "hsyntax.h"
#include "vector.h"
#include "lmntal.h"
#include "state.h"

HIL hil;

int load_hil_file(char *file_name, HIL *hil);
int get_heuristic_f(HIL hil, State *s);
int get_heuristic_g(HIL hil, State *s);
int get_heuristic_h(HIL hil, State *s, LmnMembrane *mem);
Vector *get_functors_by_atname(const char *atname);


