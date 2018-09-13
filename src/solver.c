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
  int oldcsts = csts;
  do{
    int col = -1;
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
          diff[j] = (cnsts[j] ^ cell->vals[j]) & (cnstm[j] & cell->mask[j]);

        if(diff[0] | diff[1] | diff[2] | diff[3]){
          // Satisfied
          unsatcts[y] = -1;
        }else{
          unsatcts[y] -= (
              popcount(diff[0]) |
              popcount(diff[1]) |
              popcount(diff[2]) |
              popcount(diff[3]) );
        }
      }
    }

    for(int i = 0; i < c->clausenum; i++){
      if(unsatcts[i] == 1){
        // Find and set the unsatisfied variable.
        for(int j = 0; j < c->clauses[i].numvars; j++){
          if(!ixmask(abs(c->clauses[i].vars[j]), s->cstmask)){
            int32_t n = c->clauses[i].vars[j];
            int32_t b = (n > 0)? 1 : 0;
            // Is the variable already set to a conflicting value?
            if(ixmask(abs(n), s->cstmask) && (ixcmp(abs(n), b, s->cstdata))){
              // Yes it is!! CONFLICT!!
              printf("Conflict found after %i constant propogation passes\n", passes);
              free(unsatcts);
              return abs(n);
            }else{
              ixset(abs(n), 1, s->cstmask);
              ixset(abs(n), b, s->cstdata);
              csts++;
            }
          }
        }

      }else if(unsatcts[i] == 0){
        // CONFLICT!
        printf("Conflict found after %i constant propogation passes\n", passes);
        free(unsatcts);
        return c->clauses[i].vars[0];

      }else if(unsatcts[i] != -1){
        // Reset things.
        unsatcts[i] = c->clauses[i].numvars;
      }
    }

    passes++;
  }while(oldcsts != csts);

  free(unsatcts);

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

  const int SAT0 =  0;
  const int SAT2 = -1;

  for(int i = 0; i < s->clausect; i++){
    s->fstsat[i] = SAT0;
  }

  int satct = 0;
  int prevsatct = 0;
  int cont = 50;
  int passct = 0;
  while(cont){

    prevsatct = satct;
    satct = 0;

    int col = -1;
    uint64_t data[4];
    for(int i = 0; i < t->cellCount; i++){
      if(t->allCells[i].x != col){
        col = t->allCells[i].x;
        data[0] = (((col*4)  ) > s->varsz)? s->cstdata[(col * 4)  ] : 0;
        data[1] = (((col*4)+1) > s->varsz)? s->cstdata[(col * 4)+1] : 0;
        data[2] = (((col*4)+2) > s->varsz)? s->cstdata[(col * 4)+2] : 0;
        data[3] = (((col*4)+3) > s->varsz)? s->cstdata[(col * 4)+3] : 0;
      }

      uint64_t pass = 0;
      for(int j = 0; j < 4; j++)
        pass |= (t->allCells[i].vals[j] ^ data[j]) & t->allCells[i].mask[j];

      if(pass){
        int clauseix = t->allCells[i].y;
        if(s->fstsat[clauseix] == SAT0){
          s->fstsat[clauseix] = col;
        }else if(s->fstsat[clauseix] > SAT0){
          s->fstsat[clauseix] = SAT2;
        }
      }
    }

    for(int i = 0; i < s->clausect; i++){
      satct += (s->fstsat[i] != SAT0)? 1 : 0;
    }

    if(satct == s->clausect){
      cont = 0;
      break;
    }

    for(int i = 0; i < s->varct; i++){
      uint64_t mask = ixbit(i);
      int varix     = i/64;
      int colix     = i/256;
      int wordix    = (i/64)%4;
      if(!ixmask(i, s->cstmask)){  // Is this value non-constant?
        int start = t->varbounds[i].a;
        int end   = t->varbounds[i].b;

        int tct = 0, fct = 0;   // How many clauses are satisfied if var is true, false?
        for(int j = start; j < end; j++){

          // Is there actually any overlap here, or can we just skip this cell?
          if(t->allCells[j].mask[wordix] & mask){
            uint64_t tval = t->allCells[j].vals[wordix] & t->allCells[j].mask[wordix] & mask;
            tval = tval? 1 : 0;
            uint64_t fval = !tval;

            /*
              Is the clause satisfied in multiple places? If so, changing it
              here will do nothing.
            */
            if(s->fstsat[colix] == SAT2){
              tval = 0;
              fval = 0;
            }else if(s->fstsat[colix] != colix){
              tval = 0;
              fval = 0;
            }else{
              tval *= 2;
              fval *= 2;
            }

            tct += tval;
            fct += fval;
          }
        }
        if(tct > fct){
          s->prddata[varix] |=  mask;
        }else if(tct != fct){
          s->prddata[varix] &= ~mask;
        }else{
          s->prddata[varix] ^=  mask;
        }
      }
    }
    if(satct == prevsatct){
      cont = 1;
    }else if((satct - prevsatct) < -10){
      cont -= 2;
    }else if(abs(satct - prevsatct) < 10){
      cont--;
    }

    passct++;
    if(passct > 1000) cont = 0;
  }

  printf("%i out of %i clauses satisfied after %i passes.\n", satct, s->clausect, passct);

  return (satct == s->clausect);
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
