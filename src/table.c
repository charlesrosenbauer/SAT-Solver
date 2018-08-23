#include "table.h"
#include "stdint.h"
#include "stdlib.h"
#include "util.h"










void freeTable(TABLE* t){
  /*
  for(int i = 0; i < t->cols; i++)
    free(t->paramIndex[i]);
  free(t->paramIndex);

  for(int i = 0; i < t->rows; i++)
    free(t->clauseIndex[i]);
  free(t->clauseIndex);

  free(t->columns);

  free(t->allCells);

  free(t);
  */
}










TABLECELL initCell(IX x, IX y){
  TABLECELL ret;
  for(int i = 0; i < 4; i++){
    ret.vals[i] = 0;
    ret.mask[i] = 0;
  }
  ret.xnext = NULL;
  ret.ynext = NULL;
  ret.x = 0;
  ret.y = 0;
  return ret;
}









/*!!!!!!!!!!!!!!!!!!!!!!!!!
  !!!!WIP!!!!WIP!!!!WIP!!!!
  !!!!!!!!!!!!!!!!!!!!!!!!!*/
TABLE* initTable(CNF* c, int64_t sizeSuggest){

  TABLE* ret = malloc(sizeof(TABLE));

  //Get the total number of table cells.
  int cellct = 0;
  for(int i = 0; i < c->clausenum; i++){
    Clause cl = c->clauses[i];

    /*
      We need to count how many cells there are per column. The problem is that
      we need a set to do this properly, and sets can be pretty slow.

      A small bloom filter should be able to handle most cases. An N^2 lookup
      could be done in theory, but that might be a bit slow. So let's just check
      a bloom filter and a "last" value. If the last check fails and the hash
      passes, then we can do a worst-case check.

      I'm assuming CNF vars aren't sorted. If they are, then the whole hash and
      worst-case check is redundant and the "last" value is good enough.

      This should be able to handle most cases pretty efficiently.
    */
    uint64_t hash =  0;
    uint64_t last = -1;

    for(int j = 0; j < cl.numvars; j++){
      int x = cl.vars[j] / 256;
      uint64_t hashBit = (1 << (x % 64));
      if(x != last){
        last = x;

        // Check hash
        if(hashBit & hash){
          int matches = 0;
          // Worst-case check
          for(int k = 0; k < j; k++)
            matches += (cl.vars[k] == cl.vars[j]);

          cellct += matches != 0;  // Increment pass only if no matches
        }else{
          cellct++;
          hash |= hashBit;
        }
      }
    }
  }


  return ret;
}
