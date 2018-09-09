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










// Assumes v is a number with exactly one nonzero bit. Returns the index of that bit.
// Undefined for everything else.
inline int fastbix(uint64_t v){
    float f = (float)v;
    return (*(uint32_t *)&f >> 23) - 0x7f;
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
          printf("%i constants found\n", csts);
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
    printf("%i constants found\n", csts);
    return 0;
  }

  IX* satixs = malloc(sizeof(IX) * s->varsz);
  const IX NOSAT    =  0;
  const IX MULTISAT = -1;

  int cstdelta;
  int passes = 0;
  do{
    cstdelta = 0;

    for(int i = 0; i < s->varct; i++)
      s->unsatct[i] = c->clauses[i].numvars;

    int col = -1;
    uint64_t cmask[4];
    uint64_t cdata[4];

    for(int i = 0; i < t->cellCount; i++){

      TABLECELL* cl = &(t->allCells[i]);
      int x = cl->x, y = cl->y;

      if(cl->x != col){
        col = cl->x;
        cmask[0] = s->cstmask[(4*col)  ];
        cmask[1] = s->cstmask[(4*col)+1];
        cmask[2] = s->cstmask[(4*col)+2];
        cmask[3] = s->cstmask[(4*col)+3];

        cdata[0] = s->cstdata[(4*col)  ];
        cdata[1] = s->cstdata[(4*col)+1];
        cdata[2] = s->cstdata[(4*col)+2];
        cdata[3] = s->cstdata[(4*col)+3];
      }

      // Are there any constants here?
      if(cmask[0] | cmask[1] | cmask[2] | cmask[3]){
        // Is clause currently unsatisfied?
        if(!(s->satclause[y/64] & ((uint64_t)1 << (y%64)))){
          // Maybe. Check if recent changes have satisfied it.
          uint64_t issat = ((cdata[0] ^ ~cl->vals[0]) & cmask[0] & cl->mask[0])
                         | ((cdata[1] ^ ~cl->vals[1]) & cmask[1] & cl->mask[1])
                         | ((cdata[2] ^ ~cl->vals[2]) & cmask[2] & cl->mask[2])
                         | ((cdata[3] ^ ~cl->vals[3]) & cmask[3] & cl->mask[3]);
          if(issat){
            s->satclause[y/64] |= ((uint64_t)1 << (y%64)); // Yes? Record it.
          }else{
            // Nope. Let's adjust unsatct
            int unsat = popcount((cdata[0] ^ cl->vals[0]) & cmask[0] & cl->mask[0])
                      + popcount((cdata[1] ^ cl->vals[1]) & cmask[1] & cl->mask[1])
                      + popcount((cdata[2] ^ cl->vals[2]) & cmask[2] & cl->mask[2])
                      + popcount((cdata[3] ^ cl->vals[3]) & cmask[3] & cl->mask[3]);
            s->unsatct[y] -= unsat;
          }
        }
      }
    }

    col = -1;

    for(int i = 0; i < t->cellCount; i++){
      TABLECELL* cl = &(t->allCells[i]);
      int x = cl->x, y = cl->y;

      if(s->unsatct[i] == 0){
        // UNSAT!! Find a constraint to claim is conflicting.
        printf("0 constant propogation passes\n");
        printf("%i constants found\n", csts);
        int var = 0;
        free(satixs);
        for(int i = 0; i < 256; i++){
          if((cl->mask[i/64] >> (i%64)) & 1){
            return i + (256 * x);
          }
        }
      }else if(s->unsatct[i] == 1){
        // Somewhere in this clause there is a single, untouched constraint.
        // If it's here, we set it as a new constant and set unsatct[i] to 0.
        // Otherwise, we move on.

        // Generate constant masks if necessary.
        if(cl->x != col){
          col = cl->x;
          cmask[0] = s->cstmask[(4*col)  ];
          cmask[1] = s->cstmask[(4*col)+1];
          cmask[2] = s->cstmask[(4*col)+2];
          cmask[3] = s->cstmask[(4*col)+3];
        }

        uint64_t pick[4];
        pick[0] = (~cmask[0] & cl->mask[0]);
        pick[1] = (~cmask[1] & cl->mask[1]);
        pick[2] = (~cmask[2] & cl->mask[2]);
        pick[3] = (~cmask[3] & cl->mask[3]);

        // Is the remaining result here?
        if(pick[0] | pick[1] | pick[2] | pick[3]){
          // Yes! Let's add a new constant!
          s->cstmask[(y/64)  ] |= pick[0];
          s->cstmask[(y/64)+1] |= pick[1];
          s->cstmask[(y/64)+2] |= pick[2];
          s->cstmask[(y/64)+3] |= pick[3];

          s->cstdata[(y/64)  ] |= pick[0] & cl->vals[0];
          s->cstdata[(y/64)+1] |= pick[1] & cl->vals[1];
          s->cstdata[(y/64)+2] |= pick[2] & cl->vals[2];
          s->cstdata[(y/64)+3] |= pick[3] & cl->vals[3];
          cstdelta++;
        }
      }
    }

    passes++;
    csts += cstdelta;
  }while(cstdelta != 0);

  free(satixs);

  printf("%i constant propogation passes\n", passes);

  printf("%i constants found!\n", csts);

  return 0;
}












int approximator(SOLVERSTATE* s, CNF* c, TABLE* t){
  /*
    This is where the real performance comes from. A fast, highly parallelizable
    algorithm for approximating SAT. Results are used for predictions. If an
    actual satisfying state is found, return 1. Otherwise, return 0 and move
    onto the full solver.
  */
  int cont = 1;
  while(cont){

    for(int i = 0; i < s->varct; i++){
      uint64_t mask = (uint64_t)1 << (i%64);
      int varix     = i/64;
      int colix     = i/256;
      int wordix    = (i/64)%4;
      if(!(s->cstmask[varix] & mask)){  // Is this value non-constant?
        int start = t->columnixs[i];
        int end   = ((i+1) >= t->cols)? t->cellCount : t->columnixs[i+1];

        int tct = 0, fct = 0;   // How many clauses are satisfied if var is true, false?
        for(int i = start; i < end; i++){   // Adjust this; it can be done a bit smarter w/ varbounds.

          uint64_t tval = t->allCells[i].vals[wordix] & t->allCells[i].mask[wordix] & mask;
          tval = tval? 1 : 0;
          uint64_t fval = 1 - tval;

          tct += tval;
          fct += fval;
        }
        if(tct > fct){
          s->prddata[varix] |=  mask;
        }else{
          s->prddata[varix] &= ~mask;
        }
      }
    }
  }

  return 0;
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
