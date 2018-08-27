#ifndef __TABLE_HEADER__
#define __TABLE_HEADER__

#include "stdint.h"
#include "solver.h"










/*
  This type puts a limit on how many parameters/clauses can be used.
  32 bit should suffice for now. I think there's a bottleneck here with
  sequential performance, and the solver shouldn't be able to pass ~10 million
  parameters per second, even under ideal circumstances. Handling more than 2
  billion parameters probably isn't a priority, at least not for now. ;)
*/
typedef int32_t IX;










typedef struct {
  uint64_t vals[4];
  uint64_t mask[4];

  void* xnext;
  void* ynext;

  IX x, y;
}TABLECELL;










typedef struct{
  IX* clauseixs;
  IX* columnixs;

  IX cols;      // Parameters
  IX rows;      // Clauses

  // This data likely will just be here for cleanup purposes.
  TABLECELL* allCells;
  uint64_t cellCount;

  uint64_t* predictions;
  IX varct;
}TABLE;










void      freeTable(TABLE*);
TABLECELL initCell (IX, IX);
TABLE*    initTable(CNF*, int64_t);



#endif
