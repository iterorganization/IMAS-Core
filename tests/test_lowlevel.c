#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ual_low_level.h"
#include "ual_lowlevel.h"
char * euitm_last_errmsg(){return "";}
int main(int argc, char *argv[])
{
  int ctx, opCtx;
  int status; 
  const char *codeName="CODE_NAME_XXX";
  int dims[8], retDims[8], i, j;
  dims[0] = 0;
  int intData;
  void *retData;
  double complex comp, retComp;
  
  status = euitm_create("euitm", 123, 0, 0, 0, &ctx);
  if(status)
    {
      printf("create: %s\n", euitm_last_errmsg());
      return 0;
    }
  
  printf("Read/Write an integer value in codeparam/codeversion\n");
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  
  status = putInt(opCtx, "codeparam/codeversion", 123);   
  if(status)
    {
      printf("putInt: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  
  printf("Written 123 in coreprof/codeparam....");
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getInt(opCtx, "codeparam/codeversion", &intData);
  if(status)
    {
      printf("getInt: %s\n", euitm_last_errmsg());
      return 0;
    }
  printf("Read %d in coreprof/codeversion....", intData);
  if(intData == 123)
    printf("   -------------> OK\n");
  else
    {
      printf("   -------------> FAIL\n");
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  printf("\n\nRead/Write string value in codeparam/codename\n");
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DChar(opCtx, "codeparam/codename", (char *)codeName, strlen(codeName), 0);
  if(status)
    {
      printf("putVect1DChar: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  printf("Written %s in codeparam/codename....", codeName);
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  int dim;
  status = getVect1DChar(opCtx, "codeparam/codename", 0, (char **)&retData, &dim);
  if(status)
    {
      printf("getVect1DChar: %s\n", euitm_last_errmsg());
      return 0;
    }
  printf("Read %s in codeparam/codename....", (char *)retData);
  if(!strncmp(retData, codeName, dim))
    printf("   -------------> OK\n");
  else
    {
      printf("   -------------> FAIL\n");
      return 0;
    }
  free(retData);
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\n\nRead/Write a complex value in coreprof/codeparam/parameters\n");
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  comp = 1 + 2*I;
  status = putComplex(opCtx, "codeparam/parameters", comp);   
  if(status)
    {
      printf("putComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  printf("Written 1 + 2i in coreprof/codeparam/parameters....");
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getComplex(opCtx, "codeparam/parameters", &retComp);
  if(status)
    {
      printf("getComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  printf("Read %f + %fi in coreprof/codeparam/parameters....", creal(retComp), cimag(retComp));
  if(comp == retComp)
    printf("   -------------> OK\n");
  else
    {
      printf("   -------------> FAIL\n");
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
    
    
  printf("\n\nRead/write a 1D double array (non time dependent)in coreprof/drho_dt\n"); 
  double doubleArr[3000], *retDoubleArr;
    
  for(int i = 0; i < 10; i++)
    doubleArr[i] = i;
  dims[0] = 10;
  printf("Written ");
  for(i = 0; i < 10; i++)
    printf("%f ", doubleArr[i]);

  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DDouble(opCtx, "drho_dt", doubleArr, 10, 0);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DDouble(opCtx, "drho_dt", 0, &retDoubleArr, retDims);
  if(status)
    {
      printf("getVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d]", retDims[0]);
  for(i = 0; i < retDims[0]; i++)
    printf("%f ", retDoubleArr[i]);

  for(i = 0; i < retDims[0]; i++)
    {
      if(retDoubleArr[i] != doubleArr[i])
	{
	  printf("   -------------> FAIL\n");
	  exit(0);
	}
    }
  printf("   -------------> OK\n");
  free(retDoubleArr);

 
  printf("\n\nRead/write a 1D complex array (non time dependent)in coreprof/drho_dt\n");
  double complex complexArr[3000], *retComplexArr;
  printf("Written ");
  for(i = 0; i < 10; i++)
    {
      complexArr[i] = i + I * (i+0.1);
      printf("%f+i%f ", creal(complexArr[i]), cimag(complexArr[i]));
    }
    
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DComplex(opCtx, "drho_dt", complexArr, 10, 0);
  if(status)
    {
      printf("putVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DComplex(opCtx, "drho_dt", 0, &retComplexArr, retDims);
  if(status)
    {
      printf("getVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d]", retDims[0]);
  for(i = 0; i < retDims[0]; i++)
    printf("%f+i%f ", creal(retComplexArr[i]), cimag(retComplexArr[i]));

  for(i = 0; i < retDims[0]; i++)
    {
      if(retComplexArr[i] != complexArr[i])
	{
	  printf("   -------------> FAIL\n");
	  exit(0);
	}
    }
  printf("   -------------> OK\n");
  free(retComplexArr);

   
  printf("\n\nRead/write a 2D double array (non time dependent) in coreprof/drho_dt\n"); 
  double double2DArr[2][5];
  printf("Written [");
  for(i = 0; i < 2; i++)
    {
      printf("[");
      for(j = 0; j < 5; j++)
	{
	  double2DArr[i][j] = (i+1)*j;
	  if(j < 4)
	    printf("%f,", double2DArr[i][j]);
	  else
	    printf("%f", double2DArr[i][j]);
	}
      printf("]");
    }
  printf("]\n");
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DDouble(opCtx, "drho_dt", (double *)double2DArr, 2,5, 0);
  if(status)
    {
      printf("putVect2DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  int dim1, dim2;
  status = getVect2DDouble(opCtx, "drho_dt", 0, &retDoubleArr, &dim1, &dim2);
  if(status)
    {
      printf("getVect2DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d][%d] [", dim1, dim2);
  for(i = 0; i < dim1; i++)
    {
      printf("[");
      for(j = 0; j < dim2; j++)
	{
	  if(j < dim2 - 1)
	    printf("%f,", retDoubleArr[i*dim2+j]);
	  else
	    printf("%f", retDoubleArr[i*dim2+j]);
	}
      printf("]");
    }
  for(i = 0; i < dim1; i++)
    {
      for(j = 0; j < dim2; j++)
	{
	  if(retDoubleArr[i*dim2+j] != double2DArr[i][j])
	    {
	      printf("   -------------> FAIL\n");
	      exit(0);
	    }
	}
    }
  printf("   -------------> OK\n");
  free(retDoubleArr);

  
  printf("\n\nRead/write a 2D complex array (non time dependent) in coreprof/drho_dt\n");
  double complex complex2DArr[2][5];
  printf("Written [");
  for(i = 0; i < 2; i++)
    {
      printf("[");
      for(j = 0; j < 5; j++)
	{
	  complex2DArr[i][j] = (i+1)*j + I*((i+1)*j + .1);
	  if(j < 4)
	    printf("%f+i%f,", (double)(i+1)*j, (double)(i+1)*j + .1);
	  else
	    printf("%f+i%f", (double)(i+1)*j, (double)(i+1)*j + .1);
	}
      printf("]");
    }
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DComplex(opCtx, "drho_dt", (double _Complex *)complex2DArr, 2,5, 0);
  if(status)
    {
      printf("putVect2DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect2DComplex(opCtx, "drho_dt", 0, &retComplexArr, &dim1, &dim2);
  if(status)
    {
      printf("getVect2DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d][%d] [", dim1, dim2);
  for(i = 0; i < dim1; i++)
    {
      printf("[");
      for(j = 0; j < dim2; j++)
	{
	  if(j < dim2 - 1)
	    printf("%f+i%f,", creal(retComplexArr[i*dim2+j]), cimag(retComplexArr[i*dim2+j]));
	  else
	    printf("%f+i%f", creal(retComplexArr[i*dim2+j]), cimag(retComplexArr[i*dim2+j]));
	}
      printf("]\n");
    }
  for(i = 0; i < dim1; i++)
    {
      for(j = 0; j < dim2; j++)
	{
	  if(retComplexArr[i*dim2+j] != complex2DArr[i][j])
	    {
	      printf("   -------------> FAIL\n");
	      exit(0);
	    }
	}
    }
  printf("   -------------> OK\n");
  free(retComplexArr);

  printf("\n\nRead/Write a time dependent 1D double array in coreprof/profiles1D/pi/value");
  printf("Written [");
  for(i = 0; i < 10; i++)
    {
      doubleArr[i] = i;
      printf("%f ",doubleArr[i]);
    }
  printf("]");
  dim1 = 10;
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DDouble(opCtx, "profiles1D/pi/value", doubleArr, 10, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGet: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DDouble(opCtx, "profiles1D/pi/value", 0, &retDoubleArr, &dim1);
  if(status)
    {
      printf("getVect21DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGet: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf(" Read [");
  for(i = 0; i < dim1; i++)
    printf("%f ", retDoubleArr[i]);
  printf("]");
  for(int i = 0; i < dim1; i++)
    if(retDoubleArr[i] != doubleArr[i])
      printf("--------------> FAIL\n");
  printf("--------------> OK\n");
    
  free(retDoubleArr);
    

  printf("\n\nRead/Write a time dependent 1D complex array in coreprof/profiles1D/pi/value\n");
  printf("Written ");
  for(i = 0; i < 10; i++)
    {
      complexArr[i] = i + I * (i+0.1);
      printf("%f+i%f ", creal(complexArr[i]), cimag(complexArr[i]));
    }
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DComplex(opCtx, "drho_dt", complexArr, 10, 1);
  if(status)
    {
      printf("putVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DComplex(opCtx, "drho_dt", 1, &retComplexArr, retDims);
  if(status)
    {
      printf("getVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGet: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d]", retDims[0]);
  for(i = 0; i < retDims[0]; i++)
    printf("%f+i%f ", creal(retComplexArr[i]), cimag(retComplexArr[i]));

  for(i = 0; i < retDims[0]; i++)
    {
      if(retComplexArr[i] != complexArr[i])
	{
	  printf("   -------------> FAIL\n");
	  exit(0);
	}
    }
  printf("   -------------> OK\n");
  free(retComplexArr);

 
  printf("\n\nRead/Write a time dependent 2D double array in coreprof/profiles1D/pi/value\n");
  printf("Writen [");
  for(i = 0; i < 2; i++)
    {
      printf("[");
      for(j = 0; j < 5; j++)
	{
	  double2DArr[i][j] = i*j;
	  if(j < 4)
	    printf("%f,", double2DArr[i][j]);
	  else
	    printf("%f", double2DArr[i][j]);
	}
      printf("]");
    }
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DDouble(opCtx, "profiles1D/pi/value", (double *)double2DArr, 2,5, 1);
  if(status)
    {
      printf("putVect2DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect2DDouble(opCtx, "profiles1D/pi/value", 1, &retDoubleArr, &dim1, &dim2);
  if(status)
    {
      printf("getVect2DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d][%d] [", dim1, dim2);
  for(i = 0; i < dim1; i++)
    {
      printf("[");
      for(j = 0; j < dim2; j++)
	{
	  if(j < dim2 - 1)
	    printf("%f,", retDoubleArr[i*dim2+j]);
	  else
	    printf("%f", retDoubleArr[i*dim2+j]);
	}
      printf("]");
    }
  for(i = 0; i < dim1; i++)
    {
      for(j = 0; j < dim2; j++)
	{
	  if(retDoubleArr[i*dim2+j] != double2DArr[i][j])
	    {
	      printf("   -------------> FAIL\n");
	      exit(0);
	    }
	}
    }
  printf("   -------------> OK\n");
  free(retDoubleArr);

  printf("\n\nRead/Write a time dependent 2D complex array in coreprof/profiles1D/pi/value\n");
  printf("Written [");
  for(i = 0; i < 2; i++)
    {
      printf("[");
      for(j = 0; j < 5; j++)
	{
	  complex2DArr[i][j] = (i+1)*j + I*((i+1)*j + .1);
	  if(j < 4)
	    printf("%f+i%f,", (double)(i+1)*j, (double)(i+1)*j + .1);
	  else
	    printf("%f+i%f", (double)(i+1)*j, (double)(i+1)*j + .1);
	}
      printf("]");
    }
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DComplex(opCtx, "profiles1D/pi/value", (double _Complex *)complex2DArr, 2,5, 1);
  if(status)
    {
      printf("putVect2DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
   
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect2DComplex(opCtx, "profiles1D/pi/value", 1, &retComplexArr, &dim1, &dim2);
  if(status)
    {
      printf("getVect2DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("\nRead array[%d][%d] [", dim1, dim2);
  for(i = 0; i < dim1; i++)
    {
      printf("[");
      for(j = 0; j < dim2; j++)
	{
	  if(j < dim2 - 1)
	    printf("%f+i%f,", creal(retComplexArr[i*dim2+j]), cimag(retComplexArr[i*dim2+j]));
	  else
	    printf("%f+i%f", creal(retComplexArr[i*dim2+j]), cimag(retComplexArr[i*dim2+j]));
	}
      printf("]\n");
    }
  for(i = 0; i < dim1; i++)
    {
      for(j = 0; j < dim2; j++)
	{int deleteData(int opCtx, const char *fieldPath);

	  if(retComplexArr[i*dim2+j] != complex2DArr[i][j])
	    {
	      printf("   -------------> FAIL\n");
	      exit(0);
	    }
	}
    }
  printf("   -------------> OK\n");
  free(retComplexArr);
    
  printf("\n\nRead/Write a time dependent 1D double array slice per slice in coreprof/profiles1D/pi/value\n");    
    
  //delete first dataint deleteData(int opCtx, const char *fieldPath);
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  for(int i = 0; i < 10; i++)
    doubleArr[i] = i*1000.;
  dims[0] = 0;

  for(i = 0; i < 10; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "profiles1D/pi/value", doubleArr[i]);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\t Written Value: %f\n", (double)i, doubleArr[i]); 
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
    }

  double retDouble;

  for(i = 0; i < 20; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = getDoubleSlice(opCtx, "profiles1D/pi/value", &retDouble);	
      if(status)
	{
	  printf("getDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Value: %f\n", i/2., retDouble); 
      if(((i % 2 == 0 || i == 19) && retDouble != doubleArr[i/2])||
	 ((i % 2 == 1 && i < 19) && retDouble != (doubleArr[i/2]+ doubleArr[i/2+1])/2.))
	{
	  printf("-----------------> FAIL %f %f \n",retDouble, (doubleArr[i/2]+ doubleArr[i/2+1])/2.);
	  exit(0);
	}
    }
  printf("-----------------> OK\n");
    
    
  printf("\n\nRead/Write a time dependent 1D complex array slice per slice in coreprof/profiles1D/pi/value\n");
 
    
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  for(int i = 0; i < 10; i++)
    {
      complexArr[i] = i*1000. + I * (i*1000. + 0.1);
    }
  dims[0] = 10;

  for(i = 0; i < 10; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putComplexSlice(opCtx, "profiles1D/pi/value", complexArr[i]);
      if(status)
	{
	  printf("putComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\t Written Value: (%f + i %f)\n", (double)i, creal(complexArr[i]), cimag(complexArr[i]));
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
    }

  double complex retComplex;
  for(i = 0; i < 20; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = getComplexSlice(opCtx, "profiles1D/pi/value", &retComplex);	
      if(status)
	{
	  printf("getComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Value: (%f+ i %f)\n", i/2., creal(retComplex), cimag(retComplexArr[i])); 
      if((i < 19 && i % 2 == 0 && retComplex != complexArr[i/2])||
	 (i < 19 && i % 2 == 1&& retComplex != (complexArr[i/2]+ complexArr[i/2+1])/2.)
	 ||(i == 19 && retComplex != complexArr[9] ))
	{
	  if(i % 2 == 0)
	    printf("-----------------> FAIL (%f+i%f) (%f+i%f)f \n",creal(retComplex), 
		   cimag(retComplex), creal(complexArr[i/2]), cimag(complexArr[i/2]));
	    
	  else  
	    printf("-----------------> FAIL (%f+i%f) (%f+i%f)f \n",creal(retComplex), 
		   cimag(retComplex), creal((complexArr[i/2]+ complexArr[i/2+1])/2.), cimag((complexArr[i/2]+ complexArr[i/2+1])/2.));
	  exit(0);
	}
    }
  printf("-----------------> OK\n");
    
    
    
  printf("\n\nRead/Write a time dependent 2D double array slice per slice in coreprof/profiles1D/pi/value\n"); 
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  for(int i = 0; i < 2; i++)
    for(int j = 0; j < 5; j++)
      double2DArr[i][j] = (1+i)*j;
    
  for(i = 0; i < 2; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putVect1DDoubleSlice(opCtx, "profiles1D/pi/value", double2DArr[i], 5);
      if(status)
	{
	  printf("putVect1DDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\t Written Value: [", (double)i);
      for(int j = 0; j < 5; j++)
	printf("%f ", double2DArr[i][j]);;
      printf("]\n");

    }
  for(i = 0; i < 4; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = getVect1DDoubleSlice(opCtx, "profiles1D/pi/value", &retDoubleArr, retDims);	
      if(status)
	{
	  printf("getVect1DDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Values: [", i/2.);
      for(int j = 0; j < 5; j++)
	printf("%f ", retDoubleArr[j]);
      printf("]\n");

      for(int j = 0; j < retDims[0]; j++)
	{
	  if(((i % 2 == 0 || i == 3) && retDoubleArr[j] != double2DArr[i/2][j])||
	     ((i % 2 == 1 && i < 3) && retDoubleArr[j] != (double2DArr[i/2][j]+ double2DArr[i/2+1][j])/2.))
	    {
	      printf("-----------------> FAIL\n");
	      exit(0);
	    }
	}
      free(retDoubleArr);
    }
 
  printf("----------------------> OK\n");
    
    
  printf("\n\nRead/Write a time dependent 2D complex array slice per slice in coreprof/profiles1D/pi/value\n"); 
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
 
  /*     double complex complex2DArray[2][5]; */
  /*     for(int i = 0; i < 2; i++) */
  /*       for(int j = 0; j < 5; j++) */
  /*       { */
  /* 	complex2DArray[i][j] = (1+i)*j + I * ((1+i)*j + 0.1); */
  /*       } */
  dims[0] = 5;

    
  for(int i = 0; i < 2; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putVect1DComplexSlice(opCtx, "profiles1D/pi/value", complex2DArr[i], 5);
      if(status)
	{
	  printf("putVect1DComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\t Written Value: [", (double)i);
      for(int j = 0; j < 5; j++)
	printf("(%f+i%f) ", creal(complex2DArr[i][j]), cimag(complex2DArr[i][j]));
      printf("]\n");

    }

    
  for(int i = 0; i < 4; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = getVect1DComplexSlice(opCtx, "profiles1D/pi/value", &retComplexArr, retDims);	
      if(status)
	{
	  printf("getVect1DComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Values: [", i/2.);
      for(int j = 0; j < 5; j++)
	printf("(%f+i%f) ", creal(retComplexArr[j]), cimag(retComplexArr[j]));;
      printf("]\n");

      for(int j = 0; j < 5; j++)
	{
	  if(((i % 2 == 0 || i == 3) && retComplexArr[j] != complex2DArr[i/2][j])||
	     ((i % 2 == 1 && i < 3) && cabs(retComplexArr[j] - (complex2DArr[i/2][j]+ complex2DArr[i/2+1][j])/2.) > 1E-10))
	    {
	      printf("-----------------> FAIL\n") ;
	      exit(0);
	    }
	}
      free(retComplexArr);

    }

  printf("----------------------> OK\n");
    

  printf("\n\nWrite a large time dependent double array in coreprof/profiles1D/pi/value slice by slice and read it both slice per slice and via a single get\n");
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    

    
  dims[0] = 0;
  double currDouble;
  for(i = 0; i < 3000; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      currDouble = i*100.;
      status = putDoubleSlice(opCtx, "profiles1D/pi/value", currDouble);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
    }
  for(int i = 0; i < 30; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double retDouble;
      status = getDoubleSlice(opCtx, "profiles1D/pi/value", &retDouble);	
      if(status)
	{
	  printf("getDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Value: %f   Expected value: %f\n", (double)i/2., retDouble, (double)100*i/2); 

      if(retDouble != 100 * i/2.)
	{
	  printf("--------------------> FAIL\n");
	  exit(0);
	}
    }
  printf("...\n");
  for(i = 5980; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double retDouble;
      status = getDoubleSlice(opCtx, "profiles1D/pi/value", &retDouble);	
      if(status)
	{
	  printf("getDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 5999)
	printf("Time: %f\tRead Value: %f   Expected value: %f\n", i/2., retDouble, 100*i/2.); 
      else
	printf("Time: %f\tRead Value: %f   Expected value: %f\n", i/2., retDouble, 100*5998/2.); 
      if((i < 5999 && retDouble != 100 * i/2) || (i >= 5999 && retDouble != 100*5998/2 ))
	{
	  printf("--------------------> FAIL\n");
	  exit(0);
	}
    }
    
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DDouble(opCtx, "profiles1D/pi/value", 1, &retDoubleArr, retDims);
  if(status)
    {
      printf("getVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("Read array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("%f ", retDoubleArr[i]);
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("%f ", retDoubleArr[i]);
  printf("]\n");
  printf("Expected array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("%f ", i * 100.);
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("%f ",  i * 100. );
  printf("]\n");
   
  for(i = 0; i < 3000; i++)
    {
      if(retDoubleArr[i] != i*100)
	{
	  printf("--------------------> FAIL\n");;
	  exit(0);
	}
    }
  free(retDoubleArr);

  printf("--------------------> OK\n");
    
  printf("\n\nWrite a large time dependent int array in coreprof/profiles1D/pi/value slice by slice and read it both slice per slice and via a single get\n");
   
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    

    
  dims[0] = 0;
  for(i = 0; i < 3000; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      int currInt = i*100.;
      status = putIntSlice(opCtx, "profiles1D/pi/value", currInt);
      if(status)
	{
	  printf("putIntSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
    }
  for(int i = 0; i < 30; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      int retInt;
      status = getIntSlice(opCtx, "profiles1D/pi/value", &retInt);	
      if(status)
	{
	  printf("getIntSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Value: %d   Expected value: %d\n", (double)i/2., retInt, 100*i/2); 

      if(retInt != 100 * i/2.)
	{
	  printf("--------------------> FAIL\n");
	  exit(0);
	}
    }
  printf("...\n");
  for(i = 5980; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      int retInt;
      status = getIntSlice(opCtx, "profiles1D/pi/value", &retInt);	
      if(status)
	{
	  printf("getSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 5999)
	printf("Time: %f\tRead Value: %d   Expected value: %d\n", i/2., retInt, 100*i/2); 
      else
	printf("Time: %f\tRead Value: %d   Expected value: %d\n", i/2., retInt, 100*5998/2); 
      if((i < 5999 && retInt != 100 * i/2) || (i >= 5999 && retInt != 100*5998/2 ))
	{
	  printf("--------------------> FAIL\n");
	  exit(0);
	}
    }
    
  int *retIntArr;
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DInt(opCtx, "profiles1D/pi/value", 1, &retIntArr, retDims);
  if(status)
    {
      printf("getVect1DInt: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("Read array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("%d ", retIntArr[i]);
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("%d ", retIntArr[i]);
  printf("]\n");
  printf("Expected array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("%d ", i * 100);
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("%d ",  i * 100);
  printf("]\n");
   
  for(i = 0; i < 3000; i++)
    {
      if(retIntArr[i] != i*100)
	{
	  printf("--------------------> FAIL\n");;
	  exit(0);
	}
    }
  free(retIntArr);

  printf("--------------------> OK\n");
   
  printf("\n\nWrite a large time dependent complex array in coreprof/profiles1D/pi/value slice by slice and read it both slice per slice and via a single get\n");
    
    
  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    

    
  dims[0] = 0;
  for(i = 0; i < 3000; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double complex currComplex = i*100. + I * (i*100. + 0.1);
      status = putComplexSlice(opCtx, "profiles1D/pi/value", currComplex);
      if(status)
	{
	  printf("putComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOPutSlice(opCtx);
      if(status)
	{
	  printf("endCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
    }
  for(int i = 0; i < 30; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double complex retComplex;
      status = getComplexSlice(opCtx, "profiles1D/pi/value", &retComplex);	
      if(status)
	{
	  printf("getComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      printf("Time: %f\tRead Value: (%f+i%f)   Expected value: (%f+i%f)\n", (double)i/2., creal(retComplex),
	     cimag(retComplex), i*100./2., (i*100./2. + 0.1));

      if(cabs(retComplex - (100 * i/2.+I*(i*100./2. + 0.1))) > 1E-10)
	{
	  printf("--------------------> FAIL\n");
	  exit(0);
	}
    }
  printf("...\n");
  for(i = 5980; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double complex retComplex;
      status = getComplexSlice(opCtx, "profiles1D/pi/value", &retComplex);	
      if(status)
	{
	  printf("getComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 5999)
	printf("Time: %f\tRead Value: (%f+i%f)   Expected value: (%f+i%f)\n", (double)i/2., creal(retComplex),
	       cimag(retComplex), i*100./2., (i*100./2. + 0.1));
      else
	printf("Time: %f\tRead Value: (%f+i%f)   Expected value: (%f+i%f)\n", (double)i/2., creal(retComplex),
	       cimag(retComplex), 5998*100./2, (5998*100./2 + 0.1));
      /*       if((i < 5999) && (cabs(retComplex - (100 * i/2. + I * (100 * i/2.+0.1))) > 1E-10) || (i >= 5999 && cabs(retComplex != 100*5998/2. + I * (100*5998/2.+0.1) ) > 1E-10)) */
      /*       { */
      /* 	printf("--------------------> FAIL  (%f+i%f)\n", creal(retComplex), cimag(retComplex)); */
      /* 	exit(0); */
      /*       } */
    }
      
  //retComplexArr;
  opCtx = beginCPOGet(ctx, "coreprof", 1);
  if(opCtx < 0)
    {
      printf("beginCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = getVect1DComplex(opCtx, "profiles1D/pi/value", 1, &retComplexArr, retDims);
  if(status)
    {
      printf("getVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGetNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("Read array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("%f+i%f ", creal(retComplexArr[i]), cimag(retComplexArr[i]));
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("%f+i%f ", creal(retComplexArr[i]), cimag(retComplexArr[i]));
  printf("]\n");
  printf("Expected array[%d]", retDims[0]);
  for(i = 0; i < 10; i++)
    printf("(%f+i%f) ", i * 100., i * 100. + 0.1);
  printf("...\n");
  for(i = 2990; i < 3000; i++)
    printf("(%f+i%f) ", i * 100., i * 100. + 0.1);
  printf("]\n");
   
  for(i = 0; i < 3000; i++)
    {
      if(cabs(retComplexArr[i] - (i*100 + I * (i*100. + 0.1))) > 1E-10)
	{
	  printf("--------------------> FAIL\n");;
	  exit(0);
	}
    }
  free(retComplexArr);

  printf("--------------------> OK\n");
    
 
  printf("\n\nWrite a large time dependent double array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  double times[3000];  
   
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      doubleArr[i] = i*10;
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DDouble(opCtx, "profiles1D/pi/value", doubleArr, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double retDouble;
      status = getDoubleSlice(opCtx, "profiles1D/pi/value", &retDouble);	
      if(status)
	{
	  printf("getDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: %f   Expected value: %f\n", (double)i/2., retDouble, (double)10*i/2.); 
	}
      if((i < 5999 && retDouble != 10 * i/2.)||(i >= 5999 && retDouble != 10 * 2999))
	{
	  printf("--------------------> FAIL  %f  %f \n", retDouble, 10 * i/2.);
	  exit(0);
	}
    }
  printf("--------------------> OK\n");
    
 
  printf("\n\nWrite a large time dependent int array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  int intArr[3000];
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      intArr[i] = i*10;
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DInt(opCtx, "profiles1D/pi/value", intArr, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      int retInt;
      status = getIntSlice(opCtx, "profiles1D/pi/value", &retInt);	
      if(status)
	{
	  printf("getIntSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: %d   Expected value: %d\n", (double)i/2., retInt, 10*i/2); 
	}
      if((i < 5999 && retInt != 10 * i/2)||(i >= 5999 && retInt != 10 * 2999))
	{
	  printf("--------------------> FAIL  %d  %d \n", retInt, 10 * i/2);
	  exit(0);
	}
    }
  printf("--------------------> OK\n");
    
 
  printf("\n\nWrite a large time dependent complex array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      complexArr[i] = i*10 + I * (i * 10.+ 0.1);
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect1DComplex(opCtx, "profiles1D/pi/value", complexArr, 3000, 1);
  if(status)
    {
      printf("putVect1DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      double complex retComplex;
      status = getComplexSlice(opCtx, "profiles1D/pi/value", &retComplex);	
      if(status)
	{
	  printf("getComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: (%f+i%f)   Expected value: (%f+i%f)\n", (double)i/2., creal(retComplex), cimag(retComplex), 
		 10*i/2., 10*i/2.+0.1); 
	}
      if((i < 5999 && cabs(retComplex - (10 * i/2. + I * (10 * i/2. + 0.1) )) > 1E-10)||(i >= 5999 && cabs(retComplex - ( 10 * 2999 + I * ( 10 * 2999 + 0.1))) > 1E-10))
	{
	  printf("--------------------> FAIL  (%f+i%f)  (%f+i%f) \n", creal(retComplex), cimag(retComplex), 10 * i/2., 10 * i/2.+0.1);
	  exit(0);
	}
    }
  printf("--------------------> OK\n");
    
 
  printf("\n\nWrite a large time dependent 2D double array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  double doubleArr2D[3000][10];
   
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      for(j = 0; j < 10; j++)
	doubleArr2D[i][j] = i*10 + j;
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DDouble(opCtx, "profiles1D/pi/value", (double *)doubleArr2D, 3000, 10, 1);
  if(status)
    {
      printf("putVect2DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      //      printf("%d\n", i);
      status = getVect1DDoubleSlice(opCtx, "profiles1D/pi/value", &retDoubleArr, dims);	
      if(status)
	{
	  printf("getVect1DDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(dims[0] != 10)
	{
	  printf("--------------------> Fail  %d  %d\n", retDims[0], 10);
	  exit(0);
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: [ ", (double)i);
	  for(j = 0; j < 10; j++)
	    printf("%f ", retDoubleArr[j]);
	  printf("]\n");
	  printf("Expected Value: [ ");
	  for(j = 0; j < 10; j++)
	    {
	      if(i < 5999)
		printf("%f ", (doubleArr2D[i/2][j] + doubleArr2D[i/2+1][j])/2.);
	      else
		printf("%f ",  doubleArr2D[2999][j]);
	    }  
	  printf("]\n\n");
	}
      if(i < 5999)
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(((i % 2) && retDoubleArr[j] != (doubleArr2D[i/2][j] + doubleArr2D[i/2+1][j])/2.)
		 || (!(i % 2) && retDoubleArr[j] != doubleArr2D[i/2][j]))
		{
		  printf("--------------------> FAIL \n");
		  printf("Time: %f\tRead Value: [ ", (double)i);
		  for(j = 0; j < 10; j++)
		    printf("%f ", retDoubleArr[j]);
		  printf("]\n");
		  printf("Expected Value: [ ");
		  for(j = 0; j < 10; j++)
		    printf("%f ", doubleArr2D[i/2][j]/2.);
		  printf("]\n\n");
		  exit(0);
		}
	    }
	}
      else
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(retDoubleArr[j] != doubleArr2D[2999][j])
		{
		  printf("--------------------> FAIL \n");
		  exit(0);
		}
	    }
	} 
      free((char *)retDoubleArr);

    }
  printf("--------------------> OK\n");
    

  printf("\n\nWrite a large time dependent 2D int array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  int intArr2D[3000][10];
    
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      for(j = 0; j < 10; j++)
	intArr2D[i][j] = i*10 + j;
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DInt(opCtx, "profiles1D/pi/value", (int *)intArr2D, 3000, 10, 1);
  if(status)
    {
      printf("putVect2DInt: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      //      printf("%d\n", i);
      status = getVect1DIntSlice(opCtx, "profiles1D/pi/value", &retIntArr, dims);	
      if(status)
	{
	  printf("getVect1DIntSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(dims[0] != 10)
	{
	  printf("--------------------> Fail  %d  %d\n", retDims[0], 10);
	  exit(0);
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: [ ", (double)i);
	  for(j = 0; j < 10; j++)
	    printf("%d ", retIntArr[j]);
	  printf("]\n");
	  printf("Expected Value: [ ");
	  for(j = 0; j < 10; j++)
	    {
	      if(i < 5999)
	    
		printf("%d ", (i%2)?(int)((intArr2D[i/2][j] + intArr2D[i/2+1][j])/2.):intArr2D[i/2][j]);
	      else
		printf("%d ",  intArr2D[2999][j]);
	    }  
	  printf("]\n\n");
	}
      if(i < 5999)
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(((i % 2) && retIntArr[j] != (intArr2D[i/2][j] + intArr2D[i/2+1][j])/2.)
		 || (!(i % 2) && retIntArr[j] != intArr2D[i/2][j]))
		{
		  printf("--------------------> FAIL \n");
		  printf("Time: %f\tRead Value: [ ", (double)i);
		  for(j = 0; j < 10; j++)
		    printf("%d ", retIntArr[j]);
		  printf("]\n");
		  printf("Expected Value: [ ");
		  for(j = 0; j < 10; j++)
		    printf("%f ", intArr2D[i/2][j]/2.);
		  printf("]\n\n");
		  exit(0);
		}
	    }
	}
      else
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(retIntArr[j] != intArr2D[2999][j])
		{
		  printf("--------------------> FAIL \n");
		  exit(0);
		}
	    }
	}    
      free((char *)retIntArr);
    }
  printf("--------------------> OK\n");
    
   
    
  printf("\n\nWrite a large time dependent 2D complex array in a single put() in coreprof/profiles1D/pi/value read it slice by slice\n");   

  //delete first data 
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "profiles1D/pi/value");
  if(!status) status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
    
  double complex complexArr2D[3000][10];
  for(i = 0; i < 3000; i++)
    {
      times[i] = i;
      for(j = 0; j < 10; j++)
	complexArr2D[i][j] = i*10 + j + I * (i*10 + j + 0.1) ;
    }
  dims[0] = 3000;
    
  opCtx = beginCPOPutTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = putVect2DComplex(opCtx, "profiles1D/pi/value", (double _Complex *)complexArr2D, 3000, 10, 1);
  if(status)
    {
      printf("putVect2DComplex: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = putVect1DDouble(opCtx, "time", times, 3000, 1);
  if(status)
    {
      printf("putVect1DDouble: %s\n", euitm_last_errmsg());
      return 0;
    }

  status = endCPOPutTimed(opCtx);
  if(status)
    {
      printf("endCPOPutTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  for(int i = 0; i < 6002; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i/2., INTERPOLATION);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      //      printf("%d\n", i);
      status = getVect1DComplexSlice(opCtx, "profiles1D/pi/value", &retComplexArr, dims);	
      if(status)
	{
	  printf("getVect1DComplexSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      status = endCPOGetSlice(opCtx);
      if(status)
	{
	  printf("endCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      if(dims[0] != 10)
	{
	  printf("--------------------> Fail  %d  %d\n", retDims[0], 10);
	  exit(0);
	}
      if(i < 30 || i > 5980)
	{
	  printf("Time: %f\tRead Value: [ ", (double)i);
	  for(j = 0; j < 10; j++)
	    printf("(%f,%f) ", creal(retComplexArr[j]), cimag(retComplexArr[j]));
	  printf("]\n");
	  printf("Expected Value: [ ");
	  for(j = 0; j < 10; j++)
	    {
	      if(i < 5999)
	    
		printf("(%f,%f) ", (i%2)?creal((complexArr2D[i/2][j] + complexArr2D[i/2+1][j])/2.):creal(complexArr2D[i/2][j]), (i%2)?cimag((complexArr2D[i/2][j] + complexArr2D[i/2+1][j])/2.):cimag(complexArr2D[i/2][j]));
	      else
		printf("(%f,%f) ",  creal(complexArr2D[2999][j]), cimag(complexArr2D[2999][j]));
	    }  
	  printf("]\n\n");
	}
      if(i < 5999)
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(((i % 2) && retComplexArr[j] != (complexArr2D[i/2][j] + complexArr2D[i/2+1][j])/2.)
		 || (!(i % 2) && retComplexArr[j] != complexArr2D[i/2][j]))
		{
		  printf("--------------------> FAIL \n");
		  printf("Time: %f\tRead Value: [ ", (double)i);
		  for(j = 0; j < 10; j++)
		    printf("(%f,%f) ", creal(retComplexArr[j]), cimag(retComplexArr[j]));
		  printf("]\n");
		  printf("Expected Value: [ ");
		  for(j = 0; j < 10; j++)
		    {
		      if(i < 5999)
		  
			printf("(%f,%f) ", (i%2)?creal((complexArr2D[i/2][j] + complexArr2D[i/2+1][j])/2.):creal(complexArr2D[i/2][j]), (i%2)?cimag((complexArr2D[i/2][j] + complexArr2D[i/2+1][j])/2.):cimag(complexArr2D[i/2][j]));
		      else
			printf("(%f,%f) ",  creal(complexArr2D[2999][j]), cimag(complexArr2D[2999][j]));
		    }
		  printf("]\n\n");
		  exit(0);
		}
	    }
	}
      else
	{
	  for(j = 0; j < 10; j++)
	    {
	      if(retComplexArr[j] != complexArr2D[2999][j])
		{
		  printf("--------------------> FAIL \n");
		  exit(0);
		}
	    }
	}
      free((char *)retComplexArr);

    }
  printf("--------------------> OK\n");
    

  printf("\n\nRead/Write non timedependent array of structures in coreprof/composition/amn\n"); 
    
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  int ciccioField = 10;
  double bomboField = 20., bangField = 30.;
    
  int aosCtx = ual_begin_write_arraystruct(opCtx, 0, "composition/amn", 0, 1);
  if(status)
    {
      printf("ual_begin_write_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = ual_put_in_arraystruct(aosCtx,"CICCIO", 0,&ciccioField, INTEGER_DATA, 0, 0);
  if(!status) status = ual_put_in_arraystruct(aosCtx,"BOMBO/DUDU", 0,&bomboField, DOUBLE_DATA, 0, 0);
  if(!status) status = ual_put_in_arraystruct(aosCtx,"BOMBO/DINDO/BANG", 0,&bangField, DOUBLE_DATA, 0, 0);
  if(status)
    {
      printf("ual_put_in_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }

  printf("Written CICCIO->%d BOMBO/DUDU-> %f BOMBO/DINDO/BANG->%f\n", 10, 20., 30.);
  status = ual_end_action(aosCtx);
  if(status)
    {
      printf("releaseObject: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
  

  opCtx = beginCPOGet(ctx, "coreprof", 0);
  if(opCtx < 0)
    {
      printf("beginCPOGet: %s\n", euitm_last_errmsg());
      return 0;
    }
  int aosSize;
  aosCtx = ual_begin_read_arraystruct(opCtx, 0, "composition/amn", 0, &aosSize);
  if(aosCtx < 0)
    {
      printf("ual_begin_read_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }
  void *aosData;
  aosData = &ciccioField;
  status =  ual_get_from_arraystruct(aosCtx,"CICCIO", 0, &aosData, INTEGER_DATA, 0, NULL);
  if(status)
    {
      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }
  aosData = &bomboField;
  status =  ual_get_from_arraystruct(aosCtx,"BOMBO/DUDU", 0, &aosData, INTEGER_DATA, 0, NULL);
  if(status)
    {
      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }
  aosData = &bangField;
  status =  ual_get_from_arraystruct(aosCtx,"BOMBO/DINDO/BANG", 0, &aosData, INTEGER_DATA, 0, NULL);
  if(status)
    {
      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
      return 0;
    }
  bangField = *(double *)aosData;
  printf("Read CICCIO->%d BOMBO/DUDU-> %f BOMBO/DINDO/BANG->%f\n", ciccioField, bomboField,
	 bangField);
  status = ual_end_action(aosCtx);
  if(status)
    {
      printf("ual_end_action: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOGet(opCtx);
  if(status)
    {
      printf("endCPOGet: %s\n", euitm_last_errmsg());
      return 0;
    }

  if(ciccioField == 10 && bomboField == 20. && bangField == 30.)
    printf("--------------------->OK\n");
  else
    {
      printf("--------------------->FAIL\n");
      return 0;
    }
    
  printf("\n\nWrite/Read a sequence of array of structure slices in coreprof/compositions/edgespecies/timed\n"); 
  //delete first data in time
  opCtx = beginCPOPutNonTimed(ctx, "coreprof");
  if(opCtx < 0)
    {
      printf("beginCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  status = deleteData(opCtx, "time");
  if(status)
    {
      printf("deleteData: %s\n", euitm_last_errmsg());
      return 0;
    }
  status = endCPOPutNonTimed(opCtx);
  if(status)
    {
      printf("endCPOPutNonTimed: %s\n", euitm_last_errmsg());
      return 0;
    }
    
  for(i = 0; i < 10; i++)
    {
      opCtx = beginCPOPutSlice(ctx, "coreprof", (double)i);
      if(opCtx < 0)
	{
	  printf("beginCPOPutSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      aosCtx = ual_begin_write_arraystruct(opCtx,0,"compositions/edgespecies/timed", 1, 3);
      if(aosCtx < 0)
	{
	  printf("ual_begin_write_arraystruct: %s\n", euitm_last_errmsg());
	  return 0;
	}
      ciccioField = 10;
      bomboField = 20.;
      bangField = 30.;
      for(j = 0; j < 3; j++)
	{
	  ciccioField = 10+i*10 + j;
	  bomboField = 20+i*10 + j;
	  bangField = 30+i*10+j;

	  status = ual_put_in_arraystruct(aosCtx,"CICCIO", j,&ciccioField, INTEGER_DATA, 0, 0);
	  if(!status) status = ual_put_in_arraystruct(aosCtx,"BOMBO/DUDU", j,&bomboField, DOUBLE_DATA, 0, 0);
	  if(!status) status = ual_put_in_arraystruct(aosCtx,"BOMBO/DINDO/BANG", j,&bangField, DOUBLE_DATA, 0, 0);
	  if(status)
	    {
	      printf("ual_put_in_arraystruct: %s\n", euitm_last_errmsg());
	      return 0;
	    }
	  printf("Written [%d,%d]: CICCIO->%d BOMBO/DUDU->%f BOMBO/DINDO/BANG-> %f\n",i,j,ciccioField, bomboField, bangField);      
	}
      status = putDoubleSlice(opCtx, "time", (double)i);
      if(status)
	{
	  printf("putDoubleSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      ual_end_action(aosCtx);
      endCPOPutSlice(opCtx);
    }

  for(i = 0; i < 10; i++)
    {
      opCtx = beginCPOGetSlice(ctx, "coreprof", (double)i, LINEAR_INTERP);
      if(opCtx < 0)
	{
	  printf("beginCPOGetSlice: %s\n", euitm_last_errmsg());
	  return 0;
	}
      aosCtx = ual_begin_read_arraystruct(opCtx, 0, "compositions/edgespecies/timed", 0, &aosSize);
      if(aosCtx < 0)
	{
	  printf("ual_begin_read_arraystruct: %s\n", euitm_last_errmsg());
	  return 0;
	}
      void *aosData;
      for(j = 0; j < 3; j++)
	{
	  aosData = &ciccioField;
	  status =  ual_get_from_arraystruct(aosCtx,"CICCIO", j, &aosData, INTEGER_DATA, 0, NULL);
	  if(status)
	    {
	      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
	      return 0;
	    }
	  aosData = &bomboField;
	  status =  ual_get_from_arraystruct(aosCtx,"BOMBO/DUDU", j, &aosData, DOUBLE_DATA, 0, NULL);
	  if(status)
	    {
	      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
	      return 0;
	    }
	  aosData = &bangField;
	  status =  ual_get_from_arraystruct(aosCtx,"BOMBO/DINDO/BANG", j, &aosData, DOUBLE_DATA, 0, NULL);
	  if(status)
	    {
	      printf("ual_get_from_arraystruct: %s\n", euitm_last_errmsg());
	      return 0;
	    }
	  bangField = *(double *)aosData;

	  printf("Read [%d,%d]: CICCIO->%d BOMBO/DUDU->%f BOMBO/DINDO/BANG->%f\n",i,j,ciccioField, bomboField, bangField); 
	  if(ciccioField != 10+i*10 + j)
	    {
	      printf("ERROR in slice %d arraystruct[%d] expected %d read %d\n", i,j,10+i*10 + j,ciccioField);
	      printf("-----------------> FAIL\n");
	      exit(0);
	    }
	  if(bomboField != 20+i*10 + j)
	    {
	      printf("ERROR in slice %d arraystruct[%d] expected %f read %f\n", i,j,20+i*10. + j,bomboField);
	      printf("-----------------> FAIL\n");
	      exit(0);
	    }
	  if(bangField != 30+i*10+j)
	    {
	      printf("ERROR in slice %d arraystruct[%d] expected %f read %f\n", i,j,30+i*10. + j,bangField);
	      printf("-----------------> FAIL\n");
	      exit(0);
	    }
	  printf("-----------------> OK\n");
	}
      ual_end_action(aosCtx);
      endCPOGetSlice(opCtx);
    }

  ///////////////////////////////////////////////////////////////////////   
     
  status = euitm_close(ctx);
  if(status)
    {
      printf("euitm_close: %s\n", euitm_last_errmsg());
      return 0;
    }

}
