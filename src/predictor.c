#include "predictor.h"
#include "solver.h"
#include "math.h"
#include "util.h"
#include "stdlib.h"
#include "stdio.h"
#include "SDL/SDL.h"


//#define GRAPHDISTRIBUTION
//#define SPREADFORCE










typedef struct{
  float spaceIndex;
  float spaceForce;
  int   clausect;
}MOVEVAR;



/*
  This algorithm tries to move variables around in a space so that they wind up
  closer to variables with shared clauses. Then it places them in an order based
  on where they land in this space.
*/
TRANSLATION reorderVars(CNF* cnf, int passes){
#ifdef GRAPHDISTRIBUTION
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Surface* screen = SDL_SetVideoMode(512, 512, 32, 0);
#endif

  TRANSLATION ret;
  ret.size  = cnf->varnum;
  ret.trans = malloc(sizeof(int) * cnf->varnum);

  MOVEVAR* vars = malloc(sizeof(MOVEVAR) * cnf->varnum);

  for(int i = 0; i < cnf->varnum; i++){
    vars[i].spaceIndex = ((float)i) / cnf->varnum;
    vars[i].spaceForce = 0;
  }

  float blckdist = 128.0 / cnf->varnum;
  float worddist =  32.0 / cnf->varnum;

  int chunknum  = ((cnf->varnum % 256) == 0)? (cnf->varnum / 256) : (cnf->varnum / 256) + 1;
  float* chunks = malloc(sizeof(float) * chunknum);

  for(int iter = 0; iter < passes; iter++){

#ifdef GRAPHDISTRIBUTION
    uint32_t* pix = screen->pixels;

    for(int i = 0; i < 262144; i++){
      pix[i] = 0;
    }
#endif

    /*
      For each clause, get space indices of each referenced var, and push them
      all toward the average for the clause.
      Push harder if the variables are too far apart.
    */
    for(int i = 0; i < cnf->clausenum; i++){
      Clause c = cnf->clauses[i];
      float avg = 0;
      for(int j = 0; j < c.numvars; j++)
        avg += fabsf((float) c.vars[j]);

      if(cnf->clausenum != 0)
        avg /= cnf->clausenum;

      if(c.numvars > 1){
        for(int j = 0; j < c.numvars; j++){
          int index = abs(c.vars[j]);

          MOVEVAR mvar  = vars[index];
          float diff    = (avg - mvar.spaceIndex);
          float epsilon = (diff < worddist)? 0.005 : (diff < blckdist)? 0.007 : 0.009;
          mvar.spaceForce += (avg - mvar.spaceIndex) * epsilon * c.numvars * 0.01;
          if(iter == 0) mvar.clausect++;

          vars[index] = mvar;
        }
      }
    }

    /*
      Get minimum and maximum values, move spaceIndices by spaceForces.
    */

    float min =  HUGE_VALF;
    float max = -HUGE_VALF;

    for(int i = 0; i < cnf->varnum; i++){
      vars[i].spaceIndex += (vars[i].spaceForce / vars[i].clausect);
      min = (vars[i].spaceIndex < min)? vars[i].spaceIndex : min;
      max = (vars[i].spaceIndex > max)? vars[i].spaceIndex : max;
    }

    /*
      Count how many spaceIndices fall into each equal chunk of the space.
    */
    for(int i = 0; i < cnf->varnum; i++){
      int index = ((vars[i].spaceIndex - min) / max) * chunknum;
      vars[i].spaceIndex = (vars[i].spaceIndex - min) / (max - min);
      index = (index < 0)? 0 : (index >= chunknum)? chunknum-1 : index;
      chunks[index] += 1;
    }

    /*
      The distribution may become uneven. Initialize spaceForces for the next
      iteration in a way that evens things out a bit. It seems to make things
      really unstable though.
    */
#ifdef SPREADFORCE
    for(int i = 0; i < cnf->varnum; i++){
      int index = vars[i].spaceIndex * chunknum;
      index = (index < 0)? 0 : (index >= chunknum)? chunknum-1 : index;
      float a, b, c;
      a = (index !=          0)? chunks[index-1] : chunks[index];
      b = chunks[index];
      c = (index != chunknum-1)? chunks[index+1] : chunks[index];

      float bcenter = ((((float)index) + 0.5) / (float)chunknum);
      if(vars[i].spaceIndex > bcenter)
        vars[i].spaceForce = (b - c) * 0.0001;
      else
        vars[i].spaceForce = (b - a) * 0.0001;
    }
#else
    for(int i = 0; i < cnf->varnum; i++) vars[i].spaceForce = 0;
#endif

    /*
    Render?
    */
#ifdef GRAPHDISTRIBUTION
    int ct[512];
    int ixaaa = 0;
    for(int i = 0; i < 512; i++) ct[i] = 0;

    for(int i = 0; i < cnf->varnum; i++){
      int x = (((vars[i].spaceIndex - min) / (max - min)) * 512);
      x = (x < 0)? 0 : (x >= 512)? 511 : x;
      ct[x]++;
      if(i == 455) ixaaa = x;
    }
    int mxct = 0;
    for(int i = 0; i < 512; i++){
      mxct = (ct[i] > mxct)? ct[i] : mxct;
    }
    int pixindex = 0;
    for(int i = 0; i < 512; i++){
      int hgt = 512 * (((float)ct[i]) / ((float)mxct));
      int ixb = 25 * log(mxct);
      for(int j = 0; j < 512; j++){
        pix[pixindex] = (j < 50 * log(hgt))? ((i == ixaaa)? 65535 : 255) : 0;
        if(j == ixb) pix[pixindex] += 0xFF0000;
        pixindex++;
      }
    }

    SDL_Flip(screen);
    //SDL_Delay(60);


    IntPair* pairs = malloc(sizeof(IntPair) * cnf->varnum);
    for(int i = 0; i < cnf->varnum; i++){
      pairs[i].x   = 100000 * vars[i].spaceIndex;
      pairs[i].val = i;
    }
    quicksort(pairs, 0, cnf->varnum);

    for(int i = 0; i < cnf->varnum; i++){
      ret.trans[i] = pairs[i].val;
    }

    //free(pairs);    // This seems to create frequent free() related crashes. The resulting memory leak from not calling it isn't a big deal.

    printf("%i %f\n", iter, translationScore(ret, *cnf));
#endif
  }

  /*
    Sort out the items and make a translation table.
  */
  IntPair* pairs = malloc(sizeof(IntPair) * cnf->varnum);
  for(int i = 0; i < cnf->varnum; i++){
    pairs[i].x   = 100000 * vars[i].spaceIndex;
    pairs[i].val = i;
  }
  quicksort(pairs, 0, cnf->varnum);

  for(int i = 0; i < cnf->varnum; i++){
    ret.trans[i] = pairs[i].val;
  }

#ifdef GRAPHDISTRIBUTION
  SDL_Quit();
  SDL_FreeSurface(screen);
#endif

  free(pairs);
  free(chunks);
  free(vars);
  return ret;
}










float translationScore(TRANSLATION t, CNF cnf){

  float ret = 0;

  for(int i = 0; i < cnf.clausenum; i++){
    uint64_t bits = 0;
    Clause c = cnf.clauses[i];
    for(int j = 0; j < c.numvars; j++){
      bits |= 1 << ((t.trans[abs(c.vars[j])] / 256) % 64);
    }
    int pct = popcount(bits);   // There seems to be a bug somewhere here, where some numbers are getting an extra 32 bits added.
    ret += (pct > c.numvars)? 1 : (float)pct / c.numvars;
  }

  return ret / cnf.clausenum;
}
