#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__

#include "stdint.h"










static uint64_t rseed[2];









typedef struct{
  int val, x;
}IntPair;










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










void quicksort(IntPair*, int, int);   // Sorts IntPairs by x
int cttz(uint64_t);
uint64_t isomsb(uint64_t);
int ctlz(uint64_t);
int popcount(uint64_t);
uint64_t rng64();
void rngseed(uint64_t, uint64_t);





#endif
