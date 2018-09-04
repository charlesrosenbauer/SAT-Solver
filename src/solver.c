#include "solver.h"
#include "stdlib.h"
#include "stdio.h"
#include "global.h"
#include "table.h"
#include "util.h"










int* countMentions(CNF* cnf){
  int* mentions = malloc(sizeof(int) * cnf->varnum);

  for(int i = 0; i < cnf->varnum; i++)
    mentions[i] = 0;

  for(int i = 0; i < cnf->clausenum; i++){
    Clause c = cnf->clauses[i];

    for(int j = 0; j < c.numvars; j++){
      int index = (c.vars[j] < 0)? -c.vars[j] : c.vars[j];
      mentions[index-1]++;
    }
  }

  return mentions;
}










SOLVERSTATE makeSolverState(CNF* cnf){

  SOLVERSTATE ret;
  ret.varct   =  cnf->varnum + 1;
  ret.varsz   = (ret.varct % 64)? (cnf->varnum / 64)+1 : (ret.varct/64);
  ret.clausect=  cnf->clausenum;

  ret.cstdata = malloc(sizeof(uint64_t) * ret.varsz);
  ret.cstmask = malloc(sizeof(uint64_t) * ret.varsz);
  ret.prddata = malloc(sizeof(uint64_t) * ret.varsz);
  ret.currentdata = malloc(sizeof(uint64_t) * ret.varsz);
  for(int i = 0; i < ret.varsz; i++){
    ret.cstdata[i] = 0;
    ret.cstmask[i] = 0;
    ret.prddata[i] = 0;
    ret.currentdata[i] = 0;
  }

  ret.workix = 0;

  ret.propixs = malloc(sizeof(uint64_t) * ret.varct);
  for(int i = 0; i < ret.varct; i++){
    ret.propixs[i] = -1;
  }

  ret.unsatct = malloc(sizeof( int32_t) * ret.clausect);
  ret.fstsat  = malloc(sizeof( int32_t) * ret.clausect);
  for(int i = 0; i < ret.clausect; i++){
    ret.unsatct[i] = cnf->clauses[i].numvars;
    ret.fstsat [i] = 0;
  }

  ret.clausesz  = (cnf->clausenum % 64)? (cnf->clausenum/64)+1 : (cnf->clausenum/64);
  ret.satclause = malloc(sizeof(uint64_t) * ret.clausesz);
  for(int i = 0; i < ret.clausesz; i++){
    ret.satclause[i] = 0;
  }

  return ret;
}










void freeSolverState(SOLVERSTATE* s){
  free(s->cstdata);
  free(s->cstmask);
  free(s->prddata);
  free(s->propixs);
  free(s->unsatct);
  free(s->fstsat);
  free(s->satclause);
  free(s->currentdata);
}










int getconstants(SOLVERSTATE* s, CNF* c, TABLE* t){

  int csts = 0;
  for(int i = 0; i < c->clausenum; i++){
    if(c->clauses[i].numvars == 1){
      s->unsatct[i] = 0;
      int cstx = c->clauses[i].vars[0];
      int csti = abs(cstx);
      uint64_t ic = csti / 64;
      uint64_t jc = csti % 64;
      uint64_t mk = ((uint64_t)1) << jc;
      uint64_t vl = (cstx < 0)? 0 : mk;
      if(s->cstmask[ic] & mk){
        if((s->cstdata[ic] ^ vl) & mk){
          // Conflict Here!!! UNSAT!!
          printf("0 constant propogation passes\n");
          return csti;
        }
      }else{
        // Mark constant
        s->cstmask[ic] |= mk;
        s->cstdata[ic] |= vl;
        csts++;
      }
    }
  }

  if(csts == 0){
    // No constant propagation will occur with no constants.
    printf("0 constant propogation passes\n");
    return 0;
  }

  int cstct = 0;
  int last  = -1;
  int passes= 0;
  while(cstct != last){
    last = cstct;
    passes++;
    // Iterate over columns to get constants
    for(int i = 0; i < t->cols; i++){
      int i4 = (i * 4);
      uint64_t cmask[4];
      cmask[0] = s->cstmask[i4];
      cmask[1] = ((i4 + 1) > s->varsz)? 0 : s->cstmask[i4+1];
      cmask[2] = ((i4 + 2) > s->varsz)? 0 : s->cstmask[i4+2];
      cmask[3] = ((i4 + 3) > s->varsz)? 0 : s->cstmask[i4+3];

      uint64_t cdata[4];
      cdata[0] = s->cstdata[i4];
      cdata[1] = ((i4 + 1) > s->varsz)? 0 : s->cstdata[i4+1];
      cdata[2] = ((i4 + 2) > s->varsz)? 0 : s->cstdata[i4+2];
      cdata[3] = ((i4 + 3) > s->varsz)? 0 : s->cstdata[i4+3];

      int limit = ((i+1) != t->cols)? t->columnixs[i+1] : t->cellCount;
      for(int j = t->columnixs[i]; j < limit; j++){
        TABLECELL* cl = &(t->allCells[j]);
        int x = 0;
        if((cl->mask[0]&cmask[0]) || (cl->mask[1]&cmask[1]) || (cl->mask[2]&cmask[2]) || (cl->mask[3]&cmask[3])){
          // count number of vars that are unsatisfied
          x  = popcount(cl->mask[0] & (cl->vals[0] ^ cdata[0]));
          x += popcount(cl->mask[1] & (cl->vals[1] ^ cdata[1]));
          x += popcount(cl->mask[2] & (cl->vals[2] ^ cdata[2]));
          x += popcount(cl->mask[3] & (cl->vals[3] ^ cdata[3]));
        }
        int y = s->unsatct[j];
        s->unsatct[j] -= x;
      }
    }

    // Check for and report any conflicts of course!
    // Repeat all of this until no unit propogation occurs any more
    for(int i = 0; i < c->clausenum; i++){
      // If any have 1 fewer unsat vars than total vars, unit prop!
      if(s->unsatct[i] == 1){
        int cont = 1;
        TABLECELL* next = &(t->allCells[t->clauseixs[i]]);
        while(cont && (next != NULL)){

          for(int j = 0; j < 4; j++){
            uint64_t remain = s->cstmask[(4*(next->x))+j] ^ next->mask[j];
            if(((next->x * 4) + j) >= s->varsz) break;
            if(remain){
              // Remaining unsatisfied variable found! Set it as a constant!
              cont = 0;
              int pick = ctlz(remain);
              int indx = (4*(next->x)) + j;
              uint64_t mk = ((uint64_t)1) << pick;
              uint64_t vl = next->vals[j] & mk;
              if(s->cstmask[indx] & mk){
                if((s->cstdata[indx] ^ vl) & mk){
                  // Conflict Here!!! UNSAT!!
                  printf("%i constant propogation passes\n", passes);
                  return (indx * 64) + pick;
                }
              }else{
                // Mark constant
                s->cstmask[indx] |= mk;
                s->cstdata[indx] |= vl;
                cstct++;
                csts++;
              }
            }
          }
          next = next->xnext;
        }
      }
    }
  }

  printf("%i constant propogation passes\n", passes);

  printf("%i constants found!\n", csts);

  return 0;
}
