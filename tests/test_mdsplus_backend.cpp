#include <mdsplus_backend.h>
#include <memory_backend.h>

/* Test program for the MDSplus backend.
The tests carried out so far are:

1) Read/write a String value in coreprof/codeparam/codename
2) Read/Write an integer value in coreprof/codeparam/codeversion
2 bis) Read/Write a complex value in  coreprof/codeparam/parameters
3) Read/write a 1D double array (non time dependent)in coreprof/drho_dt
3 bis) Read/write a 1D complex array (non time dependent)in coreprof/drho_dt
4) Read/write a 2D double array (non time dependent) in coreprof/drho_dt
4bis) Read/write a 2D complex array (non time dependent) in coreprof/drho_dt
5) Read/Write a time dependent double 1D array in coreprof/profiles1D/pi/value
5 bis) Read/Write a time dependent complex 1D array in coreprof/profiles1D/pi/value
6) Read/Write a time dependent 2D double array in coreprof/profiles1D/pi/value
6 bis) Read/Write a time dependent 2D complex array in coreprof/profiles1D/pi/value
7) Read/Write a time dependent 1D double array slice per slice in coreprof/profiles1D/pi/value
7 bis) Read/Write a time dependent 1D complex array slice per slice in coreprof/profiles1D/pi/value
8) Read/Write a time dependent 2D double array slice per slice in coreprof/profiles1D/pi/value
8 bis) Read/Write a time dependent 2D complex array slice per slice in coreprof/profiles1D/pi/value
9) Write a large time dependent  double array in coreprof/profiles1D/pi/value slice by slice and read it both slice per slice and via a single get
9 bis) Write a large time dependent complex array in coreprof/profiles1D/pi/value slice by slice and read it both slice per slice and via a single get
10) Write a large time dependent double array in a single put() in coreprof/profiles1D/pi/value read it slice by slice
10 bis) Write a large time dependent complex array in a single put() in coreprof/profiles1D/pi/value read it slice by slice
11) Write and read string slices
*/
int main(int argc, char *argv[])
{
#ifdef MEMORY_BACKEND
  Backend* mdsplusBackend = new MDSplusBackend; 
  Backend *backend = new MemoryBackend(mdsplusBackend);
#else
  Backend* backend = new MDSplusBackend; 
#endif
  std::string user(std::getenv("USER"));
  std::string tokamak("test");
  std::string version("3");
  int retDims[64];
  PulseContext pulseCtx(10, 123, 0, user, tokamak, version);

  try {
    backend->openPulse(&pulseCtx, CREATE_PULSE, "");
    
    std::string cpo("actuator");
    OperationContext opWriteTimedCtx(pulseCtx, cpo, WRITE_OP);
    OperationContext opReadTimedCtx(pulseCtx, cpo, READ_OP);
    OperationContext opWriteCtx(pulseCtx, cpo, WRITE_OP);
    OperationContext opReadCtx(pulseCtx, cpo, READ_OP);
 
    std::cout << "Read/write a String value in actuator/name" << std::endl;
    const char *codeName="CODE_NAME_XXX";
    int dims[8];
    dims[0] = strlen(codeName)+1; // NEED EITHER strlen+1 HERE 
    // to be done by which layer is supposed to define "string" handling
    std::string field("name");
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx, field, "", (void *)codeName, CHAR_DATA, 1, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written " << codeName << " in actuator/name....";

    char *retStr;
    int retType;
    int retNumDims;
    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **)&retStr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx); // OR PADDING THE RETURNED CHAR ARRAY WITH AN ADDITIONAL \0 HERE
    // to be done by which layer is supposed to define "string" handling
    std::cout << "Read " << retStr;
    if(!strcmp(retStr, codeName))
     std::cout << "   -------------> OK" << std::endl;
    else
    {
       std::cout << "   -------------> FAIL" << std::endl;
       exit(0);
    }
    free(retStr);
    
    
    std::cout << "\n\n1 Read/Write an integer value in actuator/code/version" << std::endl;
    int codeVersion = 123;
    field = "code/version";
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field,"", (void *)&codeVersion, INTEGER_DATA, 0, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written " << codeVersion << " in actuator/code/version....";
    
    int *retVal;
    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **)&retVal, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);
    std::cout << "Read " << *retVal;
    if(*retVal == codeVersion)
      std::cout << "   -------------> OK" << std::endl;
    else
    {
       std::cout << "   -------------> FAIL" << std::endl;
       exit(0);
    }
    free(retVal);
    
    
    std::cout << "\n\n 2 Read/Write a complex value in actuator/code/version" << std::endl;
    double complexData[2];
    complexData[0] = 1;
    complexData[1] = 2;
    field = "code/version";
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field,"", (void *)complexData, COMPLEX_DATA, 0, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written " << "Re: " << complexData[0] << "  Im: " << complexData[1] << " in actuator/code/version....";
    
    double *retComplex;
    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **)&retComplex, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);
    std::cout << "Read " << "Re: " << retComplex[0] << "  Im: " << retComplex[1];
    if(retComplex[0] == complexData[0] && retComplex[1] == complexData[1])
           std::cout << "   -------------> OK" << std::endl;
    else
    {
       std::cout << "   -------------> FAIL" << std::endl;
       exit(0);
    }
    free(retComplex);
    
    
    std::cout << "\n\n3 Read/write a 1D double array (non time dependent)in actuator/code/version" << std::endl;
    double doubleArr[3000];
    for(int i = 0; i < 10; i++)
      doubleArr[i] = i;
    field = "code/version";
    dims[0] = 10;
    
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field, "", (void *)doubleArr, DOUBLE_DATA, 1, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written ";
    for(int i = 0; i < 10; i++)
      std::cout << doubleArr[i] << " ";
    std::cout << "in actuator/code/version....";
    double *retArr;
    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **) &retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < retDims[0]; i++)
      std::cout << retArr[i] << " ";

    for(int i = 0; i < retDims[0]; i++)
    {
      if(retArr[i] != doubleArr[i])
      {
	std::cout << "   -------------> FAIL" << std::endl;
	exit(0);
      }
    }
    std::cout << "   -------------> OK" << std::endl;
    free(retArr);

    
    std::cout << "\n\n4 Read/write a 1D complex array (non time dependent)in actuator/code/version" << std::endl;
    for(int i = 0; i < 10; i++)
    {
      doubleArr[2*i] = i;
      doubleArr[2*i+1] = i+0.1;
    }
    field = "code/version";
    dims[0] = 10;
    
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field,"", (void *)doubleArr, COMPLEX_DATA, 1, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written ";
    for(int i = 0; i < 10; i++)
      std::cout << "(" << doubleArr[2*i]<<","<<doubleArr[2*i+1] << ")" << " ";
    std::cout << "in actuator/code/version....";

    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **) &retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < retDims[0]; i++)
      std::cout << "(" << retArr[2*i]<<","<<retArr[2*i+1] << ")" << " ";
    
    for(int i = 0; i < 2*retDims[0]; i++)
    {
      if(retArr[i] != doubleArr[i])
      {
	std::cout << "   -------------> FAIL" << std::endl;
	exit(0);
      }
    }
    std::cout << "   -------------> OK" << std::endl;

    free(retArr);

    
    std::cout << "\n\n5 Read/write a 2D double array (non time dependent) in actuator/code/version" << std::endl;
    double double2DArr[2][5];
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 5; j++)
	double2DArr[i][j] = (i+1)*j;
    field = "code/version";
    dims[0] = 2;
    dims[1] = 5;
    
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field, "", (void *)double2DArr, DOUBLE_DATA, 2, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written ";
    for(int i = 0; i < 2; i++)
    {
      std::cout << "[";
      for(int j = 0; j < 5; j++)
	std::cout << double2DArr[i][j] << " ";
      std::cout << "]";
    }
    std::cout << "in actuator/code/version....";

    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);

    std::cout << "Read array[" << retDims[0] << "][" << retDims[1] << "] " ;
    for(int i = 0; i < retDims[0]; i++)
    {
      std::cout << "[";
      for(int j = 0; j < retDims[1]; j++)
	std::cout << retArr[i*retDims[1]+j] << " ";
      std::cout << "]";
    }

    for(int i = 0; i < retDims[0]; i++)
    {
      for(int j = 0; j < retDims[1]; j++)
      {
	if(retArr[i*retDims[1]+j]  != double2DArr[i][j])
	{
	  std::cout << "   -------------> FAIL" << std::endl;
	  exit(0);
	}
      }
    }
    if(retDims[0] != dims[0] || retDims[1] != dims[1])
    {
      std::cout << "   -------------> FAIL" << std::endl;
      exit(0);
    }
    std::cout << "   -------------> OK" << std::endl;
    free(retArr);

    
    std::cout << "\n\n6 Read/write a 2D complex array (non time dependent) in actuator/code/version" << std::endl;
    double complex2DArr[2][5][2];
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 5; j++)
      {
	complex2DArr[i][j][0] = (i+1)*j;
	complex2DArr[i][j][1] = (i+1)*j + .1;
      }
    field = "code/version";
    dims[0] = 2;
    dims[1] = 5;
    
    backend->beginAction(&opWriteCtx);
    backend->writeData(&opWriteCtx,field,"", (void *)complex2DArr, COMPLEX_DATA, 2, dims);
    backend->endAction(&opWriteCtx);
    std::cout << "Written ";
    for(int i = 0; i < 2; i++)
    {
      std::cout << "[";
      for(int j = 0; j < 5; j++)
	std::cout << "(" << complex2DArr[i][j][0] << "," << complex2DArr[i][j][1]  << ") ";
      std::cout << "]";
    }
    std::cout << "in actuator/code/version....";

    backend->beginAction(&opReadCtx);
    backend->readData(&opReadCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadCtx);

    std::cout << "Read array[" << retDims[0] << "][" << retDims[1] << "] " ;
    for(int i = 0; i < retDims[0]; i++)
    {
      std::cout << "[";
      for(int j = 0; j < retDims[1]; j++)
	std::cout << "(" << retArr[2 *( i*retDims[1]+j)] << "," << retArr[2 * (i*retDims[1]+j) + 1]  << ") ";
      std::cout << "]";
    }

    for(int i = 0; i < retDims[0]; i++)
    {
      for(int j = 0; j < retDims[1]; j++)
      {
	if(retArr[2 * (i*retDims[1]+j)]  != complex2DArr[i][j][0] || 
	    retArr[2 * (i*retDims[1]+j) + 1]  != complex2DArr[i][j][1] )
	{
	  std::cout << "   -------------> FAIL" << std::endl;
	  exit(0);
	}
      }
    }
    if(retDims[0] != dims[0] || retDims[1] != dims[1])
    {
      std::cout << "   -------------> FAIL" << std::endl;
      exit(0);
    }
    std::cout << "   -------------> OK" << std::endl;
    free(retArr);

    
    std::string timeField("time");
    
    std::cout << "\n\n7 Read/Write a time dependent 1D double array in actuator/power" << std::endl;
    for(int i = 0; i < 10; i++)
      doubleArr[i] = i;
    field = "power";
    dims[0] = 10;
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", doubleArr, DOUBLE_DATA, 1, dims);
    double timesData[10000];
    for(int i = 0; i < 10; i++)
      timesData[i] = i;
    dims[0] = 10;
    backend->writeData(&opWriteTimedCtx,"time", "time",timesData, DOUBLE_DATA, 1, dims);
    backend->endAction(&opWriteTimedCtx);
    std::cout << "Written ";
    for(int i = 0; i < 10; i++)
      std::cout << doubleArr[i] << " ";
    std::cout << "actuator/power....";

    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < retDims[0]; i++)
      std::cout << retArr[i] << " ";
    
    if(retDims[0] != 10)
    {
      std::cout << "--------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < retDims[0]; i++)
    {
      if(retArr[i] != doubleArr[i])
      {
	std::cout << "--------------> FAIL" << std::endl;
	exit(0);
      }
      
    }
    std::cout << "---------------------> OK" << std::endl;
    free(retArr);

    
    std::cout << "\n\n8 Read/Write a time dependent 1D complex array in actuator/power" << std::endl;
    for(int i = 0; i < 10; i++)
    {
      doubleArr[2*i] = i;
      doubleArr[2*i+1] = i+.1;
    }
    field = "power";
    dims[0] = 10;
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", doubleArr, COMPLEX_DATA, 1, dims);
    backend->endAction(&opWriteTimedCtx);
    std::cout << "Written ";
    for(int i = 0; i < 10; i++)
      std::cout << "(" << doubleArr[2*i] << "," << doubleArr[2*i+1] << ") ";
    std::cout << "actuator/power...." << std::endl;

    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < retDims[0]; i++)
      std::cout << "(" << retArr[2*i] << "," << retArr[2*i+1]  << ") ";
    
    if(retDims[0] != 10)
    {
      std::cout << "--------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < 2 * retDims[0]; i++)
    {
      if(retArr[i] != doubleArr[i])
      {
	std::cout << "--------------> FAIL" << std::endl;
	exit(0);
      }
      
    }
    std::cout << "---------------------> OK" << std::endl;
    free(retArr);

    
    std::cout << "\n\n9 Read/Write a time dependent 2D double array in actuator/power" << std::endl;
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 5; j++)
	double2DArr[i][j] = i*j;
    field = "power";
    dims[0] = 2;
    dims[1] = 5;
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", (void *)double2DArr, DOUBLE_DATA, 2, dims);
    backend->endAction(&opWriteTimedCtx);
    std::cout << "Written ";
    for(int i = 0; i < 2; i++)
    {
      std::cout << "[";
      for(int j = 0; j < 5; j++)
	std::cout << double2DArr[i][j] << " ";
      std::cout << "]";
    }
    std::cout << "in actuator/power....";

    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "][" << retDims[1] << "] " ;
    for(int i = 0; i < retDims[0]; i++)
    {
      std::cout << "[";
      for(int j = 0; j < retDims[1]; j++)
	std::cout << retArr[i*retDims[1]+j] << " ";
      std::cout << "]";
    }
    if(retNumDims != 2 || retDims[0] != 2 || retDims[1] != 5)
    {
      std::cout << "--------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < retDims[0]; i++)
    {
      for(int j = 0; j < retDims[1]; j++)
      {
 	if(retArr[i*retDims[1]+j] != double2DArr[i][j])
	{
	  std::cout << "--------------> FAIL" << std::endl;
	  exit(0);
	}
      }
    }
    std::cout << std::endl;
    free(retArr);
   
    std::cout << "\n\n10 Read/Write a time dependent 2D complex array in actuator/power" << std::endl;
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 5; j++)
      {
	complex2DArr[i][j][0] = i*j;
	complex2DArr[i][j][0] = i*j+.1;
      }
    field = "power";
    dims[0] = 2;
    dims[1] = 5;
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", (void *)complex2DArr, COMPLEX_DATA, 2, dims);
    backend->endAction(&opWriteTimedCtx);
    std::cout << "Written ";
    for(int i = 0; i < 2; i++)
    {
      std::cout << "[";
      for(int j = 0; j < 5; j++)
	std::cout << "(" << complex2DArr[i][j][0] << "," << complex2DArr[i][j][1] << ") ";
      std::cout << "]";
    }
    std::cout << "in actuator/power...." << std::endl;

    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "][" << retDims[1] << "] " ;
    for(int i = 0; i < retDims[0]; i++)
    {
      std::cout << "[";
      for(int j = 0; j < retDims[1]; j++)
	std::cout << "(" << retArr[2*(i*retDims[1]+j)] << retArr[2*(i*retDims[1]+j)+1] << ") ";
      std::cout << "]";
    }
    if(retNumDims != 2 || retDims[0] != 2 || retDims[1] != 5)
    {
      std::cout << "--------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < retDims[0]; i++)
    {
      for(int j = 0; j < retDims[1]; j++)
      {
	if(retArr[2*(i*retDims[1]+j)] != complex2DArr[i][j][0] || retArr[2*(i*retDims[1]+j)+1] != complex2DArr[i][j][1])
	{
	  std::cout << "--------------> FAIL" << std::endl;
	  exit(0);
	}
      }
    }
    std::cout << "--------------------> OK" << std::endl;
    free(retArr);


//GABGABGAB    
    std::cout << "\n\n11 Read/Write a time dependent string  slice per slice in actuator/power" << std::endl;
    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
    
    char *strings[10];
    for(int i = 0; i < 10; i++)
    { 
      strings[i] = new char[6];
      sprintf(strings[i], "CIAO%d", i);
    }
    field = "power";
    dims[0] = 5;
//Gabriele July 2017
    dims[1] = 1;
    
    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
//	backend->writeData(&sliceWriteCtx,field,"time", (void *)strings[i], CHAR_DATA, 1, dims);
	dims[0] = 5;
	backend->writeData(&sliceWriteCtx,field,"time", (void *)strings[i], CHAR_DATA, 2, dims);
	double iD = i;
	dims[0] = 1;
	backend->writeData(&sliceWriteCtx,"time","time", (void *)&iD, DOUBLE_DATA, 1, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: " << strings[i] << std::endl; 
    }

    char *retStringPtr;
    for(int i = 0; i < 10; i++)
    {
	
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retStringPtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i << "\tRead Value: " << retStringPtr << std::endl; 
	if(strncmp(retStringPtr, strings[i], strlen(strings[i])))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retStringPtr);
    }
    std::cout << "-----------------> OK" << std::endl;
    
     
    
    
    
