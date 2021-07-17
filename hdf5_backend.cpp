#include "hdf5_backend.h"

#include <assert.h>
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

std::string HDF5Backend::files_directory;
std::string HDF5Backend::relative_file_path;

void
 HDF5Backend::createBackendComponents(std::string backend_version) {
    HDF5BackendFactory backendFactory(backend_version);
    hdf5Writer = backendFactory.createWriter();
    hdf5Reader = backendFactory.createReader();
    eventsHandler = backendFactory.createEventsHandler();
}

std::pair<int,int> HDF5Backend::getVersion(PulseContext *ctx)
{
  std::pair<int,int> version;
  if(ctx==NULL)
    version = {HDF5_BACKEND_VERSION_MAJOR, HDF5_BACKEND_VERSION_MINOR};
  else
    {
      std::string backend_version;
      std::string options;
      files_path_strategy = HDF5Utils::MODIFIED_MDSPLUS_STRATEGY;
      bool masterFileAlreadyOpened = (this->file_id != -1);
      //we call openPulse() which reads the backend version from the master file (no attempt for opening the master file will be performed if it is already opened) 
      HDF5Utils::openPulse(ctx, OPEN_PULSE, options, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
      std::string::size_type pos = backend_version.find_first_of('.');
      std::string version_major = backend_version.substr(0, pos);
      std::string version_minor = backend_version.substr(pos+1, std::string::npos);
      try {
          version = {std::stoi( version_major ),std::stoi( version_minor )};
        }
      catch (std::exception &e) {
            char error_message[200];
            sprintf(error_message, "Unable to get backend version: %s\n", e.what());
            throw UALBackendException(error_message, LOG);
      }
      
      HDF5BackendFactory backendFactory(backend_version);
      hdf5Reader = backendFactory.createReader();
      if (!masterFileAlreadyOpened) //the master pulse file is closed only if it was already closed before to call the getVersion() method
        hdf5Reader->closePulse(ctx, OPEN_PULSE, options, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    }
  return version;
}

std::string HDF5Backend::getVersion() {
    std::pair<int,int> version = getVersion(NULL);
    return std::to_string(version.first) + "." + std::to_string(version.second);
}

void
 HDF5Backend::openPulse(PulseContext * ctx, int mode, std::string options)
{
    access_mode = mode;
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl>= 0);
    H5Pset_alignment(fapl, 0, 16);
    std::string backend_version;
    
    files_path_strategy = HDF5Utils::MODIFIED_MDSPLUS_STRATEGY;
    switch (mode) {
    case OPEN_PULSE:
    case FORCE_OPEN_PULSE: 
        {
        int status = HDF5Utils::openPulse(ctx, mode, options, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        if (status == -1) { //master file doesn't exist
            backend_version = getVersion();
            HDF5Utils::createPulse(ctx, mode, options, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        }
        break;
        }
    case CREATE_PULSE:
    case FORCE_CREATE_PULSE:
        backend_version = getVersion();
        HDF5Utils::createPulse(ctx, mode, options, backend_version, &this->file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path, this->pulseFilePath);
        if (!options.empty() && options.find("-no_compression") != std::string::npos) {
            HDF5Writer::compression_enabled = false;
        }
        break;
    default:
        throw UALBackendException("Mode not yet supported", LOG);
    }
    assert(H5Pclose(fapl)>=0);
    createBackendComponents(backend_version);
}

void HDF5Backend::closePulse(PulseContext * ctx, int mode, std::string options)
{
    if (access_mode == OPEN_PULSE || access_mode == FORCE_OPEN_PULSE) {
        hdf5Writer->close_datasets();
        hdf5Reader->closePulse(ctx, mode, options, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    } else if (access_mode == CREATE_PULSE || access_mode == FORCE_CREATE_PULSE) {
        hdf5Reader->close_datasets();
        hdf5Writer->closePulse(ctx, mode, options, &file_id, opened_IDS_files, files_path_strategy, files_directory, relative_file_path);
    }
}

void HDF5Backend::writeData(Context * ctx, std::string fieldname, std::string timebasename, void *data, int datatype, int dim, int *size)
{
    OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
    std::string IDS_link_name = opCtx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    hdf5Writer->write_ND_Data(ctx, -1, fieldname, timebasename, datatype, dim, size, data);
}

int HDF5Backend::readData(Context * ctx, std::string fieldname, std::string timebasename, void **data, int *datatype, int *dim, int *size)
{
    int dataAvailable = 0;      //not available by default
    dataAvailable = hdf5Reader->read_ND_Data(ctx, fieldname, timebasename, *datatype, data, dim, size);
    return dataAvailable;
}


void HDF5Backend::deleteData(OperationContext * ctx, std::string path)
{
    hdf5Writer->deleteData(ctx, this->file_id, opened_IDS_files, files_directory, relative_file_path);
}

void HDF5Backend::beginWriteArraystructAction(ArraystructContext * ctx, int *size)
{
    if (*size == 0)
        return;
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    hdf5Writer->beginWriteArraystructAction(ctx, size, -1, IDS_link_name);
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
