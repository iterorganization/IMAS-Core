//-*-c++-*-

#ifndef NO_BACKEND_H
#define NO_BACKEND_H 1

#include "al_backend.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus


/**
   No backend (test) implementation.
*/
class LIBRARY_API NoBackend : public Backend
{
private:
  bool verbose = false;

public:
  
  NoBackend(bool verb=false);

  ~NoBackend() {};

  void openPulse(DataEntryContext *ctx,
		 int mode);

  void closePulse(DataEntryContext *ctx,
		  int mode);

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

  std::pair<int,int> getVersion(DataEntryContext *ctx);

  void get_occurrences(const  char* ids_name, int** occurrences_list, int* size) override;

  bool performsTimeDataInterpolation() {
      return false;
    }

  bool supportsTimeRangeOperation() {
	  return false;
	}

  void setDataInterpolationComponent(DataInterpolation *component) {
      throw ALBackendException("NoBackend backend does not support time slices operations",LOG);
  }

};

#endif

#endif // NO_BACKEND_H
