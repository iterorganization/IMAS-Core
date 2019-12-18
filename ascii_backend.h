//-*-c++-*-

/**
   \file ascii_backend.h
   Contains definition of simple ASCII Backend classe.
*/

#ifndef ASCII_BACKEND_H
#define ASCII_BACKEND_H 1

#include "ual_backend.h"
#include <fstream>
#include <complex>

/* c++ only part */
#if defined(__cplusplus)


class AsciiBackend : public Backend
{

private:
  std::fstream pulsefile;
  std::string dbname;
  std::string fname; 
  bool writemode;
  std::string idsname;
  std::stringstream curcontent;
  std::string curline;
  
  std::string getArraystructPath(ArraystructContext *aosctx);

  void writeData(const char* data, int dim, int* size);
  template <typename T> 
  void writeData(const T* data, int dim, int* size);

  void readData(char** data, int dim, int* size);
  template <typename T> 
  void readData(T** data, int dim, int* size);


public:

  AsciiBackend();
  virtual ~AsciiBackend() {};

  virtual void openPulse(PulseContext *ctx,
			 int mode,
			 std::string options);

  virtual void closePulse(PulseContext *ctx,
			  int mode,
			  std::string options);

  virtual void beginAction(OperationContext *ctx);

  virtual void endAction(Context *ctx); 

  virtual void writeData(Context *ctx,
			 std::string fieldname,
			 std::string timebasename, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size);

  virtual int readData(Context *ctx,
		       std::string fieldname,
		       std::string timebasename, 
		       void** data,
		       int* datatype,
		       int* dim,
		       int* size);

  virtual void deleteData(OperationContext *ctx,
			  std::string path);

  virtual void beginArraystructAction(ArraystructContext *ctx,
				      int *size);

};

#endif 

#endif
