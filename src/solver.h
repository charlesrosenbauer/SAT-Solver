#ifndef __SOLVER_HEADER__
#define __SOLVER_HEADER__










typedef struct{
  int  numvars;
  int* vars;
}Clause;










typedef struct{
  int varnum, clausenum;
  Clause* clauses;
}CNF;










int* countMentions(CNF*);



#endif
