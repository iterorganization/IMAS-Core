#ifndef HDF5_BACKEND_H
#define HDF5_BACKEND_H 1

#include <hdf5.h>
#include "al_backend.h"
#include "hdf5_reader.h"
#include "hdf5_writer.h"
#include "hdf5_events_handler.h"
#include <memory>
#include <vector>
#include <list>

#include "data_interpolation.h"

class HDF5Backend:public Backend {
  private:
    
    static const int HDF5_BACKEND_VERSION_MAJOR;
    static const int HDF5_BACKEND_VERSION_MINOR;

    std::string files_directory; //directory containing the pulse files
    std::string relative_file_path; 

    hid_t file_id;
     std::string pulseFilePath;
     std::unordered_map < std::string, hid_t > opened_IDS_files;

     std::unique_ptr < HDF5Writer > hdf5Writer;
     std::unique_ptr < HDF5Reader > hdf5Reader;
     std::unique_ptr < HDF5EventsHandler > eventsHandler;

    int access_mode;
    int files_path_strategy;

     void createBackendComponents(std::string backend_version);
     std::string getVersion();
     std::string getPulseFilePath(DataEntryContext * ctx);

    void beginWriteArraystructAction(ArraystructContext * ctx, int *size);
    void beginReadArraystructAction(ArraystructContext * ctx, int *size);

    DataInterpolation *data_interpolation_component;

  public:

     HDF5Backend();
     HDF5Backend(Backend * targetB);



     virtual ~ HDF5Backend();

     std::pair<int,int> getVersion(DataEntryContext *ctx) override;

        /**
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @throw BackendException
	 */
    void openPulse(DataEntryContext * ctx, int mode) override;

        /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse
     @throw BackendException
	 */
    void closePulse(DataEntryContext * ctx, int mode) override;

        /**
     Writes data.
     This function writes a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name 
     @param[in] timebasename time base field name
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (NULL is dim=0)
     @throw BackendException
	 */
    void writeData(Context * ctx, std::string fieldname, std::string timebasename, void *data, int datatype, int dim, int *size) override;

        /**
     Reads data.
     This function reads a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name
     @param[in] istimed specify the time-dependency of the field
     @param[out] data returned pointer on the read data 
     @param[out] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[out] dim returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[out] size array returned with elements filled at the size of each dimension 
     @throw BackendException
	 */
    int readData(Context * ctx, std::string fieldname, std::string timebase, void **data, int *datatype, int *dim, int *size) override;

        /**
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx pointer on operation context 
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @throw BackendException
	 */
    void deleteData(OperationContext * ctx, std::string path) override;

    void beginArraystructAction(ArraystructContext * ctx, int *size) override {
        if (ctx->getOperationContext()->getAccessmode() == READ_OP)
            beginReadArraystructAction(ctx, size);
        else
            beginWriteArraystructAction(ctx, size);
    }
    void endAction(Context * ctx) override;

    void beginAction(OperationContext * ctx) override;

    void get_occurrences(const char* ids_name, int** occurrences_list, int* size) override;

    bool performsTimeDataInterpolation() {
      return true;
    }

    void initDataInterpolationComponent() {
      this->data_interpolation_component = new DataInterpolation;
    }

};

#endif
