#include <stdio.h>
#include <stdlib.h>

#include "ual_low_level.h"
#include "ual_const.h"

#define SIZE 1000

int main(int argc, char **argv)
{
  int pctx=0, octx=0, status=0, i;
  double *data;
  int ival;
  double dval;

  const char * cponame="nonTimedCPO";
  //const char * cponame="coreprof";

  double _Complex comp = 1.2 + 0.5*I;

  data = malloc(SIZE*sizeof(double));
  for (i=0; i<SIZE; i++)
    data[i] = 10.1 * i;

  status = euitm_create("euitm", 123, 2, 0, 0, &pctx);
  check_status(status,LOG);
  printf("pctx = %d\n",octx);
    
  octx = beginCPOPutNonTimed(pctx, cponame);
  check_status(octx,LOG);
  printf("octx = %d\n",octx);

  status = putVect1DDouble(octx, "path/to/1Ddouble", data, 1, SIZE);
  check_status(status,LOG);

  status = putComplex(octx, "path/to/complex", comp);
  check_status(status,LOG);

  status = endCPOPutNonTimed(octx);
  check_status(status,LOG);

  octx = beginCPOGet(pctx, cponame, NON_TIMED);
  check_status(octx,LOG);
  printf("octx = %d\n",octx);
  
  status = getDouble(octx, "path/to/double", &dval);
  check_status(status,LOG);
  printf("try to read dval...");
  printf("dval = %f\n",dval);

  status = getInt(octx, "path/to/int", &ival);
  check_status(status,LOG);
  printf("try to read ival...");
  printf("ival = %d\n",ival);

  status = endCPOGet(octx);
  check_status(status,LOG);

  status = euitm_close(pctx);
  check_status(status,LOG);

  return EXIT_SUCCESS;
}
