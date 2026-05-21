//-*-c++-*-

#ifndef NO_BACKEND_H
#define NO_BACKEND_H 1

#include "al_backend.h"

#if defined(_WIN32)
#  define IMAS_CORE_LIBRARY_API __declspec(dllexport)
#else
#  define IMAS_CORE_LIBRARY_API
#endif

#ifdef __cplusplus


/**
   No backend (test) implementation.
*/
class IMAS_CORE_LIBRARY_API NoBackend : public Backend
{
private:
  bool verbose = false;

public:
  
  NoBackend(bool verb=false);

  ~NoBackend() {};

  void openPulse(DataEntryContext *ctx,
		 int mode) override;

  void closePulse(DataEntryContext *ctx,
		  int mode) override;

  void beginAction(OperationContext *ctx) override;

  void endAction(Context *ctx) override;

  void writeData(Context *ctx,
		 std::string fieldname,
		 std::string timebasename,
		 void* data,
		 int datatype,
		 int dim,
		 int* size) override;

  int readData(Context *ctx,
	       std::string fieldname,
	       std::string timebasename,
	       void** data,
	       int* datatype,
	       int* dim,
	       int* size) override;

  void deleteData(OperationContext *ctx,
		  std::string path) override;

  void beginArraystructAction(ArraystructContext *ctx,
			      int *size) override;

  std::pair<int,int> getVersion(DataEntryContext *ctx) override;

  void get_occurrences(Context* ctx, const  char* ids_name, int** occurrences_list, int* size) override;

  void list_filled_paths(Context* ctx, const char* dataobjectname, char*** path_list, int* size) override;

  bool supportsTimeDataInterpolation() override {
      return false;
    }

  void initDataInterpolationComponent() override {
  }

  bool supportsTimeRangeOperation() override {
	  return false;
	}

};

#endif

#endif // NO_BACKEND_H
