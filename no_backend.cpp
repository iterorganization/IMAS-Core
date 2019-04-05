#include "no_backend.h"


NoBackend::NoBackend(bool verb) : verbose(verb) 
{
}

void NoBackend::openPulse(PulseContext *ctx,
			  int mode,
			  std::string options) 
{
  if (verbose)
    std::cout << "NoBackend openPulse\n";
}

void NoBackend::closePulse(PulseContext *ctx,
			   int mode,
			   std::string options) 
{
  if (verbose)
    std::cout << "NoBackend closePulse\n";
}

void NoBackend::beginAction(OperationContext *ctx) 
{
  if (verbose)
    std::cout << "NoBackend beginAction\n";
}

void NoBackend::endAction(Context *ctx) 
{
  if (verbose)
    std::cout << "NoBackend endAction\n";
} 

void NoBackend::writeData(Context *ctx,
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

int NoBackend::readData(Context *ctx,
			std::string fieldname,
			std::string timebasename,
			void** data,
			int* datatype,
			int* dim,
			int* size) 
{
  if (verbose)
    std::cout << "NoBackend readData\n";
  return 0;
}

void NoBackend::deleteData(OperationContext *ctx,
			   std::string path)
{
  if (verbose)
    std::cout << "NoBackend deleteData\n";
}

void NoBackend::beginArraystructAction(ArraystructContext *ctx,
				       int *size) 
{
  if (verbose)
    std::cout << "NoBackend beginWriteArraystruct\n";
}

