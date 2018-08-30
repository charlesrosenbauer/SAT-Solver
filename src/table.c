#include "table.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "util.h"










void freeTable(TABLE* t){
  free(t->clauseixs);
  free(t->columnixs);
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

  for(int i = 3; i >= 0; i--){
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

  printf("Cell Estimate: %i\n", cellct);

  TABLECELL* allCells = malloc(sizeof(TABLECELL) * cellct);


  /*
    Fill allCells with tablecells
  */
  int cellTop = 0;
  for(int i = 0; i < c->clausenum; i++){
    Clause cl = c->clauses[i];

    int stack[PARLIMIT];
    int top = cl.numvars;
    for(int j = 0; j < cl.numvars; j++) stack[j] = cl.vars[j];

    while(top > 0){

      // Initialize Table Cell
      allCells[cellTop].y = i;
      allCells[cellTop].xnext = NULL;
      allCells[cellTop].ynext = NULL;
      for(int k = 0; k < 4; k++){
        allCells[cellTop].vals[k] = 0;
        allCells[cellTop].mask[k] = 0;
      }

      int index = abs(stack[0]) / 256;
      int newbot = 0;
      for(int j = 0; j < top; j++){
        uint64_t x = abs(stack[j]);
        if((x/256) == index){
          uint64_t n = (stack[j] < 0)? 0 : 1;
          uint64_t d0 = (((uint64_t)1) << (x%64));
          uint64_t d1 = (           n  << (x%64));
          allCells[cellTop].mask[(x/64)%4] |= d0;
          allCells[cellTop].vals[(x/64)%4] |= d1;
        }else{
          stack[newbot] = stack[j];
          newbot++;
        }
        allCells[cellTop].x = x / 256;
      }
      cellTop++;
      top = newbot;
    }
  }

  printf("Cell Total   : %i\n", cellTop);

  /*
    Sort tablecells
  */
  printf("Sorting Table...\n");
  sortTCells(allCells, 0, cellTop-1);
  printf("Table Sorted.\n");

  /*
    Initialize Table Data
  */
  ret->clauseixs = malloc(sizeof(IX) * c->clausenum);
  ret->columnixs = malloc(sizeof(IX) * c->varnum);
  ret->cellCount = cellTop;
  ret->varct     = c->varnum;
  ret->cols      = (c->varnum % 256)? (c->varnum / 256)+1 : (c->varnum / 256);
  ret->rows      = c->clausenum;
  ret->allCells  = allCells;

  for(int i = 0; i < c->clausenum; i++)
    ret->clauseixs[i] = -1;

  for(int i = 0; i < c->varnum; i++)
    ret->columnixs[i] = -1;

  /*
    Link Table Cells
  */
  printf("Linking up SAT Table...\n");

  for(int i = 0; i < cellTop; i++){
    TABLECELL* cell = &(allCells[i]);

    // Attempt to link to clauseixs
    IX clauseix = ret->clauseixs[cell->y];
    if(clauseix == -1){
      // Clauseix is untaken, claim it
      ret->clauseixs[cell->y] = i;
    }else{
      /*
         If clauseix is already taken, jump to the associated node and define
         it's xnext value. That way, clauseixs references a linked list of cells
         That all belong to the same clause
      */
      TABLECELL *here = &(allCells[clauseix]);
      while(here->xnext != NULL){
        here = here->xnext;
      }
      here->xnext = cell;
    }

    /*
        Link cells to the next cell in their column.
        If the cell is the last in it's column, leave it null.
        This forms a linked list of cells in each column, though the
        organization of the table kind of makes this a little redundant. Oh
        well, it's a fast way to check if we're at the end of a column with a
        lower change of a cache miss.
    */
    if((i + 1) < cellTop){
      if(allCells[i+1].x != cell->x){
        cell->ynext = NULL;
        ret->columnixs[allCells[i+1].x] = i+1;
      }else{
        cell->ynext = &(allCells[i+1]);
      }
    }else{
      cell->ynext = NULL;
    }
  }

  printf("Table linked.\n");

  return ret;
}
