#ifndef __CNFPARSER_HEADER__
#define __CNFPARSER_HEADER__










typedef struct{
  int  numvars;
  int* vars;
}Clause;










typedef struct{
  int varnum, clausenum;
  Clause* clauses;
}CNF;










CNF parseCNF(char*, int);








#endif
