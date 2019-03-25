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
  
  NoBackend(bool verb=false);

  ~NoBackend() {};

  void openPulse(PulseContext *ctx,
		 int mode,
		 std::string options);

  void closePulse(PulseContext *ctx,
		  int mode,
		  std::string options);

  void beginAction(OperationContext *ctx);

  void endAction(Context *ctx);

  void writeData(Context *ctx,
		 std::string fieldname,
		 std::string timebasename,
		 void* data,
		 int datatype,
		 int dim,
		 int* size);

  int readData(Context *ctx,
	       std::string fieldname,
	       std::string timebasename,
	       void** data,
	       int* datatype,
	       int* dim,
	       int* size);

  void deleteData(OperationContext *ctx,
		  std::string path);

  void beginArraystructAction(ArraystructContext *ctx,
			      int *size);


};


#endif