//////////////////////////    
    
    
    std::cout << "\n\n11 BIS Read/Write a time dependent 1D double array slice per slice in actuator/power" << std::endl;
    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
//    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
    
    for(int i = 0; i < 10; i++)
      doubleArr[i] = i*1000.;
    field = "power";
    dims[0] = 1;

    
    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	//gABRIELE jULY 2017
	//backend->writeData(&sliceWriteCtx,field,"time", (void *)&doubleArr[i], DOUBLE_DATA, 0, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&doubleArr[i], DOUBLE_DATA, 1, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: " << doubleArr[i] << std::endl; 
    }

    double *retDoublePtr;
    for(int i = 0; i < 20; i++)
    {
	
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << std::endl; 

	if(((i % 2 == 0 || i == 19) && retDoublePtr[0] != doubleArr[i/2])||
	  ((i % 2 == 1 && i < 19) && retDoublePtr[0] != (doubleArr[i/2]+ doubleArr[i/2+1])/2.))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
    }
    std::cout << "-----------------> OK" << std::endl;
    
    
    
    
    std::cout << "\n\n12 Read/Write a time dependent 1D complex array slice per slice in actuator/power" << std::endl;
    double complexArr[3000][2];
    
    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
//    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
    
    for(int i = 0; i < 10; i++)
    {
      complexArr[i][0] = i*1000.;
      complexArr[i][1] = i*1000. + 0.1;
    }
    field = "power";
    dims[0] = 1;

    
    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
