#include "solver.h"
#include "stdlib.h"
#include "stdio.h"
#include "global.h"










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
  ret.varct   =  cnf->varnum;
  ret.varsz   = (cnf->varnum % 64)? (cnf->varnum / 64)+1 : (cnf->varnum/64);
  ret.clausect=  cnf->clausenum;

  ret.cstdata = malloc(sizeof(uint64_t) * ret.varsz);
  ret.cstmask = malloc(sizeof(uint64_t) * ret.varsz);
  ret.prddata = malloc(sizeof(uint64_t) * ret.varsz);
  for(int i = 0; i < ret.varsz; i++){
    ret.cstdata[i] = 0;
    ret.cstmask[i] = 0;
    ret.prddata[i] = 0;
  }

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

  return ret;
}










void freeSolverState(SOLVERSTATE* s){
  free(s->cstdata);
  free(s->cstmask);
  free(s->prddata);
  free(s->propixs);
  free(s->unsatct);
  free(s->fstsat);
}
