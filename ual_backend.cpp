#include "ual_backend.h"
#include "no_backend.h"
#include "mdsplus_backend.h"
#include "memory_backend.h"
//#include "ascii_backend.h"
//#include "hdf5_backend.h"
#ifdef UDA
#include "uda_backend.h"
#endif

Backend* Backend::initBackend(int id)
{
  Backend *be = NULL;

  if (id==ualconst::no_backend)
    {
      NoBackend* tbe = new NoBackend();
      be = tbe;
    }
  /*
  else if (id==ualconst::ascii_backend)
    {
      AsciiBackend* tbe = new AsciiBackend(true);
      be = tbe;
    }
  */
  else if (id==ualconst::mdsplus_backend)
    {
      MDSplusBackend* tbe = new MDSplusBackend();
      be = tbe;
    }
  /*
  else if (id==ualconst::hdf5_backend)
    {
    HDF5Backend* tbe = new HDF5Backend();
    be = tbe;
    }
  */
  else if (id==ualconst::memory_backend)
    {
      MemoryBackend* tbe = new MemoryBackend();
      be = tbe;
    }
  else if (id==ualconst::uda_backend)
    {
#ifdef UDA
      UDABackend* tbe = new UDABackend(true);
      be = tbe;
#else
      throw UALBackendException("UDA backend is not available within current install",LOG);
#endif
    }
  else
    {
      std::cerr << "Non-identified backend ID (" << id << ")\n";
      throw UALBackendException("Wrong backend identifier "+std::to_string(id),LOG);
    }

  return be;
}