//Gabriele July 2017
	//backend->writeData(&sliceWriteCtx,field,"time", (void *)&complexArr[i][0], COMPLEX_DATA, 0, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&complexArr[i][0], COMPLEX_DATA, 1, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: " << "(" << complexArr[i][0] << "," << complexArr[i][1] << ")" << std::endl; 
    }

    for(int i = 0; i < 20; i++)
    {
	
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "  Read Value: (" << retDoublePtr[0] << "," <<  retDoublePtr[1] << ")" << std::endl; 
	if(((i % 2 == 0 || i == 19) && retDoublePtr[0] != complexArr[i/2][0])||
	 ((i % 2 == 0 || i == 19) && retDoublePtr[1] != complexArr[i/2][1])||
	 ((i % 2 == 1 && i < 19) && retDoublePtr[0] != (complexArr[i/2][0]+ complexArr[i/2+1][0])/2.)||
	 ((i % 2 == 1 && i < 19) && retDoublePtr[1] != (complexArr[i/2][1]+ complexArr[i/2+1][1])/2.))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
    }

    std::cout << "-----------------> OK" << std::endl;
    
    std::cout << "\n\n13 Read/Write a time dependent 2D double array slice per slice in actuator/power" << std::endl;

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
 //   backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 5; j++)
	double2DArr[i][j] = (1+i)*j;
    field = "power";
    dims[0] = 5;
    dims[1] = 1;

    
    for(int i = 0; i < 2; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
//Gabriele July 2017
	//backend->writeData(&sliceWriteCtx,field,"time", (void *)&double2DArr[i], DOUBLE_DATA, 1, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&double2DArr[i], DOUBLE_DATA, 2, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: [";
	for(int j = 0; j < 5; j++)
	  std::cout << double2DArr[i][j] << " ";
	std::cout << "]" << std::endl;

    }

    for(int i = 0; i < 4; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retArr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Values: [";
	for(int j = 0; j < 5; j++)
	  std::cout << retArr[j] << " ";
	std::cout << "]" << std::endl;

	for(int j = 0; j < 5; j++)
	{
	  if(((i % 2 == 0 || i == 3) && retArr[j] != double2DArr[i/2][j])||
	  ((i % 2 == 1 && i < 3) && retArr[j] != (double2DArr[i/2][j]+ double2DArr[i/2+1][j])/2.))
	  {
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	  }
	}
	free(retArr);
    }

    std::cout << "----------------------> OK" << std::endl;

    std::cout << "\n\n14 Read/Write a time dependent 2D complex array slice per slice in actuator/power" << std::endl;

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
//    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 
     double complex2DArray[2][5][2];
     for(int i = 0; i < 2; i++)
       for(int j = 0; j < 5; j++)
       {
 	complex2DArray[i][j][0] = (1+i)*j;
 	complex2DArray[i][j][1] = (1+i)*j + 0.1;
       }
    field = "power";
    dims[0] = 5;
    dims[1] = 1;
    
    for(int i = 0; i < 2; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	//Gabriele July 2017
//	backend->writeData(&sliceWriteCtx,field,"time", (void *)&complex2DArr[i], COMPLEX_DATA, 1, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&complex2DArr[i], COMPLEX_DATA, 2, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: [";
	for(int j = 0; j < 5; j++)
	  std::cout << "(" << complex2DArr[i][j][0] << ","<< complex2DArr[i][j][1] << ") ";
	std::cout << "]" << std::endl;

    }

    for(int i = 0; i < 4; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retArr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Values: [";
	for(int j = 0; j < 5; j++)
	  std::cout << "(" << retArr[2*j] << "," << retArr[2*j+1] << ") ";
	std::cout << "]" << std::endl;

	for(int j = 0; j < 5; j++)
	{
	  if ( 
	      ((i % 2 == 0 || i == 3) && (retArr[2*j] != complex2DArr[i/2][j][0]))   ||
	      ((i % 2 == 0 || i == 3) && (retArr[2*j+1] != complex2DArr[i/2][j][1])) ||
	      ((i % 2 == 1 && i < 3) && (fabs(retArr[2*j] - (complex2DArr[i/2][j][0]+ complex2DArr[i/2+1][j][0])/2.) > 1E-10)) ||
	      ((i % 2 == 1 && i < 3) && (fabs(retArr[2*j+1] - (complex2DArr[i/2][j][1]+ complex2DArr[i/2+1][j][1])/2.) > 1E-10)))
	    {
	      std::cout << "-----------------> FAIL  " ;
	      exit(0);
	    }
	}
	free(retArr);
    }

    std::cout << "----------------------> OK" << std::endl;




    std::cout << "\n\n15 Write a large time dependent double array in actuator/power slice by slice and read it both slice per slice and via a single get" << std::endl;

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

    field = "power";
    dims[0] = 1;
    double currDouble;
    for(int i = 0; i < 3000; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	currDouble = i*100.;
//Gabriele July 2017
//	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currDouble, DOUBLE_DATA, 0, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currDouble, DOUBLE_DATA, 1, dims);
	double currTime = i;
//	backend->writeData(&sliceWriteCtx, "time", "time",(void *)&currTime, DOUBLE_DATA, 0, dims);
	backend->writeData(&sliceWriteCtx, "time", "time",(void *)&currTime, DOUBLE_DATA, 1, dims);
	backend->endAction(&sliceWriteCtx);
     }
    for(int i = 0; i < 30; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "   Expected value:  " << 100*i/2 << std::endl; 

	if(*retDoublePtr != 100 * i/2.)
	{
	  std::cout << "--------------------> FAIL" << std::endl;
	  exit(0);
	}
	
	
	free(retDoublePtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i < 5999)
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "   Expected value:  " << 100*i/2 << std::endl; 
	else
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "   Expected value:  " << 100*5998/2 << std::endl; 

	if((i < 5999 && *retDoublePtr != 100 * i/2) || (i >= 5999 && *retDoublePtr != 100*5998/2 ))
	{
	  std::cout << "--------------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
    }
    
    
    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "time", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << retArr[i] << " ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << retArr[i] << " ";
    std::cout << "]" << std::endl;
    std::cout << "Expected array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << i * 100. << " ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << i * 100. << " ";
    std::cout << "]" << std::endl;
   
   for(int i = 0; i < 3000; i++)
   {
     if(retArr[i] != i*100)
    {
	std::cout << "--------------------> FAIL" << std::endl;
	exit(0);
    }
       
   }
    free(retArr);

    std::cout << "--------------------> OK" <<std::endl;
    
    


    std::cout << "\n\n16 Write a large time dependent int array in actuator/power slice by slice and read it both slice per slice and via a single get" << std::endl;

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
//    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

    field = "power";
    dims[0] = 1;
    int currInt;
    int *retIntPtr;
    for(int i = 0; i < 3000; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo, WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	currInt = i*100.;
//Gabriele July 2017
//	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currInt, INTEGER_DATA, 0, dims);
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currInt, INTEGER_DATA, 1, dims);
	backend->endAction(&sliceWriteCtx);
     }
    for(int i = 0; i < 30; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)(i/2. - .1), CLOSEST_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retIntPtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< (i/2.-.1) << "\tRead Value: " << *retIntPtr << "   Expected value:  " << 100*(i/2) << std::endl; 

	if(*retIntPtr != 100 * (i/2))
	{
	  std::cout << "--------------------> FAIL" << std::endl;
	  exit(0);
	}
	
	
	free(retIntPtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2.-.1, CLOSEST_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retIntPtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i < 5999)
	  std::cout << "Time: "<< (i/2. -.1) << "\tRead Value: " << *retIntPtr << "   Expected value:  " << 100*(i/2) << std::endl; 
	else
	  std::cout << "Time: "<< (i/2. -.1) << "\tRead Value: " << *retIntPtr << "   Expected value:  " << 100*5998/2 << std::endl; 

	if((i < 5999 && *retIntPtr != 100 * (i/2)) || (i >= 5999 && *retIntPtr != 100*5998/2 ))
	{
	  std::cout << "--------------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retIntPtr);
    }
    
    
    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "time", (void **)&retIntPtr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << retIntPtr[i] << " ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << retIntPtr[i] << " ";
    std::cout << "]" << std::endl;
    std::cout << "Expected array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << i * 100 << " ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << i * 100 << " ";
    std::cout << "]" << std::endl;
   
   for(int i = 0; i < 3000; i++)
   {
     if(retIntPtr[i] != i*100)
    {
	std::cout << "--------------------> FAIL" << std::endl;
	exit(0);
    }
       
   }
    free(retIntPtr);

    std::cout << "--------------------> OK" <<std::endl;
    
    

    std::cout << "\n\n17 Write a large time dependent complex array in actuator/power slice by slice and read it both slice per slice and via a single get" << std::endl;

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
//    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

    field = "power";
    dims[0] = 1;
    double currComplex[3000][2];
    for(int i = 0; i < 3000; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpo,  WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	currComplex[i][0] = i*100.;
	currComplex[i][1] = i*100. + 0.1;
//Gabriele July 2017
	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currComplex[i][0], COMPLEX_DATA, 1, dims);
//	backend->writeData(&sliceWriteCtx,field,"time", (void *)&currComplex[i][0], COMPLEX_DATA, 0, dims);
	backend->endAction(&sliceWriteCtx);
     }
    for(int i = 0; i < 30; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i % 2 == 0)
	{
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1] << ")   Expected value:  (" << currComplex[i/2][0] << "," << currComplex[i/2][1] << ")" << std::endl; 

	  if(fabs(retDoublePtr[0] - currComplex[i/2][0]) > 1E-10 ||
	   fabs(retDoublePtr[1] - currComplex[i/2][1])> 1E-10)
	  {
	    std::cout << "--------------------> FAIL" << fabs(retDoublePtr[0] - currComplex[i/2][0] / 2) << " " << fabs(retDoublePtr[1] - currComplex[i/2][1] / 2)<< std::endl;
	    exit(0);
	  }
	}
	else
	{
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1] << ")   Expected value:  (" << (currComplex[i/2][0] + currComplex[i/2 + 1][0])/2 << "," << (currComplex[i/2][1] + currComplex[i/2+1][1])/2<< ")" << std::endl; 

	  if(fabs(retDoublePtr[0] - (currComplex[i/2][0] + currComplex[i/2 + 1][0])/2) > 1E-10 ||
	   fabs(retDoublePtr[1] - (currComplex[i/2][1] + currComplex[i/2 + 1][1])/2)> 1E-10)
	  {
	    std::cout << "--------------------> FAIL" << fabs(retDoublePtr[0] - currComplex[i/2][0] / 2) << " " << fabs(retDoublePtr[1] - currComplex[i/2][1] / 2)<< std::endl;
	    exit(0);
	  }
	}
	free(retDoublePtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i < 5999)
	{
	  if(i % 2 == 0)
	  {
	    std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1] << ")   Expected value:  (" << currComplex[i/2][0] << "," << currComplex[i/2][1] << ")" << std::endl; 

	    if(fabs(retDoublePtr[0] - currComplex[i/2][0]) > 1E-10 ||
	      fabs(retDoublePtr[1] - currComplex[i/2][1])> 1E-10)
	    {
	      std::cout << "--------------------> FAIL" << fabs(retDoublePtr[0] - currComplex[i/2][0] / 2) << " " << fabs(retDoublePtr[1] - currComplex[i/2][1] / 2)<< std::endl;
	      exit(0);
	    }
	  }
	  else
	  {
	    std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1] << ")   Expected value:  (" << (currComplex[i/2][0] + currComplex[i/2 + 1][0])/2 << "," << (currComplex[i/2][1] + currComplex[i/2+1][1])/2<< ")" << std::endl; 

	    if(fabs(retDoublePtr[0] - (currComplex[i/2][0] + currComplex[i/2 + 1][0])/2) > 1E-10 ||
	   fabs(retDoublePtr[1] - (currComplex[i/2][1] + currComplex[i/2 + 1][1])/2)> 1E-10)
	    {
	      std::cout << "--------------------> FAIL" << fabs(retDoublePtr[0] - currComplex[i/2][0] / 2) << " " << fabs(retDoublePtr[1] - currComplex[i/2][1] / 2)<< std::endl;
	      exit(0);
	    }
	  }
	}
	else
	{
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1] << ")   Expected value:  " << "("<<currComplex[2999][0]<<","<<currComplex[2999][1]<<")"<< std::endl; 

	  if(retDoublePtr[0] != currComplex[2999][0] || retDoublePtr[1] != currComplex[2999][1])
	  {
	    std::cout << "--------------------> FAIL" << std::endl;
	    exit(0);
	  }
	}
	free(retDoublePtr);
    }
    
    std::cout << "---------------------------> OK" << std::endl;
    
    backend->beginAction(&opReadTimedCtx);
    backend->readData(&opReadTimedCtx,field, "time", (void **)&retArr, &retType, &retNumDims, retDims);
    backend->endAction(&opReadTimedCtx);

    std::cout << "Read array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << "(" << retArr[2*i]<<","<< retArr[2*i+1]<< ") ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << "(" << retArr[2*i]<<","<< retArr[2*i+1]<< ") ";
    std::cout << "]" << std::endl;
    std::cout << "Expected array[" << retDims[0] << "]";
    for(int i = 0; i < 10; i++)
      std::cout << "(" << currComplex[i][0] << "," << currComplex[i][1] << ") ";
    std::cout << "..." ;
   for(int i = 2990; i < 3000; i++)
      std::cout << "(" << currComplex[i][0] << "," << currComplex[i][1] << ") ";
    std::cout << "]" << std::endl;
   
   for(int i = 0; i < 3000; i++)
   {
     if(retArr[2*i] != currComplex[i][0] || retArr[2*i+1] != currComplex[i][1])
    {
	std::cout << "--------------------> FAIL" << std::endl;
	exit(0);
    }
       
   }
    free(retArr);

    std::cout << "--------------------> OK" <<std::endl;
    
    
    
    
    
    
    std::cout << "\n\n18 Write a large time dependent double array in a single put() in actuator/power read it slice by slice" << std::endl;   

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

    double times[3000];  
   
   for(int i = 0; i < 3000; i++)
   {
      times[i] = i;
      doubleArr[i] = i*10;
   }
    dims[0] = 3000;
    field = "power";
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", (void *)doubleArr, DOUBLE_DATA, 1, dims);
    backend->writeData(&opWriteTimedCtx,timeField,"time", (void *)times, DOUBLE_DATA, 1, dims);
    backend->endAction(&opWriteTimedCtx);
  
    for(int i = 0; i < 20; i++)
    {

        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "  Expected value: " << i*10./2<< std::endl; 

	if(((i % 2 == 0) && retDoublePtr[0] != doubleArr[i/2])||
	  ((i % 2 == 1) && retDoublePtr[0] != (doubleArr[i/2]+ doubleArr[i/2+1])/2.))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i < 5999)
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "  Expected value: " << i*10./2 << std::endl; 
	else
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retDoublePtr << "  Expected value: " << 5998*10./2 << std::endl; 
	  
	if(((i % 2 == 0 && i < 5999) && retDoublePtr[0] != doubleArr[i/2])||
	  ((i % 2 == 1 && i < 5999) && retDoublePtr[0] != (doubleArr[i/2]+ doubleArr[i/2+1])/2.)||
	  ((i >= 5999) && retDoublePtr[0] != doubleArr[2999]))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
      }
      std::cout << "------------------> OK " << std::endl;
   
     std::cout << "\n\n19 Write a large time dependent int array in a single put() in actuator/power read it slice by slice" << std::endl;   

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

    int intArr[3000];
   for(int i = 0; i < 3000; i++)
   {
      times[i] = i;
      intArr[i] = i*10;
   }
    dims[0] = 3000;
    field = "power";
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", (void *)intArr, INTEGER_DATA, 1, dims);
    backend->writeData(&opWriteTimedCtx,timeField,"time", (void *)times, DOUBLE_DATA, 1, dims);
    backend->endAction(&opWriteTimedCtx);
  
    for(int i = 0; i < 20; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., PREVIOUS_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retIntPtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	std::cout << "Time: "<< i/2. << "\tRead Value: " << *retIntPtr << "  Expected value: " << intArr[i/2] << std::endl; 

	if(retIntPtr[0] != intArr[i/2])
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retIntPtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., PREVIOUS_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retIntPtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i < 5999)
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retIntPtr << "  Expected value: " << intArr[i/2] << std::endl; 
	else
	  std::cout << "Time: "<< i/2. << "\tRead Value: " << *retIntPtr << "  Expected value: " << 5998*10./2 << std::endl; 
	  
	if((i < 5999 && retIntPtr[0] != intArr[i/2])||
	   (i >= 5999 && retIntPtr[0] != intArr[2999]))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retIntPtr);
      }
      std::cout << "------------------> OK " << std::endl;
   
     std::cout << "\n\n20 Write a large time dependent complex array in a single put() in actuator/power read it slice by slice" << std::endl;   

    backend->beginAction(&opReadTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadTimedCtx,field);
    backend->deleteData(&opReadTimedCtx,timeField);
    backend->endAction(&opReadTimedCtx);
 

   for(int i = 0; i < 3000; i++)
   {
      times[i] = i;
      complexArr[i][0] = i*10;
      complexArr[i][1] = i*10+0.1;
   }
    dims[0] = 3000;
    field = "power";
    
    backend->beginAction(&opWriteTimedCtx);
    backend->writeData(&opWriteTimedCtx,field,"time", (void *)complexArr, COMPLEX_DATA, 1, dims);
    backend->writeData(&opWriteTimedCtx,timeField,"time", (void *)times, DOUBLE_DATA, 1, dims);
    backend->endAction(&opWriteTimedCtx);
  
    for(int i = 0; i < 20; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	if(i % 2 == 0)
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1]  << ")  Expected value: (" << complexArr[i/2][0] << "," << complexArr[i/2][1] << ")" << std::endl; 
	else
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1]  << ")  Expected value: (" << (complexArr[i/2][0]+ complexArr[i/2+1][0])/2.<< "," << (complexArr[i/2][1]+ complexArr[i/2+1][1])/2. << ")" << std::endl; 
	  
	if(((i % 2 == 0) && (retDoublePtr[0] != complexArr[i/2][0] || retDoublePtr[1] != complexArr[i/2][1]))||
	  ((i % 2 == 1) && (retDoublePtr[0] != (complexArr[i/2][0]+ complexArr[i/2+1][0])/2. || 
	    (retDoublePtr[1] != (complexArr[i/2][1]+ complexArr[i/2+1][1])/2. ))))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
    }
    std::cout << "..." << std::endl;
    for(int i = 5980; i < 6002; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpo, READ_OP, SLICE_OP, (double)i/2., LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
	backend->readData(&sliceReadCtx,field, "time", (void **)&retDoublePtr, &retType, &retNumDims, retDims);
	backend->endAction(&sliceReadCtx);
	  
	if(i % 2 == 0 && i < 5999)
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1]  << ")  Expected value: (" << complexArr[i/2][0] << "," << complexArr[i/2][1] << ")" << std::endl; 
	else if(i < 5999)
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1]  << ")  Expected value: (" << (complexArr[i/2][0]+ complexArr[i/2+1][0])/2.<< "," << (complexArr[i/2][1]+ complexArr[i/2+1][1])/2. << ")" << std::endl; 
	else
	  std::cout << "Time: "<< i/2. << "\tRead Value: (" << retDoublePtr[0] << "," << retDoublePtr[1]  << ")  Expected value: (" << complexArr[2999][0]<< "," << complexArr[2999][1] << ")" << std::endl; 

	if(((i % 2 == 0 && i < 5999) && (retDoublePtr[0] != complexArr[i/2][0] || retDoublePtr[1] != complexArr[i/2][1]))||
	  ((i % 2 == 1 && i < 5999) && (retDoublePtr[0] != (complexArr[i/2][0]+ complexArr[i/2+1][0])/2. || 
	    (retDoublePtr[1] != (complexArr[i/2][1]+ complexArr[i/2+1][1])/2. ))) ||
	  ((i >= 5999) && (retDoublePtr[0] != complexArr[2999][0] || retDoublePtr[1] != complexArr[2999][1])))
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
	free(retDoublePtr);
      }
      std::cout << "------------------> OK " << std::endl;
   



