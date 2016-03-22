#include "mdlClassifier.h"

static clock_t _tic, _toc;

void tic()
{
  _tic = clock();
}

void toc()
{
  _toc = clock();
}

double get_elapsed_time()
{
  return ( (double(_toc - _tic) / CLOCKS_PER_SEC) * 1000) ;
}
