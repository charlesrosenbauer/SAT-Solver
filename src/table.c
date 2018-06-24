#include "table.h"
#include "stdint.h"
#include "stdlib.h"










void freeTable(TABLE* t){
  for(int i = 0; i < t->cols; i++)
    free(t->paramIndex[i]);
  free(t->paramIndex);

  for(int i = 0; i < t->rows; i++)
    free(t->clauseIndex[i]);
  free(t->clauseIndex);

  free(t->columns);

  free(t->allCells);

  free(t);
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
*/!!!!!!!!!!!!!!!!!!!!!!!!!
TABLE* initTable(CNF* c, int64_t sizeSuggest){

  TABLE* ret = malloc(sizeof(TABLE));

  /*
    Around here we should probably sort clauses and parameters to get them into
    a friendly order, but we'll save that for later.
  */

  if(sizeSuggest <= 0){
    sizeSuggest = c->clausenum * 2;
  }

  ret->allCells = malloc(sizeof(TABLECELL) * sizeSuggest);

  ret->cols = (c->varnum % 256)? (c->varnum / 256)+1 : c->varnum;
  ret->rows =  c->clausenum;


  // Generate Table Cells

  uint64_t cellIndex = 0;

  for(int i = 0; i < ret->cols; i++){   // Parameters
    for(int j = 0; j < ret->rows; j++){   // Clauses

      /*
        Rows are the inner loop here because most operations on this table will
        iterate over data from within the same column. As a result,
      */

      Clause* cl = &(c->clauses[j]);

      TABLECELL cell = initCell(i, j);

      int isCellModified = 0;

      for(int k = 0; k < cl->numvars; k++){
        int x = cl->vars[k];

        if((x >= (i * 256)) && (x < ((i+1) * 256))){
          // Fill Cell if possible
          IX xi = x / 256;
          IX xj = x % 256;
          cell.vals[xi] = (1 << xj);
          cell.mask[xi] = (1 << xj);
          isCellModified = 1;
        }
      }

      // TODO:
      // If Cell is nonzero, add it to the cell index. If there's not enough
      // room, retry the whole initialization with a bigger size.

      if(isCellModified){
        if(cellIndex >= sizeSuggest){
          // Memory allocated for table is too small! Allocate more and retry!
          freeTable(ret);
          return initTable(c, sizeSuggest * 2);
        }


        cellIndex++;
      }
    }
  }

  ret->cellCount = cellIndex + 1;




  return ret;
}