//GABGABGABGABGAB QUI SI PARRA' LA TUA NOBILITATE
    OperationContext opDeleteEquilibriumCtx(pulseCtx, "equilibrium", WRITE_OP);


    std::string cpoAos("equilibrium");
    OperationContext opWriteAosCtx(pulseCtx, cpoAos, WRITE_OP);
    OperationContext opReadAosCtx(pulseCtx, cpoAos, READ_OP);
    OperationContext opReadAosTimedCtx(pulseCtx, cpoAos, READ_OP);


      
    std::cout << "\n\n21 Read/Write non time timedependent array of structures in equilibrium/time_slice" << std::endl;
    std::string fieldStruct("time_slice");
    ArraystructContext opWriteStructCtx(opWriteAosCtx, fieldStruct, "");


    backend->beginAction(&opDeleteEquilibriumCtx); //Dummy to provide context
    backend->deleteData(&opDeleteEquilibriumCtx,"time_slice");
    backend->endAction(&opDeleteEquilibriumCtx);



    backend->beginWriteArraystruct(&opWriteStructCtx,10);
    int intData = 10;
    double doubleData = 20.;
    double doubleData1 = 30.;
    std::string field1("CICCIO");
    std::string field2("BOMBO/DUDU");
    std::string field3("BOMBO/DINDO/BANG");
    std::string field4("BOMBO/DINDO/ARRAYSTRUCT");
    backend->putInArraystruct(&opWriteStructCtx, field1, "", 0, &intData, ualconst::integer_data, 0, dims);
    backend->putInArraystruct(&opWriteStructCtx, field2, "", 0, &doubleData, ualconst::double_data, 0, dims);
    backend->putInArraystruct(&opWriteStructCtx, field3, "", 0, &doubleData1, ualconst::double_data, 0, dims);


    ArraystructContext opRecWriteStructCtx(opWriteAosCtx, field4, "", &opWriteStructCtx, 0);
    backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
    std::string field5("CICCIOBOMBO");
    backend->putInArraystruct(&opRecWriteStructCtx, field5, "", 0, &intData, ualconst::integer_data, 0, dims);
    intData = 11;
    backend->putInArraystruct(&opRecWriteStructCtx, field5, "", 1, &intData, ualconst::integer_data, 0, dims);
    backend->endAction(&opRecWriteStructCtx);
    
    
    
    backend->endAction(&opWriteStructCtx);

   
    std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
    ArraystructContext opReadStructCtx(opReadAosCtx, fieldStruct, "", NULL, 0);
    int retStructSize;
    backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
    intData = -1;
    void *retData;
    backend->getFromArraystruct(&opReadStructCtx, field1, 0, &retData, &retType, &retNumDims, retDims);
    intData = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx, field2, 0, &retData, &retType, &retNumDims, retDims);
    doubleData = *(double *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx, field3, 0, &retData, &retType, &retNumDims, retDims);
    doubleData1 = *(double *)retData;
    free((char *)retData);
 
    ArraystructContext opRecReadStructCtx(opReadAosCtx, field4, "", &opReadStructCtx, 0);
    backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
    std::string field6("CICCIOBOMBO");
    backend->getFromArraystruct(&opRecReadStructCtx, field6, 0, &retData, &retType, &retNumDims, retDims);
    int intdata1 = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opRecReadStructCtx, field6, 1, &retData, &retType, &retNumDims, retDims);
    int intdata2 = *(int *)retData;
    free((char *)retData);
    
    
    backend->endAction(&opRecReadStructCtx);
    backend->endAction(&opReadStructCtx);
    std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
  
   if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    {
      std::cout << "-----------------> FAIL" << std::endl;
      exit(0);
    }
    std::cout << "-----------------> OK" << std::endl;
     
    std::cout << "\n\n22 Write/Read a sequence of array of structure slices in equilibrium/time_slice" << std::endl;

