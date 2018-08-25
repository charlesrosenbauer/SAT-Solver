#include "table.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
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










inline int lessTCell(TABLECELL* a, TABLECELL* b){
  if(a->x < b->x) return 1;
  if(a->x > b->x) return 0;

  for(int i = 0; i < 4; i++){
    if(a->mask[i] < b->mask[i]) return 1;
    if(a->mask[i] > b->mask[i]) return 0;
  }

  return 0;
}










inline int moreTCell(TABLECELL* a, TABLECELL* b){
  if(a->x > b->x) return 1;
  if(a->x < b->x) return 0;

  for(int i = 0; i < 4; i++){
    if(a->mask[i] > b->mask[i]) return 1;
    if(a->mask[i] < b->mask[i]) return 0;
  }

  return 0;
}










void sortTCells(TABLECELL* arr, int lo, int hi){
  int pivot, j, i;
  TABLECELL temp;
  if(lo < hi){
    pivot = lo;
    i = lo;
    j = hi;

    while(i < j){

      while(!(moreTCell(&arr[i], &arr[pivot])) && (i < hi)) i++;

      while(moreTCell(&arr[j], &arr[pivot])) j--;

      if(i < j){
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }

    temp = arr[pivot];
    arr[pivot] = arr[j];
    arr[j] = temp;
    sortTCells(arr, lo , j-1);
    sortTCells(arr, j+1, hi );
  }
}






int min(int a, int b){
  return (a < b)? a : b;
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
    uint64_t prev = -1;

    for(int j = 0; j < cl.numvars; j++){
      int x = abs(cl.vars[j]) / 256;
      uint64_t hashBit = (1 << (x % 64));
      if(x != last){
        // Check hash
        if((x != prev) && (hashBit & hash)){
          int matches = 0;
          // Worst-case check
          for(int k = 0; k < j; k++)
            matches += (abs(cl.vars[k]/256) == abs(cl.vars[j]/256));

          cellct += matches != 0;  // Increment pass only if no matches
        }else{
          cellct++;
          hash |= hashBit;
        }
        prev = last;
        last = x;
      }
    }
  }

  printf("CELLCT: %i\n", cellct);

  TABLECELL* allCells = malloc(sizeof(TABLECELL) * cellct);


  /*
    Fill allCells with tablecells
  */
  int cellTop = 0;
  for(int i = 0; i < c->clausenum; i++){
    Clause cl = c->clauses[i];

    int stack[1024];
    int top = cl.numvars;
    for(int j = 0; j < cl.numvars; j++) stack[j] = cl.vars[j];

    while(top > 0){
      int index = abs(stack[0]) / 256;
      int newtop = top;
      allCells[cellTop].y = i;
      for(int k = 0; k < 4; k++){
        allCells[cellTop].vals[k] = 0;
        allCells[cellTop].mask[k] = 0;
      }
      int newbot = 0;
      for(int j = 0; j < newtop; j++){
        int x = abs(stack[j]);
        if((x/256) == index){
          int n = (stack[j] < 0)? 0 : 1;
          allCells[cellTop].mask[(x/64)%4] |= (1 << (x%64));
          allCells[cellTop].vals[(x/64)%4] |= (n << (x%64));
          newtop--;
        }else{
          stack[newbot] = stack[j];
          newbot++;
        }
        allCells[cellTop].x = x / 256;
      }
      cellTop++;
      top = newtop;
    }
  }

  /*
    Sort tablecells
  */
  sortTCells(allCells, 0, cellct);


  for(int i = 0; i < min(16, cellct); i++){
    TABLECELL t = allCells[i];
    printf("x%i y%i || %lu %lu %lu %lu\n", t.x, t.y, t.mask[0], t.mask[1], t.mask[2], t.mask[3]);
  }

  return ret;
}
