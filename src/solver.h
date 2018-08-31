#ifndef __SOLVER_HEADER__
#define __SOLVER_HEADER__


#include "stdint.h"









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
  uint64_t* cstdata;
  uint64_t* cstmask;
  uint64_t* prddata;

  int64_t*  propixs;

  int32_t*  unsatct;
  int32_t*  fstsat;

  int clausect, varct;
}SOLVERSTATE;




#endif