//Prepare time field
    std::string fieldTimeStruct("time");
    std::string fieldAosStruct("time_slice");
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadStructCtx,fieldAosStruct);
    backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, "equilibrium", WRITE_OP, SLICE_OP, (double)i, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
//Gabriele July 2017
	dims[0] = 1;
	backend->writeData(&sliceWriteCtx,"time","time", (void *)&doubleArr[i], DOUBLE_DATA, 1, dims);
//	backend->writeData(&sliceWriteCtx,"time","time", (void *)&doubleArr[i], DOUBLE_DATA, 0, dims);
	backend->endAction(&sliceWriteCtx);
 	std::cout << "Time: "<< (double)i << "\t Written Value: " << doubleArr[i] << std::endl; 
    }



    std::string fieldSliceStruct("time_slice");
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,fieldSliceStruct);
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);

    for(int i = 0; i < 10; i++) //10 Slices
    {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)i*10, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	ArraystructContext opWriteSliceStructCtx(sliceWriteCtx, fieldSliceStruct, "../../time", NULL, 0, true);
	backend->beginWriteArraystruct(&opWriteSliceStructCtx,10);
	
	int intData = 10;
	double doubleData = 20.;
	double doubleData1 = 30.;
	std::string field1("CICCIO");
	std::string field2("BOMBO/DUDU");
	std::string field3("BOMBO/DINDO/BANG");
	//for(int j = 0; j < 3; j++)
	{
	  intData = 10+i*10 ;
	  doubleData = 20+i*10;
	  doubleData1 = 30+i*10;
	  backend->putInArraystruct(&opWriteSliceStructCtx, field1, "", 0, &intData, ualconst::integer_data, 0, dims);
	  backend->putInArraystruct(&opWriteSliceStructCtx, field2, "", 0, &doubleData, ualconst::double_data, 0, dims);
	  backend->putInArraystruct(&opWriteSliceStructCtx, field3, "", 0, &doubleData1, ualconst::double_data, 0, dims);
	  std::cout << "Written [" << i <<"]: CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
	      "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	}
	backend->endAction(&opWriteSliceStructCtx);
	backend->endAction(&sliceWriteCtx);
    }
    for(int i = 0; i < 10; i++)
    {
	OperationContext sliceReadCtx(pulseCtx, cpoAos,  ualconst::read_op, SLICE_OP, (double)i*10, LINEAR_INTERP);
//	backend->beginAction(&sliceReadCtx);
	ArraystructContext opReadStructCtx(sliceReadCtx, fieldSliceStruct, "../../time", NULL, 0,true);
	backend->beginAction(&opReadStructCtx);
	int retStructSize;
	backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
	intData = -1;
	void *retData;
	int intData = 10;
	std::string field1("CICCIO");
	std::string field2("BOMBO/DUDU");
	std::string field3("BOMBO/DINDO/BANG");
	for(int j = 0; j < 1; j++)
	{
	    backend->getFromArraystruct(&opReadStructCtx, field1, j, &retData, &retType, &retNumDims, retDims);
	    intData = *(int *)retData;
    	    free((char *)retData);
	    backend->getFromArraystruct(&opReadStructCtx, field2, j, &retData, &retType, &retNumDims, retDims);
	    doubleData = *(double *)retData;
    	    free((char *)retData);
	    backend->getFromArraystruct(&opReadStructCtx, field3, j, &retData, &retType, &retNumDims, retDims);
	    doubleData1 = *(double *)retData;
    	    free((char *)retData);
	    std::cout << "Read ["<<i<<","<<j << "]: CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl; 
	    if(intData != 10+i*10 + j)
	    {
		std::cout << "ERROR in slice " << i << " arraystruct[" << j <<"] expected " << 10+i*10 + j << " read " << intData << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
	    }
	    if(doubleData != 20+i*10 + j)
	    {
		std::cout << "ERROR in slice " << i << " arraystruct[" << j <<"] expected " << 20+i*10 + j << " read " << doubleData << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
	    }
	    if(doubleData1 != 30+i*10+j)
	    {
		std::cout << "ERROR in slice " << i << " arraystruct[" << j <<"] expected " << 30+i*10+j << " read " << doubleData1 << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
	    }	
	    std::cout << "-----------------> OK" << std::endl;
	}
	backend->endAction(&opReadStructCtx);
	backend->endAction(&sliceReadCtx);
    }


    std::cout << "\n\n23 Write static AOS in equilibrium/time_slice, containing a time dependent signal (timebase in equilibrium/time already set)" << std::endl;

    std::string fieldSliceStruct23("time_slice");
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,fieldSliceStruct23);
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    std::string fieldStruct23("time_slice");
    ArraystructContext opWriteStructCtx23(opWriteAosCtx, fieldStruct23, "");
    backend->beginWriteArraystruct(&opWriteStructCtx23,10);
    intData = 10;
    doubleData = 20.;
    doubleData1 = 30.;
    std::string field231("CICCIO");
    std::string field232("BOMBO/DUDU");
    std::string field233("BOMBO/DINDO/BANG");
    std::string field234("BOMBO/DINDO/ARRAYSTRUCT");
    backend->putInArraystruct(&opWriteStructCtx23, field231, "", 0, &intData, ualconst::integer_data, 0, dims);
    backend->putInArraystruct(&opWriteStructCtx23, field232, "", 0, &doubleData, ualconst::double_data, 0, dims);
    backend->putInArraystruct(&opWriteStructCtx23, field233, "", 0, &doubleData1, ualconst::double_data, 0, dims);


    ArraystructContext opRecWriteStructCtx23(opWriteAosCtx, field234, "", &opWriteStructCtx23, 0);
    backend->beginWriteArraystruct(&opRecWriteStructCtx23,10);
    std::string field235("CICCIOBOMBO");
    std::string field236("SIGNAL");
    backend->putInArraystruct(&opRecWriteStructCtx23, field235, "", 0, &intData, ualconst::integer_data, 0, dims);
    double sig23[] = {2,4,6,8,10,12,14,16,18,20};
    dims[0] = 10;
    intData = 11;
    backend->putInArraystruct(&opRecWriteStructCtx23, field236, "../../../../../time", 0, &sig23, ualconst::double_data, 1, dims);

    backend->putInArraystruct(&opRecWriteStructCtx23, field5, "", 1, &intData, ualconst::integer_data, 0, dims);
    backend->putInArraystruct(&opRecWriteStructCtx23, field236, "../../../../../time", 1, &sig23, ualconst::double_data, 1, dims);
    backend->endAction(&opRecWriteStructCtx23);
    
    backend->endAction(&opWriteStructCtx23);
   
    std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
    ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
    backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
    intData = -1;
    backend->getFromArraystruct(&opReadStructCtx23, field231, 0, &retData, &retType, &retNumDims, retDims);
    intData = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx23, field232, 0, &retData, &retType, &retNumDims, retDims);
    doubleData = *(double *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx23, field233, 0, &retData, &retType, &retNumDims, retDims);
    doubleData1 = *(double *)retData;
    free((char *)retData);
 
    ArraystructContext opRecReadStructCtx23(opReadAosCtx, field234, "", &opReadStructCtx23, 0);
    backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
     backend->getFromArraystruct(&opRecReadStructCtx23, field235, 0, &retData, &retType, &retNumDims, retDims);
    intdata1 = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opRecReadStructCtx23, field236, 0, &retData, &retType, &retNumDims, retDims);
    double *retDoubles0 = (double *)retData;
	


    backend->getFromArraystruct(&opRecReadStructCtx23, field235, 1, &retData, &retType, &retNumDims, retDims);
    intdata2 = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opRecReadStructCtx23, field236, 1, &retData, &retType, &retNumDims, retDims);
    double *retDoubles1 = (double *)retData;
    
    backend->endAction(&opRecReadStructCtx23);
    backend->endAction(&opReadStructCtx23);
    std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
  
   if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    {
      std::cout << "-----------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < 10; i++)
    {
	if(retDoubles0[i] != (i+1)*2 || retDoubles1[i] != (i+1)*2) 
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;
    free((char *)retDoubles0);
    free((char *)retDoubles1);

     
//Same read slice by slice

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i)*10, LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
    	ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
    	backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
    	intData = -1;
    	backend->getFromArraystruct(&opReadStructCtx, field231, 0, &retData, &retType, &retNumDims, retDims);
    	intData = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field232, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData = *(double *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field233, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData1 = *(double *)retData;
    	free((char *)retData);
 
    	ArraystructContext opRecReadStructCtx(sliceReadCtx, field234, "", &opReadStructCtx, 0);
    	backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
     	backend->getFromArraystruct(&opRecReadStructCtx, field235, 0, &retData, &retType, &retNumDims, retDims);
    	intdata1 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 0, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata1 = *(double*)retData;
   	free((char *)retData);
     	backend->getFromArraystruct(&opRecReadStructCtx, field235, 1, &retData, &retType, &retNumDims, retDims);
    	intdata2 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 1, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata2 = *(double*)retData;
    	free((char *)retData);
    
    	backend->endAction(&opRecReadStructCtx);
    	backend->endAction(&opReadStructCtx);
    	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[0]/SIGNAL-> " << aosdoubledata1 << "  BOMBO/DINDO/ARRAYSTRUCT[1]/SIGNAL-> " << aosdoubledata2 <<  std::endl;      
  
   	if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    	{
      	    std::cout << "-----------------> FAIL" << std::endl;
      	    exit(0);
    	}
 	if(aosdoubledata1 != (i+1)*2 || aosdoubledata2 != (i+1)*2) 
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;

    std::cout << "\n\n24Write static AOS in equilibrim/time_slice , containing a time dependent signal slice per slice (timebase in equilbrium/time already set)" << std::endl;

    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,fieldSliceStruct23);
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)i*10, LINEAR_INTERP);
     	ArraystructContext opWriteStructCtx(sliceWriteCtx, fieldStruct23, "");
        backend->beginWriteArraystruct(&opWriteStructCtx,10);
        intData = 10;
        doubleData = 20.;
        doubleData1 = 30.;
        backend->putInArraystruct(&opWriteStructCtx, field231, "", 0, &intData, ualconst::integer_data, 0, dims);
        backend->putInArraystruct(&opWriteStructCtx, field232, "", 0, &doubleData, ualconst::double_data, 0, dims);
        backend->putInArraystruct(&opWriteStructCtx, field233, "", 0, &doubleData1, ualconst::double_data, 0, dims);


        ArraystructContext opRecWriteStructCtx(sliceWriteCtx, field234, "", &opWriteStructCtx, 0);
        backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
    	backend->putInArraystruct(&opRecWriteStructCtx, field235, "", 0, &intData, ualconst::integer_data, 0, dims);
	double currDouble = i;
	dims[0] = 1;
    	backend->putInArraystruct(&opRecWriteStructCtx, field236, "../../../../../time", 0, &currDouble, ualconst::double_data, 1, dims);

	currDouble = i*100;
	intData = 11;
    	backend->putInArraystruct(&opRecWriteStructCtx, field5, "", 1, &intData, ualconst::integer_data, 1, dims);
    	backend->putInArraystruct(&opRecWriteStructCtx, field236, "../../../../../time", 1, &currDouble, ualconst::double_data, 1, dims);
    	backend->endAction(&opRecWriteStructCtx);
    	backend->endAction(&opWriteStructCtx);
    }
   
    std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
