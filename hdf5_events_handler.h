#ifndef HDF5_EVENTS_HANDLER_H
#define HDF5_EVENTS_HANDLER_H 1

#include <hdf5.h>
#include "al_backend.h"
#include "hdf5_reader.h"
#include "hdf5_writer.h"

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include <string>

class HDF5EventsHandler {
  private:



  public:

    HDF5EventsHandler();
    ~HDF5EventsHandler();

  /**
   Upon receiving a Low Level beginAction event, send operations sequences to reader and writer instances.
   **/
    virtual void beginAction(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, HDF5Writer & writer, HDF5Reader & reader, std::string & files_directory, std::string & relative_file_path, int access_mode);

  /**
   Upon receiving a Low Level endAction event, send operations sequences to reader and writer instances.
   **/
    virtual void endAction(Context * ctx, hid_t file_id, HDF5Writer & writer, HDF5Reader & reader, std::unordered_map < std::string, hid_t > &opened_IDS_files);


};

#endif
