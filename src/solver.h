#ifndef __SOLVER_HEADER__
#define __SOLVER_HEADER__










// Limit to the number of parameters per clause
static const int PARLIMIT = 16384;










typedef struct{
  int  numvars;
  int* vars;
}Clause;










typedef struct{
  int varnum, clausenum;
  Clause* clauses;
}CNF;










typedef struct{
  int* trans;
  int  size;
}TRANSLATION;




#endif
