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

static const int ASCII_BACKEND_VERSION_MAJOR = 0;
static const int ASCII_BACKEND_VERSION_MINOR = 0;

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus


class LIBRARY_API AsciiBackend : public Backend
{

private:
  std::fstream pulsefile;
  //std::string prefix;
  //std::string suffix;
  std::string fullpath;
  std::string dbname;
  std::string fname; 
  bool writemode;
  std::string idsname;
  std::stringstream curcontent;
  std::map<std::string, std::streampos> curcontent_map;
  std::string curline;
  
  std::string getArraystructPath(ArraystructContext *aosctx);

  void writeData(const char* data, int dim, int* size);
  // extern "C" do not permit to declare template functions
  //template <typename T> 
  //void writeData(const T* data, int dim, int* size);
  void writeData(const int* data, int dim, int* size);
  void writeData(const double* data, int dim, int* size);
  void writeData(const std::complex<double>* data, int dim, int* size);

  void readData(char** data, int dim, int* size);
  // extern "C" do not permit to declare template functions
  //template <typename T> 
  //void readData(T** data, int dim, int* size);
  void readData(int** data, int dim, int* size);
  void readData(double** data, int dim, int* size);
  void readData(std::complex<double>** data, int dim, int* size);


public:

  AsciiBackend();
  virtual ~AsciiBackend() {};

  virtual void openPulse(DataEntryContext *ctx,
			 int mode);

  virtual void closePulse(DataEntryContext *ctx,
			  int mode);

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

  virtual std::pair<int,int> getVersion(DataEntryContext *ctx);

};

#endif

#endif // ASCII_BACKEND_H
