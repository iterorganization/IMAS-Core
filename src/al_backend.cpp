#include "al_backend.h"
#include "no_backend.h"
#include "memory_backend.h"
#ifdef ASCII
#include "ascii_backend.h"
#endif
#ifdef MDSPLUS
#include "mdsplus_backend.h"
#endif
#ifdef HDF5
#include "hdf5_backend.h"
#endif
#ifdef UDA
#include "uda_backend.h"
#endif
#include "flexbuffers_backend.h"

#include "data_interpolation.h"


#if !defined(__GNUC__) && !defined(__clang__)
#define strcasecmp _stricmp
#define strtok_r strtok_s
#endif


Backend* Backend::initBackend(DataEntryContext *ctx)
{
  Backend *be = NULL;
  int id = ctx->getBackendID();

  if (id==alconst::no_backend)
    {
      NoBackend* tbe = new NoBackend();
      be = tbe;
    }
  else if (id==alconst::ascii_backend)
    {
#ifdef ASCII
      AsciiBackend* tbe = new AsciiBackend();
      be = tbe;
#else
      throw ALBackendException("ASCII backend is not available within current install",LOG);
#endif
    }
  else if (id==alconst::mdsplus_backend)
    {
#ifdef MDSPLUS
      MDSplusBackend* tbe = new MDSplusBackend();
      be = tbe;
#else
      throw ALBackendException("MDSplus backend is not available within current install",LOG);
#endif
    }
  else if (id==alconst::hdf5_backend)
    {
#ifdef HDF5
      HDF5Backend* tbe = new HDF5Backend();
      be = tbe;
#else
      throw ALBackendException("HDF5 backend is not available within current install",LOG);
#endif
    }
  else if (id==alconst::memory_backend)
    {
      MemoryBackend* tbe = new MemoryBackend();
      be = tbe;
    }
  else if (id==alconst::uda_backend)
    {
#ifdef UDA
      UDABackend* tbe = new UDABackend(ctx->getURI());
      be = tbe;
#else
      throw ALBackendException("UDA backend is not available within current install",LOG);
#endif
    }
  else if (id==alconst::flexbuffers_backend) {
      FlexbuffersBackend *tbe = new FlexbuffersBackend();
      be = tbe;
    } 
  else
    {
      std::cerr << "Non-identified backend ID (" << id << ")\n";
      throw ALBackendException("Wrong backend identifier "+std::to_string(id),LOG);
    }

  if (be->supportsTimeDataInterpolation() || be->supportsTimeRangeOperation()) 
     be->initDataInterpolationComponent();
     
  return be;
}
