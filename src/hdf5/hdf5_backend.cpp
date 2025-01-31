#include "hdf5_backend.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_backend_factory.h"

/**
 * @brief Constructor for the HDF5Backend class.
 *
 * This constructor initializes the HDF5Backend object by setting the file_id to -1,
 * initializing the pulseFilePath to an empty string, and initializing the opened_IDS_files
 * container. It also calls the createBackendComponents method with the version obtained
 * from the getVersion method.
 */
HDF5Backend::HDF5Backend()
:  file_id(-1), pulseFilePath(""), opened_IDS_files()
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    createBackendComponents(getVersion());
}

HDF5Backend::HDF5Backend(Backend * targetB)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Backend::~HDF5Backend()
{
}

const int HDF5Backend::HDF5_BACKEND_VERSION_MAJOR = 1;
const int HDF5Backend::HDF5_BACKEND_VERSION_MINOR = 0;


/**
 * @brief Creates and initializes the backend components for HDF5 operations.
 *
 * This function initializes the HDF5 backend components by creating instances
 * of the writer, reader, and events handler using the HDF5BackendFactory.
 *
 * @param backend_version The version of the backend to be used for creating
 *                        the components.
 */
void
 HDF5Backend::createBackendComponents(std::string backend_version) {
    HDF5BackendFactory backendFactory(backend_version);
    hdf5Writer = backendFactory.createWriter();
    hdf5Reader = backendFactory.createReader();
    eventsHandler = backendFactory.createEventsHandler();
}

/**
 * @brief Retrieves the version of the HDF5 backend.
 *
 * This function returns the version of the HDF5 backend as a pair of integers
 * representing the major and minor version numbers. If the provided context
 * is null, it returns the predefined version constants. Otherwise, it reads
 * the version from the master file associated with the given context.
 *
 * @param ctx Pointer to the DataEntryContext. If null, the predefined version
 *            constants are returned.
 * @return A std::pair<int, int> representing the major and minor version numbers.
 * @throws ALBackendException if unable to parse the backend version from the
 *         master file.
 */
