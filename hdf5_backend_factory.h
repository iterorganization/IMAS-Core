#ifndef HDF5_BACKEND_FACTORY_H
#define HDF5_BACKEND_FACTORY_H 1


#include <hdf5.h>
#include "hdf5_writer.h"
#include "hdf5_reader.h"
#include "hdf5_events_handler.h"

#include <memory>
#include <string>

class HDF5BackendFactory {
  private:
    std::string backend_version_;

  public:

    HDF5BackendFactory(std::string backend_version);
    ~HDF5BackendFactory();

  /**
   Creates a writer instance according to the backend version passed to the factory.
   **/
    std::unique_ptr < HDF5Writer > createWriter();

  /**
   Creates a reader instance according to the backend version passed to the factory.
   **/
    std::unique_ptr < HDF5Reader > createReader();

  /**
   Creates a events handler instance according to the backend version passed to the factory.
   **/
    std::unique_ptr < HDF5EventsHandler > createEventsHandler();

};

#endif
