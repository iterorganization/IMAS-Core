#include "ual_backend.h"
#include "no_backend.h"
//#include "ascii_backend.h"
#include "mdsplus_backend.h"
//#include "hdf5_backend.h"
#include "memory_backend.h"


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
  else
    {
      std::cerr << "Non-identified backend ID (" << id << ")\n";
      throw UALBackendException("Wrong backend identifier "+std::to_string(id),LOG);
    }

  return be;
}


