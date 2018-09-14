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










inline uint64_t ixbit(int x){
  uint64_t ret = 1;
  return (ret << (x % 64));
}










inline uint64_t ixmask(int x, uint64_t* data){
  uint64_t ret = data[x/64];
  return ret & ixbit(x);
}










inline uint64_t ixcmp(int x, uint64_t v, uint64_t* data){
  uint64_t ret = data[x/64];
  uint64_t val = v << (x%64);
  return ret & ixbit(x) & val;
}










inline uint64_t ixdmask(int x, uint64_t delta, uint64_t* data){
  uint64_t ret = data[x/64];
  return ret & (ixbit(x) ^ delta);
}










inline void ixset(int x, int v, uint64_t* data){
  uint64_t m0 =  (uint64_t)1 << (x%64);
  uint64_t m1 = ((uint64_t)v << (x%64)) & m0;

  data[x/64]  = (data[x/64] & (~m0)) | m1;
}










inline void dump4(int ix, uint64_t* from, uint64_t* to){
  to[0] = from[ix  ];
  to[1] = from[ix+1];
  to[2] = from[ix+2];
  to[3] = from[ix+3];
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










// Assumes v is a number with exactly one nonzero bit. Returns the index of that bit.
// Undefined for everything else.
inline int fastbix(uint64_t v){
    float f = (float)v;
    return (*(uint32_t *)&f >> 23) - 0x7f;
}









// Returns -1 if all clauses are satisfied
// Returns  0 if there are clauses left
// Returns  N if there is a conflict on N.
int getconstants(SOLVERSTATE* s, CNF* c, TABLE* t){

  int csts = 0;
  for(int i = 0; i < s->varsz; i++){
    s->cstdata[i] = 0;
    s->cstmask[i] = 0;
  }

  for(int i = 0; i < c->clausenum; i++){

    // Does this clause have only a single variable?
    if(c->clauses[i].numvars == 1){
      // Yes it does. Let's try to make it a constant.
      s->unsatct[i] = 0;
      int cstx = c->clauses[i].vars[0];
      int csti = abs(cstx);
      uint64_t mk = ixbit(csti);
      uint64_t vl = (cstx < 0)? 0 : mk;

      // Is there already a conflicting constant here?
      if(ixmask(csti, s->cstmask)){
        if(ixmask(csti, s->cstdata) ^ vl){
          // Conflict Here!!! UNSAT!!
          printf("0 constant propogation passes\n");
          printf("%i constants found\n", csts);
          return csti;
        }
      }else{
        // Mark constant
        s->cstmask[csti/64] |= mk;
        s->cstdata[csti/64] |= vl;
        csts++;
      }
    }
  }

  if(csts == 0){
    // No constant propagation will occur with no constants.
    printf("0 constant propogation passes\n");
    printf("%i constants found\n", csts);
    return 0;
  }




  // Let's try constant propagation

  IX* unsatcts = malloc(sizeof(IX) * s->clausect);
  for(int i = 0; i < c->clausenum; i++){
    // unsatcts will store how many unsatisfied variables are in each clause.
    // Set it to -1 if the corresponding clause is satisfied.
    unsatcts[i] = c->clauses[i].numvars;
  }

  int passes = 0;
  int oldcsts;
  int unsatclauses = s->clausect;
  int checkpass = 0;
  int checkpasses = 0;
  do{
    if(checkpass){
      checkpasses++;
    }else{
      checkpasses = 0;
    }
    int col = -1;
    oldcsts = csts;
    checkpass = 0;
    uint64_t cnsts[4], cnstm[4];
    for(int i = 0; i < t->cellCount; i++){
      TABLECELL* cell = &t->allCells[i];
      if(cell->x != col){
        col = cell->x;
        dump4(col, s->cstdata, cnsts);
        dump4(col, s->cstmask, cnstm);
      }
      // Any satisfied clauses? If so, mark them as -1.
      // Else, subtract the number of unsat vars.
      int y = cell->y;
      if(unsatcts[y] != -1){
        uint64_t diff[4];
        for(int j = 0; j < 4; j++)
          diff[j] = (cnsts[j] ^ ~cell->vals[j]) & (cnstm[j] & cell->mask[j]);

        if(diff[0] | diff[1] | diff[2] | diff[3]){
          // Satisfied
          unsatcts[y] = -1;
          unsatclauses--;
        }else{
          int xval = (
              popcount(cnstm[0] & cell->mask[0]) +
              popcount(cnstm[1] & cell->mask[1]) +
              popcount(cnstm[2] & cell->mask[2]) +
              popcount(cnstm[3] & cell->mask[3]) );
          unsatcts[y] -= xval;
        }
      }
    }

    // Iterate over clauses, check if constant propagation is possible.
    // Otherwise, just reset any unsatct values for unsatisfied clauses.
    for(int i = 0; i < c->clausenum; i++){


      if(unsatcts[i] == 1){

        /*
          Okay, this gets complicated. If multiple clauses all try to do
          constant propagation on the same variable during the same pass, we run
          into issues here. If this happens, we run an extra "check pass", and
          set this particular
        */

        // Find and set the unsatisfied variable.
        int constspropagated = 0;
        for(int j = 0; j < c->clauses[i].numvars; j++){
          // Is this var unsatisfied?
          if(!ixmask(abs(c->clauses[i].vars[j]), s->cstmask)){
            int32_t n = c->clauses[i].vars[j];
            int32_t b = (n > 0)? 1 : 0;
            // Is the variable already set to a conflicting value?
            if(ixmask(abs(n), s->cstmask) && (!ixcmp(abs(n), b, s->cstdata))){
              // Yes it is!! CONFLICT!!
              printf("Conflict found after %i constant propogation passes\n", passes);
              free(unsatcts);
              return abs(n);
            }else{
              constspropagated++;
              ixset(abs(n), 1, s->cstmask);
              ixset(abs(n), b, s->cstdata);
              unsatcts[i] = -1;
              csts++;
            }
          }
        }
        if(constspropagated == 0){
            // There were some overlapping constants this pass it seems.
            checkpass = 1;
            unsatcts[i] = c->clauses[i].numvars;
        }

      }else if(unsatcts[i] == 0){
        // CONFLICT!
        printf("Conflict found after %i constant propogation passes\n", passes);
        free(unsatcts);
        return abs(c->clauses[i].vars[0]);

      }else if(unsatcts[i] != -1){
        // Reset things.
        unsatcts[i] = c->clauses[i].numvars;
      }else if((unsatcts[i] < -1) || (unsatcts[i] >= s->varct)){
        printf("THERE IS A BUG IN getconstants()!! CRITICAL FAILURE!!\n");
        exit(-1);
      }
    }

    passes++;
  }while((checkpass && (checkpasses < 3)) || ((unsatclauses != 0) && (oldcsts != csts)));

  free(unsatcts);

  printf("%i constant propogation passes\n", passes);

  printf("%i constants found!\n", oldcsts);

  return unsatclauses? 0 : -1;
}












void approximator(SOLVERSTATE* s, CNF* c, TABLE* t){
  /*
    This is where the real performance comes from. A fast, highly parallelizable
    algorithm for approximating SAT. Results are used for predictions.
  */
}









int fullSolver(SOLVERSTATE* s, CNF* c, TABLE* t){


  int varix = 1;
  int lowerfail = 0; // If we have to backtrack before here, we know it's unsat.
  while(varix <= c->varnum){

    /*
      Basically, a depth-first search on the space, with backtracking on
      conflicts and unit prop. Maybe some CDCL too, we'll see.
    */

  }

  return 0;
}
