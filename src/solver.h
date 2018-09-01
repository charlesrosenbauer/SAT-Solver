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

  // Constants
  uint64_t* cstdata;
  uint64_t* cstmask;

  // Predictions
  uint64_t* prddata;

  // Unit Propagation
  int64_t*  propixs;

  // Clause Satisifaction
  int32_t*  unsatct;
  int32_t*  fstsat;

  uint64_t* satclause;

  uint64_t* currentdata;
  uint64_t  workix;

  // Sizes
  int clausect, varct, clausesz, varsz;
}SOLVERSTATE;




#endif
