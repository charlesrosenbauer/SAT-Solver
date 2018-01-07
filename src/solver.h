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










typedef struct{
  int val, x;
}IntPair;










typedef struct{
  void* nodes[32];
}PersistentNode;










typedef struct{
  int size;
  int depth;
  void* nodes[32];
}PersistentByteArray;










IntPair* sortByMentions(CNF*);
PersistentByteArray* createByteArray(int);
void pbaUnsafeWrite(PersistentByteArray*, int, char);
unsigned char pbaRead       (PersistentByteArray*, int);




#endif