//Same as in previous case
    {
   	ArraystructContext opRecReadStructCtx23(opReadAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "", &opReadStructCtx23, 0);
    	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, field231, 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field232, 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field233, 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
	free((char *)retData);
	
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	backend->getFromArraystruct(&opRecReadStructCtx23, field235, 0, &retData, &retType, &retNumDims, retDims);
	intdata1 = *(int *)retData;
	free((char *)retData);
	backend->getFromArraystruct(&opRecReadStructCtx23, field236, 0, &retData, &retType, &retNumDims, retDims);
	retDoubles0 = (double *)retData;
	


	backend->getFromArraystruct(&opRecReadStructCtx23, field235, 1, &retData, &retType, &retNumDims, retDims);
	intdata2 = *(int *)retData;
	free((char *)retData);
	backend->getFromArraystruct(&opRecReadStructCtx23, field236, 1, &retData, &retType, &retNumDims, retDims);
	retDoubles1 = (double *)retData;
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
    }
    std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
  
   if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    {
      std::cout << "-----------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < 10; i++)
    {
	if(retDoubles0[i] != i|| retDoubles1[i] != 100*i) 
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;
    free((char *)retDoubles0);
    free((char *)retDoubles1);

//Same read slice by slice

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i)*10, LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
    	ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
    	backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
    	intData = -1;
    	backend->getFromArraystruct(&opReadStructCtx, field231, 0, &retData, &retType, &retNumDims, retDims);
    	intData = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field232, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData = *(double *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field233, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData1 = *(double *)retData;
    	free((char *)retData);
 
    	ArraystructContext opRecReadStructCtx(sliceReadCtx, field234, "", &opReadStructCtx, 0);
    	backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
     	backend->getFromArraystruct(&opRecReadStructCtx, field235, 0, &retData, &retType, &retNumDims, retDims);
    	intdata1 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 0, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata1 = *(double*)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field235, 1, &retData, &retType, &retNumDims, retDims);
    	intdata2 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 1, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata2 = *(double*)retData;
    	free((char *)retData);
    
    	backend->endAction(&opRecReadStructCtx);
    	backend->endAction(&opReadStructCtx);
    	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[0]/SIGNAL-> " << aosdoubledata1 << "  BOMBO/DINDO/ARRAYSTRUCT[1]/SIGNAL-> " << aosdoubledata2 <<  std::endl;      
  
   	if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    	{
      	    std::cout << "-----------------> FAIL" << std::endl;
      	    exit(0);
    	}
 	if(aosdoubledata1 != i|| aosdoubledata2 != i*100)
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;




    std::cout << "\n\n25Write static AOS in equilibrium/time_slice, containing a time dependent signal slice per slice. The timebase is written slice by slice in a twin node" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,fieldSliceStruct23);
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)i*10, LINEAR_INTERP);
     	ArraystructContext opWriteStructCtx(sliceWriteCtx, fieldStruct23, "");
        backend->beginWriteArraystruct(&opWriteStructCtx,10);
        intData = 10;
        doubleData = 20.;
        doubleData1 = 30.;
        backend->putInArraystruct(&opWriteStructCtx, field231, "", 0, &intData, ualconst::integer_data, 0, dims);
        backend->putInArraystruct(&opWriteStructCtx, field232, "", 0, &doubleData, ualconst::double_data, 0, dims);
        backend->putInArraystruct(&opWriteStructCtx, field233, "", 0, &doubleData1, ualconst::double_data, 0, dims);


        ArraystructContext opRecWriteStructCtx(sliceWriteCtx, field234, "", &opWriteStructCtx, 0);
        backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
    	backend->putInArraystruct(&opRecWriteStructCtx, field235, "", 0, &intData, ualconst::integer_data, 0, dims);
	double currDouble = i;
    	backend->putInArraystruct(&opRecWriteStructCtx, field236, "timebase", 0, &currDouble, ualconst::double_data, 0, dims);
	currDouble = 10*i;
    	backend->putInArraystruct(&opRecWriteStructCtx, "timebase", "timebase", 0, &currDouble, ualconst::double_data, 0, dims);

	currDouble = i*100;
	intData = 11;
    	backend->putInArraystruct(&opRecWriteStructCtx, field5, "", 1, &intData, ualconst::integer_data, 0, dims);
    	backend->putInArraystruct(&opRecWriteStructCtx, field236, "timebase", 1, &currDouble, ualconst::double_data, 0, dims);
	currDouble = 10*i;
    	backend->putInArraystruct(&opRecWriteStructCtx, "timebase", "timebase", 1, &currDouble, ualconst::double_data, 0, dims);
    	backend->endAction(&opRecWriteStructCtx);
    	backend->endAction(&opWriteStructCtx);
    }
   
    std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
//Same as in previous case
    backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
    intData = -1;
    backend->getFromArraystruct(&opReadStructCtx23, field231, 0, &retData, &retType, &retNumDims, retDims);
    intData = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx23, field232, 0, &retData, &retType, &retNumDims, retDims);
    doubleData = *(double *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opReadStructCtx23, field233, 0, &retData, &retType, &retNumDims, retDims);
    doubleData1 = *(double *)retData;
    free((char *)retData);
 
    backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
    backend->getFromArraystruct(&opRecReadStructCtx23, field235, 0, &retData, &retType, &retNumDims, retDims);
    intdata1 = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opRecReadStructCtx23, field236, 0, &retData, &retType, &retNumDims, retDims);
    retDoubles0 = (double *)retData;
 	


    backend->getFromArraystruct(&opRecReadStructCtx23, field235, 1, &retData, &retType, &retNumDims, retDims);
    intdata2 = *(int *)retData;
    free((char *)retData);
    backend->getFromArraystruct(&opRecReadStructCtx23, field236, 1, &retData, &retType, &retNumDims, retDims);
    retDoubles1 = (double *)retData;
    
    backend->endAction(&opRecReadStructCtx23);
    backend->endAction(&opReadStructCtx23);
    std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
  
   if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    {
      std::cout << "-----------------> FAIL" << std::endl;
      exit(0);
    }
    for(int i = 0; i < 10; i++)
    {
	if(retDoubles0[i] != i|| retDoubles1[i] != 100*i) 
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;

   free((char *)retDoubles0);
   free((char *)retDoubles1);
//Same read slice by slice

    for(int i = 0; i < 10; i++)
    {
        OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i)*10, LINEAR_INTERP);
	backend->beginAction(&sliceReadCtx);
    	ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
    	backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
    	intData = -1;
    	backend->getFromArraystruct(&opReadStructCtx, field231, 0, &retData, &retType, &retNumDims, retDims);
    	intData = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field232, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData = *(double *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opReadStructCtx, field233, 0, &retData, &retType, &retNumDims, retDims);
    	doubleData1 = *(double *)retData;
    	free((char *)retData);
 
    	ArraystructContext opRecReadStructCtx(sliceReadCtx, field234, "", &opReadStructCtx, 0);
    	backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
     	backend->getFromArraystruct(&opRecReadStructCtx, field235, 0, &retData, &retType, &retNumDims, retDims);
    	intdata1 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 0, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata1 = *(double*)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field235, 1, &retData, &retType, &retNumDims, retDims);
    	intdata2 = *(int *)retData;
    	free((char *)retData);
    	backend->getFromArraystruct(&opRecReadStructCtx, field236, 1, &retData, &retType, &retNumDims, retDims);
	if(retNumDims > 1)
	{
	    std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
	    std::cout << "-----------------> FAIL" << std::endl;
	    exit(0);
	}
	double aosdoubledata2 = *(double*)retData;
    	free((char *)retData);
    
    	backend->endAction(&opRecReadStructCtx);
    	backend->endAction(&opReadStructCtx);
    	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
        "  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[0]/SIGNAL-> " << aosdoubledata1 << "  BOMBO/DINDO/ARRAYSTRUCT[1]/SIGNAL-> " << aosdoubledata2 <<  std::endl;      
  
   	if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| intdata1 != 10 || intdata2 != 11)
    	{
      	    std::cout << "-----------------> FAIL" << std::endl;
      	    exit(0);
    	}
 	if(aosdoubledata1 != i|| aosdoubledata2 != i*100)
	{
	  std::cout << "-----------------> FAIL" << std::endl;
	  exit(0);
	}
    }
    std::cout << "-----------------> OK" << std::endl;


    std::cout << "\n\n26 Write static AOS in equilibrium/time_slice, containing a time dependent dynamic AoS (timebase in equilibrium/time already set)" << std::endl;

