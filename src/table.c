#include "table.h"
#include "stdint.h"
#include "stdlib.h"
#include "util.h"










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
  !!!!!!!!!!!!!!!!!!!!!!!!!*/
TABLE* initTable(CNF* c, int64_t sizeSuggest){

  TABLE* ret = malloc(sizeof(TABLE));



  return ret;
}
