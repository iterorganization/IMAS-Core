//-*-c++-*-

#ifndef NO_BACKEND_H
#define NO_BACKEND_H 1

#include "ual_backend.h"

/**
   No backend (test) implementation.
*/
class NoBackend : public Backend
{
private:
  bool verbose = false;

public:
  
  NoBackend(bool verb=false) : verbose(verb)
  {
    if (verbose)
      std::cout << "NoBackend constructor\n";
  }
  
  ~NoBackend() 
  {
    if (verbose)
      std::cout << "NoBackend destructor\n";
  }

  void openPulse(PulseContext *ctx,
		 int mode,
		 std::string options) 
  {
    if (verbose)
      std::cout << "NoBackend openPulse\n";
  }

  void closePulse(PulseContext *ctx,
		  int mode,
		  std::string options) 
  {
    if (verbose)
      std::cout << "NoBackend closePulse\n";
  }

  void beginAction(OperationContext *ctx) 
  {
    if (verbose)
      std::cout << "NoBackend beginAction\n";
  }

  void endAction(Context *ctx) 
  {
    if (verbose)
      std::cout << "NoBackend endAction\n";
  } 

  void writeData(Context *ctx,
		 std::string fieldname,
		 std::string timebasename,
		 void* data,
		 int datatype,
		 int dim,
		 int* size) 
  {
    if (verbose)
      std::cout << "NoBackend writeData\n";
  }

  int readData(Context *ctx,
		std::string fieldname,
		std::string timebasename,
		void** data,
		int* datatype,
		int* dim,
		int* size) 
  {
    if (verbose)
      std::cout << "NoBackend readData, test scalar\n";
    
    if (fieldname=="path/to/double") 
      {
	**(double**)data = 123.45;
	*datatype = DOUBLE_DATA;
	*dim = 0;
	size[0] = 0; // size is optional with scalar
      }
    else if (fieldname=="path/to/int") 
      {
	**(int**)data = 42;
	*datatype = INTEGER_DATA;
	*dim = 0;
	size[0] = 0; // size is optional with scalar
      }
    else
      {
	if (verbose)
	  std::cout << "NoBackend readData\n";
	return 0;
	//throw UALBackendException("test unrecoverable exception",LOG);
      }
     return 1;
  }

  void deleteData(OperationContext *ctx,
		  std::string path)
  {
    if (verbose)
      std::cout << "NoBackend deleteData\n";
  }

  void beginArraystructAction(ArraystructContext *ctx,
			      int *size) 
  {
    if (verbose)
      std::cout << "NoBackend beginWriteArraystruct\n";
  }


};


#endif