#ifndef MEMORY_BACKEND

    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif

    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {
	std::string fieldStruct23("time_slice");
	ArraystructContext opWriteStructCtx23(opWriteAosCtx, fieldStruct23, "");
	backend->beginWriteArraystruct(&opWriteStructCtx23,10);
	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	std::string field231("CICCIO");
	std::string field232("BOMBO/DUDU");
	std::string field233("BOMBO/DINDO/BANG");
	std::string field234("BOMBO/DINDO/ARRAYSTRUCT");
	backend->putInArraystruct(&opWriteStructCtx23, field231, "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteStructCtx23, field232, "", 0, &doubleData, ualconst::double_data, 0, dims);
	backend->putInArraystruct(&opWriteStructCtx23, field233, "", 0, &doubleData1, ualconst::double_data, 0, dims);
	
	ArraystructContext opRecWriteStructCtx23(opWriteAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../../../../time", &opWriteStructCtx23, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx23,10);
	for(int i = 0; i < 10; i++)
	{
	    std::string field235("CICCIOBOMBO");
	    std::string field236("SIGNAL");
	    double currDouble = 11*i;
	    int intData = 10*i;
	    backend->putInArraystruct(&opRecWriteStructCtx23, field235, "", i, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx23, field236, "", i, &currDouble, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx23);
	
	backend->endAction(&opWriteStructCtx23);
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, field231, 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
    	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field232, 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
    	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field233, 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
    	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, field234, "../../../../../time", &opReadStructCtx23, 0, true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, field235, i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
    		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, field236, i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
   		free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i)*10, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, field231, 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, field232, 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, field233, 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
   		free((char *)retData);
	
		ArraystructContext opRecReadStructCtx(sliceReadCtx, field234, "../../../../../time", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, field235, 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, field236, 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
   		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }
    std::cout << "\n\n27 Write static AOS in equilibrium/time_slice , containing a time dependent dynamic AoS. The timebase is written in a twin node" << std::endl;




#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);

    {
	std::string fieldStruct23("time_slice");
	ArraystructContext opWriteStructCtx23(opWriteAosCtx, fieldStruct23, "");
	backend->beginWriteArraystruct(&opWriteStructCtx23,10);
	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	backend->putInArraystruct(&opWriteStructCtx23, "CICCIO", "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/DUDU", "", 0, &doubleData, ualconst::double_data, 0, dims);
	
	double timebase[10] = {100,200, 300,400, 500,600, 700,800, 900,1000}; 
	dims[0] = 10;
	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/TIMEBASE", "BOMBO/TIMEBASE", 0, timebase, ualconst::double_data, 1, dims);

	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/DINDO/BANG", "", 0, &doubleData1, ualconst::double_data, 0, dims);


	ArraystructContext opRecWriteStructCtx23(opWriteAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../TIMEBASE", &opWriteStructCtx23, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx23,10);
	for(int i = 0; i < 10; i++)
	{
	    double currDouble = 11*i;
	    int intData = 10*i;
	    backend->putInArraystruct(&opRecWriteStructCtx23, "CICCIOBOMBO", "", i, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx23, "SIGNAL", "", i, &currDouble, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx23);
	
	backend->endAction(&opWriteStructCtx23);
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
   	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
   	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
   	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../TIMEBASE", &opReadStructCtx23, 0,true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
   		free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i+1)*100, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
   		free((char *)retData);
		ArraystructContext opRecReadStructCtx(sliceReadCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../TIMEBASE", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
   		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }

    std::cout << "\n\n28 Write static AOS in equilibrium/time_slice, containing a time dependent dynamic AoS. The timebase is written in the Dynamic AoS itsefle (field \"time\")" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {
	std::string fieldStruct23("time_slice");
	ArraystructContext opWriteStructCtx23(opWriteAosCtx, fieldStruct23, "");
	backend->beginWriteArraystruct(&opWriteStructCtx23,10);
	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	backend->putInArraystruct(&opWriteStructCtx23, "CICCIO", "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/DUDU", "", 0, &doubleData, ualconst::double_data, 0, dims);
	
	double timebase[10] = {100,200, 300,400, 500,600, 700,800, 900,1000}; 
	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/TIMEBASEOLD", "BOMBO/TIMEBASEOLD", 0, timebase, ualconst::double_data, 1, dims); //not used hiowever as timebae

	backend->putInArraystruct(&opWriteStructCtx23, "BOMBO/DINDO/BANG", "", 0, &doubleData1, ualconst::double_data, 0, dims);


	ArraystructContext opRecWriteStructCtx23(opWriteAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opWriteStructCtx23, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx23,10);
	for(int i = 0; i < 10; i++)
	{
	    double currTime = 100*i;
	    double currDouble = 11*i;
	    int intData = 10*i;
	    backend->putInArraystruct(&opRecWriteStructCtx23, "CICCIOBOMBO", "", i, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx23, "SIGNAL", "", i, &currDouble, ualconst::double_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx23, "time", "", i, &currTime, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx23);
	
	backend->endAction(&opWriteStructCtx23);
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
   	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
   	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
   	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opReadStructCtx23, 0, true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
	   	free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)i*100, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
   		free((char *)retData);
	
		ArraystructContext opRecReadStructCtx(sliceReadCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
   		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }







    std::cout << "\n\n29 Write slices of static AOS in equilibrium/time_slice, containing a time dependent dynamic AoS (timebase in equilibrium/time already set)" << std::endl;

#ifndef MEMORY_BACKEND

    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif

    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {
      for(int sliceIdx = 0; sliceIdx < 10; sliceIdx++)
      {

        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)sliceIdx*10, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	ArraystructContext opWriteSliceStructCtx(sliceWriteCtx, "time_slice", "", NULL, 0, false);
	backend->beginWriteArraystruct(&opWriteSliceStructCtx,10);

	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	std::string field231("CICCIO");
	std::string field232("BOMBO/DUDU");
	std::string field233("BOMBO/DINDO/BANG");
	std::string field234("BOMBO/DINDO/ARRAYSTRUCT");
	backend->putInArraystruct(&opWriteSliceStructCtx, field231, "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteSliceStructCtx, field232, "", 0, &doubleData, ualconst::double_data, 0, dims);
	backend->putInArraystruct(&opWriteSliceStructCtx, field233, "", 0, &doubleData1, ualconst::double_data, 0, dims);
	
	ArraystructContext opRecWriteStructCtx(sliceWriteCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../../../../../time", &opWriteSliceStructCtx, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
	//for(int i = 0; i < 10; i++)
	{
	    std::string field235("CICCIOBOMBO");
	    std::string field236("SIGNAL");
	    double currDouble = 11*sliceIdx;
	    int intData = 10*sliceIdx;
	    backend->putInArraystruct(&opRecWriteStructCtx, field235, "", 0, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx, field236, "", 0, &currDouble, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx);
	
	backend->endAction(&opWriteSliceStructCtx);
      }
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      

	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, field231, 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
   	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field232, 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
  	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, field233, 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
  	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, field234, "../../../../../../time", &opReadStructCtx23, 0, true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, field235, i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
	  	free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, field236, i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
 	 	free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i)*10, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, field231, 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, field232, 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, field233, 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
  		free((char *)retData);
	
		ArraystructContext opRecReadStructCtx(sliceReadCtx, field234, "../../../../../../time", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, field235, 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, field236, 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
  		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }











    std::cout << "\n\n30 Write static AOS in equilibrium/time_slice, containing a time dependent dynamic AoS. The timebase is written in a twin node" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {

      for(int sliceIdx = 0; sliceIdx < 10; sliceIdx++)
      {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)(sliceIdx+1)*100, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	ArraystructContext opWriteSliceStructCtx(sliceWriteCtx, "time_slice", "", NULL, 0, false);
	backend->beginWriteArraystruct(&opWriteSliceStructCtx,10);

	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	backend->putInArraystruct(&opWriteSliceStructCtx, "CICCIO", "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/DUDU", "", 0, &doubleData, ualconst::double_data, 0, dims);
	
	//double timebase[10] = {100,200, 300,400, 500,600, 700,800, 900,1000}; 
	double currTime = (sliceIdx+1)*100;
	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/TIMEBASE", "BOMBO/TIMEBASE", 0, &currTime, ualconst::double_data, 0, dims);

	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/DINDO/BANG", "", 0, &doubleData1, ualconst::double_data, 0, dims);


	ArraystructContext opRecWriteStructCtx(sliceWriteCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../TIMEBASE", &opWriteSliceStructCtx, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
	//for(int i = 0; i < 10; i++)
	{
	    double currDouble = 11*sliceIdx;
	    int intData = 10*sliceIdx;
	    backend->putInArraystruct(&opRecWriteStructCtx, "CICCIOBOMBO", "", 0, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx, "SIGNAL", "", 0, &currDouble, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx);
	
	backend->endAction(&opWriteSliceStructCtx);
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
      }
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
  	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
  	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
  	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../TIMEBASE", &opReadStructCtx23, 0,true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
  		free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)(i+1)*100, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
  		free((char *)retData);
	
		ArraystructContext opRecReadStructCtx(sliceReadCtx, "BOMBO/DINDO/ARRAYSTRUCT", "../../timebase", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
  		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }

    std::cout << "\n\n31 Write AOS in equilibrium/time_slice, containing a time dependent dynamic AoS. The timebase is written in the Dynamic AoS itself (field \"TIME\")" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {
      for(int sliceIdx = 0; sliceIdx < 10; sliceIdx++)
      {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)(sliceIdx+1)*100, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	ArraystructContext opWriteSliceStructCtx(sliceWriteCtx, "time_slice", "", NULL, 0, false);
	backend->beginWriteArraystruct(&opWriteSliceStructCtx,10);

	intData = 10;
	doubleData = 20.;
	doubleData1 = 30.;
	backend->putInArraystruct(&opWriteSliceStructCtx, "CICCIO", "", 0, &intData, ualconst::integer_data, 0, dims);
	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/DUDU", "", 0, &doubleData, ualconst::double_data, 0, dims);
	
	double timebase[10] = {100,200, 300,400, 500,600, 700,800, 900,1000}; 
	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/TIMEBASEOLD", "BOMBO/TIMEBASEOLD", 0, timebase, ualconst::double_data, 1, dims); //not used hiowever as timebae

	backend->putInArraystruct(&opWriteSliceStructCtx, "BOMBO/DINDO/BANG", "", 0, &doubleData1, ualconst::double_data, 0, dims);


	ArraystructContext opRecWriteStructCtx(sliceWriteCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opWriteSliceStructCtx, 0,true);
	backend->beginWriteArraystruct(&opRecWriteStructCtx,10);
	//for(int i = 0; i < 10; i++)
	{
	    double currTime = 100*sliceIdx;
	    double currDouble = 11*sliceIdx;
	    int intData = 10*sliceIdx;
	    backend->putInArraystruct(&opRecWriteStructCtx, "CICCIOBOMBO", "", 0, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx, "SIGNAL", "", 0, &currDouble, ualconst::double_data, 0, dims);
	    backend->putInArraystruct(&opRecWriteStructCtx, "time", "", 0, &currTime, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opRecWriteStructCtx);
	
	backend->endAction(&opWriteSliceStructCtx);
	std::cout << "Written " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
      }
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	intData = -1;
	backend->getFromArraystruct(&opReadStructCtx23, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
	intData = *(int *)retData;
  	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
	doubleData = *(double *)retData;
  	free((char *)retData);
	backend->getFromArraystruct(&opReadStructCtx23, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
	doubleData1 = *(double *)retData;
  	free((char *)retData);
	
	ArraystructContext opRecReadStructCtx23(opReadAosCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opReadStructCtx23, 0, true);
	backend->beginReadArraystruct(&opRecReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opRecReadStructCtx23, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx23, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
	  	free((char *)retData);
	}
	
	backend->endAction(&opRecReadStructCtx23);
	backend->endAction(&opReadStructCtx23);
	std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << std::endl;      
	
	if(intData != 10 || doubleData != 20. || doubleData1 != 30.)
	{
	std::cout << "-----------------> FAIL" << std::endl;
	exit(0);
	}
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)i*100, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "", NULL, 0);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		intData = -1;
		backend->getFromArraystruct(&opReadStructCtx, "CICCIO", 0, &retData, &retType, &retNumDims, retDims);
		intData = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DUDU", 0, &retData, &retType, &retNumDims, retDims);
		doubleData = *(double *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "BOMBO/DINDO/BANG", 0, &retData, &retType, &retNumDims, retDims);
		doubleData1 = *(double *)retData;
  		free((char *)retData);
	
		ArraystructContext opRecReadStructCtx(sliceReadCtx, "BOMBO/DINDO/ARRAYSTRUCT", "time", &opReadStructCtx, 0,true);
		backend->beginReadArraystruct(&opRecReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opRecReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opRecReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
  		free((char *)retData);
	
		backend->endAction(&opRecReadStructCtx);
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIO-> " << intData << "  BOMBO/DUDU-> " << doubleData << 
		"  BOMBO/DINDO/BANG-> " << doubleData1 << "  BOMBO/DINDO/ARRAYSTRUCT[i]/CICCIOBOMBO-> " << retInt[i] << "  BOMBO/DINDO/ARRAYSTRUCT[i]/SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(intData != 10 || doubleData != 20. || doubleData1 != 30.|| retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }





    std::cout << "\n\n32Write  a time dependent dynamic AoS (Not Static Root). The timebase is written in the Dynamic AoS itsefle (field \"time\")" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {

	ArraystructContext opWriteStructCtx(opWriteAosCtx, "time_slice", "time", NULL, 0, true);
	backend->beginWriteArraystruct(&opWriteStructCtx,10);
	for(int i = 0; i < 10; i++)
	{
	    double currTime = 100*i;
	    double currDouble = 11*i;
	    int intData = 10*i;
	    backend->putInArraystruct(&opWriteStructCtx, "CICCIOBOMBO", "", i, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opWriteStructCtx, "SIGNAL", "", i, &currDouble, ualconst::double_data, 0, dims);
	    backend->putInArraystruct(&opWriteStructCtx, "time", "", i, &currTime, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opWriteStructCtx);

	ArraystructContext opReadStructCtx(opReadAosCtx, fieldStruct, "time", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opReadStructCtx, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
	   	free((char *)retData);
	}
	
	backend->endAction(&opReadStructCtx);

	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)i*100, LINEAR_INTERP);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "time", NULL, 0, true);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);

		backend->getFromArraystruct(&opReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
   		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
   		free((char *)retData);
	
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << " CICCIOBOMBO-> " << retInt[i] << "  SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }





    std::cout << "\n\n33 Write time dependent dynamic AoS (No containing static AoS). The timebase is written in the Dynamic AoS itsefle (field \"time\"" << std::endl;

#ifndef MEMORY_BACKEND
    ((MDSplusBackend *)backend)->fullResetNodePath();
#endif
    backend->beginAction(&opReadAosTimedCtx); //Dummy to provide context
    backend->deleteData(&opReadAosTimedCtx,"time_slice");
    //backend->deleteData(&opReadAosTimedCtx,timeField);
    backend->endAction(&opReadAosTimedCtx);


    {
      for(int sliceIdx = 0; sliceIdx < 10; sliceIdx++)
      {
        OperationContext sliceWriteCtx(pulseCtx, cpoAos,  WRITE_OP, SLICE_OP, (double)(sliceIdx+1)*100, LINEAR_INTERP);
	backend->beginAction(&sliceWriteCtx);
	ArraystructContext opWriteSliceStructCtx(sliceWriteCtx, "time_slice", "time", NULL, 0, true);
	backend->beginWriteArraystruct(&opWriteSliceStructCtx,10);
	//for(int i = 0; i < 10; i++)
	{
	    double currTime = 100*sliceIdx;
	    double currDouble = 11*sliceIdx;
	    int intData = 10*sliceIdx;
	    backend->putInArraystruct(&opWriteSliceStructCtx, "CICCIOBOMBO", "", 0, &intData, ualconst::integer_data, 0, dims);
	    backend->putInArraystruct(&opWriteSliceStructCtx, "SIGNAL", "", 0, &currDouble, ualconst::double_data, 0, dims);
	    backend->putInArraystruct(&opWriteSliceStructCtx, "time", "", 0, &currTime, ualconst::double_data, 0, dims);
	}
	backend->endAction(&opWriteSliceStructCtx);
	
      }
	ArraystructContext opReadStructCtx23(opReadAosCtx, fieldStruct, "time", NULL, 0);
	backend->beginReadArraystruct(&opReadStructCtx23, &retStructSize);
	int retInt[10];
	double retDouble[10];
	for(int i = 0; i <10; i++)
	{
		backend->getFromArraystruct(&opReadStructCtx23, "CICCIOBOMBO", i, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx23, "SIGNAL", i, &retData, &retType, &retNumDims, retDims);
		retDouble[i] = *(double *)retData;
	  	free((char *)retData);
	}
	
	backend->endAction(&opReadStructCtx23);
	for(int i = 0; i < 10; i++)
	{
		if(retInt[i] != 10*i || retDouble[i] != 11*i) 
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
		
	
	//Same read slice by slice
	
	for(int i = 0; i < 10; i++)
	{
		OperationContext sliceReadCtx(pulseCtx, cpoAos, READ_OP, SLICE_OP, (double)i*100, LINEAR_INTERP);
		backend->beginAction(&sliceReadCtx);
		ArraystructContext opReadStructCtx(sliceReadCtx, fieldStruct, "time", NULL, 0, true);
		backend->beginReadArraystruct(&opReadStructCtx, &retStructSize);
		backend->getFromArraystruct(&opReadStructCtx, "CICCIOBOMBO", 0, &retData, &retType, &retNumDims, retDims);
		retInt[i] = *(int *)retData;
  		free((char *)retData);
		backend->getFromArraystruct(&opReadStructCtx, "SIGNAL", 0, &retData, &retType, &retNumDims, retDims);
		if(retNumDims > 1)
		{
		std::cout << "Invalid dimension returned in get AoS Slice" << std::endl;
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
		retDouble[i] = *(double*)retData;
  		free((char *)retData);
	
		backend->endAction(&opReadStructCtx);
		std::cout << "Read " << "CICCIOBOMBO-> " << retInt[i] << "  SIGNAL-> " << retDouble[i] <<  std::endl;      
	
		if(retInt[i] != 10*i || retDouble[i] != 11*i)
		{
		std::cout << "-----------------> FAIL" << std::endl;
		exit(0);
		}
	}
	std::cout << "-----------------> OK" << std::endl;
	
    }


















    
  }catch(UALException &exc)
  {
    std::cout << "Error :" << exc.what() << std::endl;
  }  
  
  
 
  
  
    return 0;
}    
