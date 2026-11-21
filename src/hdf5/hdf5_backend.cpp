#include "hdf5_backend.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_backend_factory.h"

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


void
 HDF5Backend::createBackendComponents(std::string backend_version) {
    HDF5BackendFactory backendFactory(backend_version);
    hdf5Writer = backendFactory.createWriter();
    hdf5Reader = backendFactory.createReader();
    eventsHandler = backendFactory.createEventsHandler();
}

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
      //When the pulse is already open, read its actual access mode and reuse it so that the
      //file is not closed and reopened (avoiding e.g. silently upgrading a read-only open to read-write).
      int version_check_mode;
      if (masterFileAlreadyOpened) {
        unsigned intent = 0;
        H5Fget_intent(this->file_id, &intent);
        if (intent & H5F_ACC_RDWR)
          version_check_mode = OPEN_RP;
        else
          version_check_mode = OPEN_R;
      } else {
        version_check_mode = OPEN_R;
      }
      HDF5Utils::openPulse(ctx, version_check_mode, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
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
    case OPEN_R:
        // read-only: file must exist
        HDF5Utils::openPulse(ctx, OPEN_R, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        break;
    case OPEN_W:
        // write, truncate: delete and recreate
        backend_version = getVersion();
        HDF5Utils::createPulse(ctx, OPEN_W, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        break;
    case OPEN_X:
        // exclusive create: fail if file already exists
        {
        HDF5Utils hdf5_utils;
        std::string fs_dir, rel_path, file_path;
        file_path = hdf5_utils.pulseFilePathFactory(ctx, OPEN_X, files_path_strategy, fs_dir, rel_path);
        if (hdf5_utils.pulseFileExists(file_path))
            throw ALBackendException("HDF5 master file already exists. Use OPEN_A or OPEN_W instead.", LOG);
        backend_version = getVersion();
        HDF5Utils::createPulse(ctx, OPEN_X, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        break;
        }
    case OPEN_A:
        // append: open if exists, create if not
        {
        int status = HDF5Utils::openPulse(ctx, FORCE_OPEN_PULSE, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        if (status == -1) {
            backend_version = getVersion();
            HDF5Utils::createPulse(ctx, FORCE_OPEN_PULSE, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        }
        break;
        }
    case OPEN_RP:
        // read-write: file must exist (no fallback to read-only)
        {
        int status = HDF5Utils::openPulse(ctx, OPEN_RP, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        if (status == -1) {
            throw ALBackendException("HDF5 master file not found: " + this->pulseFilePath, LOG);
        }
        break;
        }
    default:
        throw ALBackendException("Mode not yet supported", LOG);
    }
    createBackendComponents(backend_version);
}

void HDF5Backend::closePulse(DataEntryContext * ctx, int mode)
{
    if (ctx == nullptr)
        throw ALBackendException("HDF5Backend: unexpected null context in HDF5Backend::closePulse()", LOG);
    if (access_mode == OPEN_PULSE || access_mode == FORCE_OPEN_PULSE || access_mode == OPEN_R) {
        hdf5Reader->closePulse(ctx, mode, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    } else if (access_mode == CREATE_PULSE || access_mode == FORCE_CREATE_PULSE
            || access_mode == OPEN_W || access_mode == OPEN_X
            || access_mode == OPEN_A || access_mode == OPEN_RP) {
        hdf5Writer->closePulse(ctx, mode, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    }
    hdf5Writer->close_datasets();
    hdf5Reader->close_datasets();
}

void HDF5Backend::writeData(Context * ctx, std::string fieldname, std::string timebasename, void *data, int datatype, int dim, int *size)
{
    hdf5Writer->write_ND_Data(ctx, fieldname, timebasename, datatype, dim, size, data);
}

int HDF5Backend::readData(Context * ctx, std::string fieldname, std::string timebasename, void **data, int *datatype, int *dim, int *size)
{
    int dataAvailable = 0;      //not available by default
    dataAvailable = hdf5Reader->read_ND_Data(ctx, fieldname, timebasename, *datatype, data, dim, size);
    return dataAvailable;
}


void HDF5Backend::deleteData(OperationContext * ctx, std::string path)
{
    if (file_id == -1) //master file is closed
        return;
    hdf5Writer->deleteData(ctx, this->file_id, opened_IDS_files, files_directory, relative_file_path);
}

void HDF5Backend::beginWriteArraystructAction(ArraystructContext * ctx, int *size)
{
    if (*size == 0)
        return;
    hdf5Writer->beginWriteArraystructAction(ctx, size);
}

void HDF5Backend::beginReadArraystructAction(ArraystructContext * ctx, int *size)
{
    hdf5Reader->beginReadArraystructAction(ctx, size);
}

void HDF5Backend::beginAction(OperationContext * ctx)
{
    eventsHandler->beginAction(ctx, file_id, opened_IDS_files, *hdf5Writer, *hdf5Reader, files_directory, relative_file_path, access_mode);
}

void HDF5Backend::endAction(Context * ctx)
{
    eventsHandler->endAction(ctx, file_id, *hdf5Writer, *hdf5Reader, opened_IDS_files);
}

void HDF5Backend::get_occurrences(Context* ctx, const  char* ids_name, int** occurrences_list, int* size)
{
    if (file_id == -1) //master file not opened
        throw ALBackendException("HDF5Backend: master file not opened while calling HDF5Backend::get_occurrences()", LOG); 
    hdf5Reader->get_occurrences(ids_name, occurrences_list, size, file_id);
}

void HDF5Backend::list_filled_paths(Context* ctx, const char* dataobjectname, char*** path_list, int* size)
{
    if (file_id == -1) // master file not opened
        throw ALBackendException("HDF5Backend: master file not opened while calling HDF5Backend::list_filled_paths()", LOG); 

    hdf5Reader->list_filled_paths(dataobjectname, path_list, size, file_id, opened_IDS_files, files_directory, relative_file_path, access_mode);
}