std::pair<int,int> HDF5Backend::getVersion(DataEntryContext *ctx)
{
  std::pair<int,int> version;
  if(ctx==NULL)
    version = {HDF5_BACKEND_VERSION_MAJOR, HDF5_BACKEND_VERSION_MINOR};
  else
    {
      std::string backend_version;
      files_path_strategy = HDF5Utils::MODIFIED_MDSPLUS_STRATEGY;
      bool masterFileAlreadyOpened = (this->file_id != -1);
      //we call openPulse() which reads the backend version from the master file (no attempt for opening the master file will be performed if it is already opened) 
      HDF5Utils::openPulse(ctx, OPEN_PULSE, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
      std::string::size_type pos = backend_version.find_first_of('.');
      std::string version_major = backend_version.substr(0, pos);
      std::string version_minor = backend_version.substr(pos+1, std::string::npos);
      try {
          version = {std::stoi( version_major ),std::stoi( version_minor )};
        }
      catch (std::exception &e) {
            char error_message[200];
            sprintf(error_message, "Unable to get backend version: %s\n", e.what());
            throw ALBackendException(error_message, LOG);
      }
      
      HDF5BackendFactory backendFactory(backend_version);
      auto hdf5Reader_version = backendFactory.createReader();
      if (!masterFileAlreadyOpened) //the master pulse file is closed only if it was already closed before to call the getVersion() method
        hdf5Reader_version->closePulse(ctx, OPEN_PULSE, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    }
  return version;
}

std::string HDF5Backend::getVersion() {
    std::pair<int,int> version = getVersion(NULL);
    return std::to_string(version.first) + "." + std::to_string(version.second);
}

/**
 * @brief Opens or creates a pulse file in the HDF5 backend.
 *
 * This function handles the opening or creation of a pulse file based on the specified mode.
 * It sets the access mode, determines the backend version, and uses the HDF5Utils utility
 * functions to open or create the pulse file as needed.
 *
 * @param ctx Pointer to the DataEntryContext object.
 * @param mode Integer representing the mode of operation. Supported modes are:
 *             - OPEN_PULSE: Open an existing pulse file.
 *             - FORCE_OPEN_PULSE: Force open an existing pulse file.
 *             - CREATE_PULSE: Create a new pulse file.
 *             - FORCE_CREATE_PULSE: Force create a new pulse file.
 *
 * @throws ALBackendException if the mode is not supported.
 */
void
 HDF5Backend::openPulse(DataEntryContext * ctx, int mode)
{
    access_mode = mode;

    std::string backend_version;
    
    files_path_strategy = HDF5Utils::MODIFIED_MDSPLUS_STRATEGY;

    switch (mode) {
    case OPEN_PULSE:
    case FORCE_OPEN_PULSE: 
        {
        int status = HDF5Utils::openPulse(ctx, mode, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        if (status == -1) { //master file doesn't exist
            backend_version = getVersion();
            HDF5Utils::createPulse(ctx, mode, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        }
        break;
        }
    case CREATE_PULSE:
    case FORCE_CREATE_PULSE:
        backend_version = getVersion();
        HDF5Utils::createPulse(ctx, mode, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        break;
    default:
        throw ALBackendException("Mode not yet supported", LOG);
    }
    createBackendComponents(backend_version);
}

/**
 * @brief Closes the pulse in the HDF5 backend.
 *
 * This function closes the pulse by delegating the task to either the HDF5 reader or writer
 * based on the current access mode. It also ensures that datasets are closed properly.
 *
 * @param ctx Pointer to the DataEntryContext. Must not be nullptr.
 * @param mode The mode in which the pulse should be closed.
 *
 * @throws ALBackendException if the context is null.
 */
void HDF5Backend::closePulse(DataEntryContext * ctx, int mode)
{
    if (ctx == nullptr)
        throw ALBackendException("HDF5Backend: unexpected null context in HDF5Backend::closePulse()", LOG);
    if (access_mode == OPEN_PULSE || access_mode == FORCE_OPEN_PULSE) {
        hdf5Reader->closePulse(ctx, mode, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    } else if (access_mode == CREATE_PULSE || access_mode == FORCE_CREATE_PULSE) {
        hdf5Writer->closePulse(ctx, mode, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    }
    hdf5Writer->close_datasets();
    hdf5Reader->close_datasets();
}

/**
 * @brief Writes data to an HDF5 file.
 *
 * This function writes multi-dimensional data to an HDF5 file using the provided context, field name, 
 * time base name, data type, dimensions, and size.
 *
 * @param ctx Pointer to the context object.
 * @param fieldname The name of the field to write the data to.
 * @param timebasename The base name for the time dimension.
 * @param data Pointer to the data to be written.
 * @param datatype The type of the data (e.g., H5T_NATIVE_INT, H5T_NATIVE_FLOAT).
 * @param dim The number of dimensions of the data.
 * @param size Pointer to an array containing the size of each dimension.
 */
void HDF5Backend::writeData(Context * ctx, std::string fieldname, std::string timebasename, void *data, int datatype, int dim, int *size)
{
    hdf5Writer->write_ND_Data(ctx, fieldname, timebasename, datatype, dim, size, data);
}

/**
 * @brief Reads data from an HDF5 file.
 *
 * This function reads multi-dimensional data from an HDF5 file using the provided context, field name, and time base name.
 *
 * @param ctx Pointer to the context object.
 * @param fieldname The name of the field to read data from.
 * @param timebasename The base name of the time to read data from.
 * @param data Pointer to the location where the read data will be stored.
 * @param datatype Pointer to an integer where the data type of the read data will be stored.
 * @param dim Pointer to an integer where the dimension of the read data will be stored.
 * @param size Pointer to an integer where the size of the read data will be stored.
 * @return An integer indicating whether the data is available (1) or not (0).
 */
int HDF5Backend::readData(Context * ctx, std::string fieldname, std::string timebasename, void **data, int *datatype, int *dim, int *size)
{
    int dataAvailable = 0;      //not available by default
    dataAvailable = hdf5Reader->read_ND_Data(ctx, fieldname, timebasename, *datatype, data, dim, size);
    return dataAvailable;
}


/**
 * @brief Deletes data from the HDF5 file.
 *
 * This function deletes data from the HDF5 file specified by the given path.
 * It uses the HDF5Writer to perform the deletion operation.
 *
 * @param ctx The operation context containing information about the current operation.
 * @param path The path to the data that needs to be deleted.
 *
 * @note If the master file is closed (file_id == -1), the function will return without performing any operation.
 */
void HDF5Backend::deleteData(OperationContext * ctx, std::string path)
{
    if (file_id == -1) //master file is closed
        return;
    hdf5Writer->deleteData(ctx, this->file_id, opened_IDS_files, files_directory, relative_file_path);
}

/**
 * @brief Begins the write action for an array structure in the HDF5 backend.
 *
 * This function initiates the process of writing an array structure to the HDF5 backend.
 * If the size of the array structure is zero, the function returns immediately without
 * performing any action.
 *
 * @param ctx Pointer to the ArraystructContext which contains the context information
 *            for the array structure to be written.
 * @param size Pointer to an integer representing the size of the array structure. If the
 *             size is zero, the function will return without performing any action.
 */
void HDF5Backend::beginWriteArraystructAction(ArraystructContext * ctx, int *size)
{
    if (*size == 0)
        return;
    hdf5Writer->beginWriteArraystructAction(ctx, size);
}

/**
 * @brief Begins the action of reading an array structure from the HDF5 backend.
 *
 * This function delegates the action to the `hdf5Reader` object, which performs
 * the actual reading operation.
 *
 * @param ctx Pointer to the context of the array structure to be read.
 * @param size Pointer to an integer where the size of the array structure will be stored.
 */
void HDF5Backend::beginReadArraystructAction(ArraystructContext * ctx, int *size)
{
    hdf5Reader->beginReadArraystructAction(ctx, size);
}

/**
 * @brief Begins an action in the HDF5 backend.
 *
 * This function initiates an action by invoking the `beginAction` method of the `eventsHandler` object.
 * It passes the operation context and various HDF5-related parameters to the handler.
 *
 * @param ctx Pointer to the operation context.
 */
void HDF5Backend::beginAction(OperationContext * ctx)
{
    eventsHandler->beginAction(ctx, file_id, opened_IDS_files, *hdf5Writer, *hdf5Reader, files_directory, relative_file_path, access_mode);
}

/**
 * @brief Ends the current action in the HDF5 backend.
 *
 * This function is responsible for finalizing the current action by invoking
 * the `endAction` method of the `eventsHandler` object. It passes the context,
 * file identifier, HDF5 writer, HDF5 reader, and the list of opened IDS files
 * to the `eventsHandler` for proper handling.
 *
 * @param ctx Pointer to the current context.
 */
void HDF5Backend::endAction(Context * ctx)
{
    eventsHandler->endAction(ctx, file_id, *hdf5Writer, *hdf5Reader, opened_IDS_files);
}

/**
 * @brief Retrieves the occurrences of a given ID name from the HDF5 file.
 *
 * This function fetches the occurrences of the specified ID name from the HDF5 file
 * and stores them in the provided occurrences list. It also updates the size of the list.
 *
 * @param ids_name The name of the ID whose occurrences are to be retrieved.
 * @param occurrences_list A pointer to an array of integers where the occurrences will be stored.
 * @param size A pointer to an integer where the size of the occurrences list will be stored.
 *
 * @throws ALBackendException if the master file is not opened.
 */
void HDF5Backend::get_occurrences(const  char* ids_name, int** occurrences_list, int* size)
{
    if (file_id == -1) //master file not opened
        throw ALBackendException("HDF5Backend: master file not opened while calling HDF5Backend::get_occurrences()", LOG); 
    hdf5Reader->get_occurrences(ids_name, occurrences_list, size, file_id);
}
