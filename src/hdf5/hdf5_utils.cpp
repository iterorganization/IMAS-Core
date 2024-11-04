#include "hdf5_utils.h"

#include <string.h>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

#define MASTER_FILE_NAME "master.h5" 


HDF5Utils::HDF5Utils()
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Utils::~HDF5Utils()
{
}

bool HDF5Utils::debug = false;

int
 HDF5Utils::openPulse(DataEntryContext * ctx, int mode, std::string & backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path, std::string &pulseFilePath)
{
    HDF5Utils hdf5_utils;
    pulseFilePath = hdf5_utils.pulseFilePathFactory(ctx, mode, files_paths_strategy, files_directory, relative_file_path);
    //std::cout << "Opening HDF5 file at path: " << filePath << std::endl;
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_alignment(fapl, 0, 16);
    H5Pclose(fapl);

    /* Save old error handler */
    //H5E_auto2_t old_func;
    //void *old_client_data;
    //hid_t current_stack_id = H5Eget_current_stack();
    //H5Eget_auto(current_stack_id, &old_func, &old_client_data); 

    /* Turn off error handling */
    if (!debug)
        H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    if (! (mode == OPEN_PULSE || mode == FORCE_OPEN_PULSE))
        throw ALBackendException("HDF5Backend: unexepcted mode in HDF5Utils::openPulse()", LOG);

    if (*file_id != -1)
        hdf5_utils.closeMasterFile(file_id);

    switch (mode) {
        case OPEN_PULSE:
            hdf5_utils.openMasterFile(file_id, pulseFilePath);
            break;
        case FORCE_OPEN_PULSE:
            if (!exists(pulseFilePath.c_str())) {
                return -1;
            }
            else {
                hdf5_utils.openMasterFile(file_id, pulseFilePath);
            }
            break;
    }

    const char *backend_version_attribute_name = "HDF5_BACKEND_VERSION";

    //read version from file
    if (H5Aexists(*file_id, backend_version_attribute_name) > 0) {
        hid_t att_id = H5Aopen(*file_id, backend_version_attribute_name, H5P_DEFAULT);
        if (att_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open attribute: %s\n", backend_version_attribute_name);
            throw ALBackendException(error_message, LOG);
        }

        hid_t dtype_id = H5Tcopy (H5T_C_S1);
        H5Tset_size(dtype_id, strlen(backend_version_attribute_name));
        
        herr_t tset = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
        if (tset < 0) {
            char error_message[100];
            sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", backend_version_attribute_name);
            throw ALBackendException(error_message);
        }

        char version[10];
        herr_t status = H5Aread(att_id, dtype_id, version);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read attribute: %s\n", backend_version_attribute_name);
            throw ALBackendException(error_message, LOG);
        }
        backend_version = std::string(version);
        H5Tclose(dtype_id);
        H5Aclose(att_id);
    } else {
        char error_message[200];
        sprintf(error_message, "Not a IMAS HDF5 pulse file. Unable to find attribute: %s\n", backend_version_attribute_name);
        throw ALBackendException(error_message, LOG);
    }

    /* Restore previous error handler */
    //H5Eset_auto(current_stack_id, old_func, old_client_data); //TODO
    return 0;
}

void
 HDF5Utils::createPulse(DataEntryContext * ctx, int mode, std::string backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path, std::string &pulseFilePath)
{
    HDF5Utils hdf5_utils;
    pulseFilePath = hdf5_utils.pulseFilePathFactory(ctx, mode, files_paths_strategy, files_directory, relative_file_path);

    /* Save old error handler */
    //H5E_auto2_t old_func;
    //void *old_client_data;
    //hid_t current_stack_id = H5Eget_current_stack();
    //H5Eget_auto(current_stack_id, &old_func, &old_client_data);

    /* Turn off error handling */
    if (!debug)
        H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    
    if (*file_id != -1)
        hdf5_utils.closeMasterFile(file_id);

    //Opening master file
    switch (mode) {
        case FORCE_OPEN_PULSE:
        case CREATE_PULSE:
            if (!exists(pulseFilePath.c_str())) {
                hdf5_utils.createMasterFile(ctx, pulseFilePath, file_id, backend_version);
            }
            else {
                char error_message[200];
                sprintf(error_message, "HDF5 master file: %s already exists. Use FORCE_CREATE_PULSE instead.\n", pulseFilePath.c_str());
                throw ALBackendException(error_message, LOG);
            }
            break;
        case FORCE_CREATE_PULSE:
            hdf5_utils.deleteMasterFile(pulseFilePath, file_id, opened_IDS_files, files_directory, relative_file_path);
            hdf5_utils.createMasterFile(ctx, pulseFilePath, file_id, backend_version);
            break;
        default:
            throw ALBackendException("Mode not yet supported", LOG);
    }

    /* Restore previous error handler */
    //H5Eset_auto(current_stack_id, old_func, old_client_data); //TODO

}

bool HDF5Utils::pulseFileExists(const std::string &IDS_pulse_file) {
	return exists(IDS_pulse_file.c_str());
}

void HDF5Utils::deleteIDSFiles(std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path) {
    auto it = opened_IDS_files.begin();
    while (it != opened_IDS_files.end()) {
        const std::string & external_link_name = it->first;
        std::string IDSpulseFile = getIDSPulseFilePath(files_directory, relative_file_path, external_link_name);
        deleteIDSFile(IDSpulseFile);
        it++;
    }
}

void HDF5Utils::deleteIDSFile(const std::string &filePath) {
    if (exists(filePath.c_str())) {
        remove(filePath.c_str());
        if (exists(filePath.c_str())) {
            char error_message[200];
            sprintf(error_message, "Unable to remove HDF5 pulse file: %s\n", filePath.c_str());
            throw ALBackendException(error_message, LOG);
        }
    }
}

void HDF5Utils::deleteMasterFile(const std::string &filePath, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string &files_directory, std::string &relative_file_path) {

    if (exists(filePath.c_str())) {
        openMasterFile(file_id, filePath);
        initExternalLinks(file_id, opened_IDS_files, files_directory, relative_file_path);
        deleteIDSFiles(opened_IDS_files, files_directory, relative_file_path);
        closeMasterFile(file_id);
        remove(filePath.c_str());
        if (exists(filePath.c_str())) {
            char error_message[200];
            sprintf(error_message, "Unable to remove HDF5 master file: %s\n", filePath.c_str());
            throw ALBackendException(error_message, LOG);
        }
     }
}


void HDF5Utils::createMasterFile(DataEntryContext * ctx, std::string &filePath, hid_t *file_id, std::string &backend_version) {
    hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
    herr_t status = H5Pset_userblock(create_plist, 1024);
    if (status < 0) {
        throw ALBackendException("createPulse:unable to set a user block.", LOG);
    }

    //Creating master file
    *file_id = H5Fcreate(filePath.c_str(), H5F_ACC_TRUNC, create_plist, H5P_DEFAULT);

    H5Pclose(create_plist);
    //H5Pclose(fap_plist);

    //write backend version in the file
    if (*file_id < 0) {
        std::string message("Unable to create HDF5 file: ");
        message += filePath;
        throw ALBackendException(message, LOG);
    }
    HDF5Utils hdf5_utils;
    hdf5_utils.writeHeader(ctx, *file_id, filePath, backend_version);

}

void HDF5Utils::createIDSFile(OperationContext * ctx, std::string &IDSpulseFile, std::string &backend_version, hid_t *IDS_file_id) {

    hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
    herr_t status = H5Pset_userblock(create_plist, 1024);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to set a user block on pulse file for IDS: %s.\n", ctx->getDataobjectName().c_str());
        throw ALBackendException(error_message, LOG);
    }
    *IDS_file_id = H5Fcreate(IDSpulseFile.c_str(), H5F_ACC_TRUNC, create_plist, H5P_DEFAULT);
    if (*IDS_file_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create external file for IDS: %s.\n", ctx->getDataobjectName().c_str());
        throw ALBackendException(error_message, LOG);
    }
    H5Pclose(create_plist);

    writeHeader(ctx->getDataEntryContext(), *IDS_file_id, IDSpulseFile, backend_version);

}

void HDF5Utils::openIDSFile(OperationContext * ctx, std::string &IDSpulseFile, hid_t *IDS_file_id, bool try_read_only) {
    if (!exists(IDSpulseFile.c_str()))
	    return;
    *IDS_file_id = H5Fopen(IDSpulseFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (*IDS_file_id < 0) {
        if(try_read_only) {
            *IDS_file_id = H5Fopen(IDSpulseFile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (*IDS_file_id < 0) { 
                char error_message[200];
                sprintf(error_message, "Unable to open external file in Read-Only mode for IDS: %s. It might indicate that the file is being currently handled by a writing concurrent process.\n", ctx->getDataobjectName().c_str());
                throw ALBackendException(error_message, LOG);
            }
            else {
				//printf("IDS read successfully with file_id=%d\n", *IDS_file_id);
			}
        }
        else {
		    char error_message[200];
		    sprintf(error_message, "Unable to open external file in Read-Write mode for IDS: %s. It might indicate that the file is being currently handled by a writing concurrent process.\n", ctx->getDataobjectName().c_str());
		    throw ALBackendException(error_message, LOG);
	        
        }
    }
}

void HDF5Utils::openMasterFile(hid_t *file_id, const std::string &filePath) { //open master file
    if (*file_id != -1)
      return;
    if (!exists(filePath)) {
        std::string message("HDF5 master file not found: ");
        message += filePath;
        throw ALBackendException(message, LOG);
    }

    *file_id = H5Fopen(filePath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT); 

    if (*file_id < 0) { //have a try now in read only access
        *file_id = H5Fopen(filePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (*file_id < 0) {
            std::string message("Unable to open HDF5 master file: ");
            message += filePath;
            throw ALBackendException(message, LOG);
        }
		else {
			//printf("master file read successfully with file_id=%d\n", *file_id);
		}
    }
    
}

void HDF5Utils::closeMasterFile(hid_t *file_id) {
    if (*file_id == -1)
      return;
    herr_t status = H5Fclose(*file_id);
    if (status < 0) {
        char error_message[100];
        sprintf(error_message, "Unable to close HDF5 master file with handler: %d\n", (int) *file_id);
        throw ALBackendException(error_message, LOG);
    }
    *file_id = -1;
}

void HDF5Utils::initExternalLinks(hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string &files_directory, std::string &relative_file_path) {
    if (! (*file_id > 0))
        throw ALBackendException("HDF5Backend: unexpected file id in HDF5Utils::initExternalLinks()", LOG);
    struct opdata od;
    od.mode = false;
    od.files_directory = files_directory;
    od.relative_file_path = relative_file_path;
    od.count = 0;
    H5Literate(*file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, file_info, (void *) &od);

    for (int i = 0; i < od.count; i++) {
        std::string ids_name = std::string(od.link_names[i]);
        free(od.link_names[i]);
        std::replace(ids_name.begin(), ids_name.end(), '/', '_');
        opened_IDS_files[ids_name] = -1;
    }
}

void HDF5Utils::writeHeader(DataEntryContext * ctx, hid_t file_id, std::string & filePath, std::string backend_version)
{

    const char *backend_version_attribute_name = "HDF5_BACKEND_VERSION";

    //write version to file
    hid_t dataspace_id = H5Screate(H5S_SCALAR);
    hid_t dtype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size(dtype_id, strlen(backend_version_attribute_name));
    herr_t tset = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
    if (tset < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", backend_version_attribute_name);
        throw ALBackendException(error_message);
    }
    hid_t att_id = H5Acreate2(file_id, backend_version_attribute_name, dtype_id,
                              dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (att_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create attribute: %s\n", backend_version_attribute_name);
        throw ALBackendException(error_message, LOG);
    }
    herr_t status = H5Awrite(att_id, dtype_id, backend_version.c_str());
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write attribute: %s\n", backend_version_attribute_name);
        throw ALBackendException(error_message, LOG);
    }

    writeUserBlock(filePath.c_str(), ctx);
    H5Sclose(dataspace_id);
    H5Tclose(dtype_id);
    H5Aclose(att_id);
}

// WHAT SHALL WE STORE IN USERBLOCK W.R.T URI WHERE SHOT AND RUN ARE NOT MANDATORY ?
void HDF5Utils::writeUserBlock(const std::string & filePath, DataEntryContext * ctx)
{
    std::ofstream file(filePath, std::ifstream::binary);
    if (!file.is_open()) {
        char error_message[200];
        sprintf(error_message, "Unable to open the file %s for writing the user block.\n", filePath.c_str());
        throw ALBackendException(error_message, LOG);       
    }
    file.seekp(0, std::ios::beg);
    std::string path = ctx->getURI().query.get("path").value();
    file.write((char *) path.c_str(), path.length());
    file.close();
}

herr_t file_info(hid_t loc_id, const char *IDS_link_name, const H5L_info_t * linfo, void *opdata1)
{
    HDF5Utils hdf5_utils;

    struct opdata *od = (struct opdata *) opdata1;
    std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(od->files_directory, od->relative_file_path, std::string(IDS_link_name));
    if (exists(IDSpulseFile.c_str())) {
        hid_t IDS_file_id = H5Fopen(IDSpulseFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        if (IDS_file_id < 0) {
            std::string message("Unable to open external file: ");
            message += IDSpulseFile;
            throw ALBackendException(message, LOG);
       
            /*if (!od->mode) {
                if (H5Lexists(IDS_file_id, IDS_link_name, H5P_DEFAULT) > 0)
                    H5Ldelete(IDS_file_id, IDS_link_name, H5P_DEFAULT);
            }*/
        }
        hdf5_utils.closeIDSFile(IDS_file_id, IDS_link_name); //closing the IDS file
    }
    od->link_names[od->count] = (char *) malloc(100);
    strcpy(od->link_names[od->count], IDS_link_name);
    od->count++;

    return 0;
}


std::string HDF5Utils::pulseFilePathFactory(DataEntryContext * ctx, int mode, int strategy, std::string & files_directory, std::string & relative_file_path)
{
    switch (strategy) {
    case FULL_MDSPLUS_STRATEGY:
        return getPulseFilePath(ctx, mode, strategy, files_directory, relative_file_path);
        break;
    case MODIFIED_MDSPLUS_STRATEGY:
        return getPulseFilePath(ctx, mode, strategy, files_directory, relative_file_path);
        break;
    case FREE_PATH_STRATEGY:
        throw ALBackendException("Strategy for pulse files path location not yet implemented", LOG);
        break;
    default:
        throw ALBackendException("Unknow strategy for pulse files path location", LOG);
    }
}

std::string HDF5Utils::getPulseFilePath(DataEntryContext * ctx, int mode, int strategy, std::string & files_directory, std::string & relative_file_path)
{
    std::string path = ctx->getURI().query.get("path").value();
    files_directory = path;
    relative_file_path = MASTER_FILE_NAME;

    try {
        if (mode == CREATE_PULSE || mode == FORCE_CREATE_PULSE || mode == FORCE_OPEN_PULSE) {
         if (!exists(files_directory.c_str()))
            create_directories(files_directory.c_str());
        }
    }
    catch(std::exception & e) {
        std::string message("Unable to create data-entry directory: ");
        message += files_directory;
        throw ALBackendException(message, LOG);
    }
    return files_directory + "/" + relative_file_path;
}


std::string HDF5Utils::getIDSPulseFilePath(const std::string & files_directory, const std::string & relative_file_name, const std::string & ids_name)
{
    return files_directory + "/" + ids_name + ".h5";
}

/**
* Find maximum between two numbers.
*/
int HDF5Utils::max(int num1, int num2)
{
    return (num1 > num2) ? num1 : num2;
}


/**
* Find maximum between two numbers.
*/
int HDF5Utils::min(int num1, int num2)
{
    return (num1 < num2) ? num1 : num2;
}

bool HDF5Utils::isTimed(Context * ctx, int *timed_AOS_index)
{
    bool isTimed = false;
    *timed_AOS_index = -1;
    int index = 0;
    int _timed_AOS_index = -1;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        index++;
        ArraystructContext *arrCtx = dynamic_cast < ArraystructContext * >(ctx);
        if (arrCtx->getTimed()) {
            isTimed = arrCtx->getTimed();
            _timed_AOS_index = index - 1;
        }
        ArraystructContext *parent = arrCtx->getParent();
        while (parent != NULL) {
            index++;
            if (parent->getTimed()) {
                isTimed = parent->getTimed();
                _timed_AOS_index = index - 1;
            }
            parent = parent->getParent();
        }

    }
    if (_timed_AOS_index != -1) {
        *timed_AOS_index = index - _timed_AOS_index - 1;
    }
    return isTimed;
}

void HDF5Utils::getAOSIndices(Context * ctx, std::vector < int >&indices, int *timedAOS_index)
{
    *timedAOS_index = -1;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        int i = 0;
        ArraystructContext *arrCtx = dynamic_cast < ArraystructContext * >(ctx);
        if (arrCtx->getTimed()) {
            *timedAOS_index = i;
        }
	    indices.push_back(arrCtx->getIndex());
        ArraystructContext *parent = arrCtx->getParent();
        while (parent != NULL) {
	        i++;
            if (parent->getTimed()) {
                *timedAOS_index = i;
            }
	        indices.push_back(parent->getIndex());
            parent = parent->getParent();
        }
        std::reverse(indices.begin(), indices.end());
        if (*timedAOS_index != -1)
	        *timedAOS_index =  indices.size() - *timedAOS_index - 1;
    }
}

int HDF5Utils::getAOSIndicesSize(Context * ctx)
{

    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        int i = 1;
        ArraystructContext *arrCtx = dynamic_cast < ArraystructContext * >(ctx);
        ArraystructContext *parent = arrCtx->getParent();
        while (parent != NULL && parent->getType() == CTX_ARRAYSTRUCT_TYPE) {
            i++;
            parent = parent->getParent();
        }
        return i;
    }
    return 0;
}

void HDF5Utils::setTensorizedPaths(ArraystructContext * ctx, std::vector < std::string > &tensorized_paths)
{
    if (tensorized_paths.size() == 0) {
        tensorized_paths.push_back(ctx->getPath() + "[]");
    } else {
        tensorized_paths.push_back(tensorized_paths.back() + "/" + ctx->getPath() + "[]");
    }
    std::replace(tensorized_paths.back().begin(), tensorized_paths.back().end(), '/', '&');
}


hid_t HDF5Utils::createOrOpenHDF5Group(const std::string & path, const hid_t & parent_loc_id)
{
    hid_t loc_id = -1;
    std::string att_name = path;
    std::replace(att_name.begin(), att_name.end(), '/', '_');
    if (H5Lexists(parent_loc_id, att_name.c_str(), H5P_DEFAULT) == 0) {
        loc_id = H5Gcreate(parent_loc_id, att_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (loc_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create HDF5 group: %s\n", att_name.c_str());
            throw ALBackendException(error_message, LOG);
        }
    } else {
        loc_id = H5Gopen2(parent_loc_id, att_name.c_str(), H5P_DEFAULT);
        if (loc_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open HDF5 group: %s\n", att_name.c_str());
            throw ALBackendException(error_message, LOG);
        }
    }
    return loc_id;
}

hid_t HDF5Utils::createHDF5Group(const std::string & path, const hid_t & parent_loc_id, bool * group_already_exists)
{
    hid_t loc_id = -1;
    std::string att_name = path;
    std::replace(att_name.begin(), att_name.end(), '/', '_');
    if (H5Lexists(parent_loc_id, att_name.c_str(), H5P_DEFAULT) == 0) {
        *group_already_exists = false;
        loc_id = H5Gcreate(parent_loc_id, att_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (loc_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create HDF5 group: %s\n", att_name.c_str());
            throw ALBackendException(error_message, LOG);
        }
    } else {
        *group_already_exists = true;
    }
    return loc_id;
}

hid_t HDF5Utils::openHDF5Group(const std::string & path, const hid_t & parent_loc_id)
{
    std::string att_name = path;
    std::replace(att_name.begin(), att_name.end(), '/', '_');
    if (H5Lexists(parent_loc_id, att_name.c_str(), H5P_DEFAULT) == 0) {
        return -1;
    }
    hid_t loc_id = H5Gopen2(parent_loc_id, att_name.c_str(), H5P_DEFAULT);
    if (loc_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to open HDF5 group: %s\n", att_name.c_str());
        throw ALBackendException(error_message, LOG);
    }
    return loc_id;
}

void HDF5Utils::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, 
hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, hid_t *IDS_group_id)
{
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    hid_t IDS_file_id = -1;
    if (opened_IDS_files.find(IDS_link_name) != opened_IDS_files.end()) {
        IDS_file_id = opened_IDS_files[IDS_link_name];
    }

    if (IDS_file_id == -1) {
        std::string IDS_pulse_file = getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
        openIDSFile(ctx, IDS_pulse_file, &IDS_file_id, true);
        opened_IDS_files[IDS_link_name] = IDS_file_id;
    }

    *IDS_group_id = openHDF5Group(ctx->getDataobjectName().c_str(), file_id);
}

void HDF5Utils::showStatus(hid_t file_id) {
  size_t n = H5Fget_obj_count (file_id, H5F_OBJ_GROUP);
  std::cout << "number of groups opened: " << n << std::endl;
  if (n >= 0)
    n = H5Fget_obj_count (file_id, H5F_OBJ_DATASET);
  if (n >= 0)
    std::cout << "number of datasets opened: " << n << std::endl;
  n = H5Fget_obj_count (file_id, H5F_OBJ_FILE);
  /*hid_t dataset_ids[10];
  H5Fget_obj_ids(file_id, H5F_OBJ_DATASET, 10, dataset_ids);
  for (int i = 0; i < 10; i++)
    printf("opened datasetid=%d\n", dataset_ids[i]);*/
  if (n >= 0)
    std::cout << "number of files opened: " << n << std::endl;
  n = H5Fget_obj_count (file_id, H5F_OBJ_ALL);
  if (n >= 0)
    std::cout << "number of objects opened: " << n << std::endl;
}

int HDF5Utils::compareShapes(int *first_slice_shape, int *second_slice_shape, int dim) {
    for (int i = 0; i < dim; i++) {
        if (first_slice_shape[i] != second_slice_shape[i]) {
            return 1; //shapes are different
        }
    }
    return 0; //shapes are the same
}

int HDF5Utils::indices_to_flat_index(const std::vector<int>& indices, const hsize_t* shapes) {
    int flat_index = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        flat_index = flat_index * shapes[i] + indices[i];
    }
    return flat_index;
}


void HDF5Utils::closeIDSFile(hid_t pulse_file_id, const std::string &external_link_name) {
    if (pulse_file_id != -1) {
        herr_t status = H5Fclose(pulse_file_id);
        if (status < 0) {
            char error_message[100];
            sprintf(error_message, "Unable to close HDF5 file for IDS: %s\n", external_link_name.c_str());
            throw ALBackendException(error_message, LOG);
        }
    }
}

void HDF5Utils::removeLinkFromIDSPulseFile(hid_t &IDS_file_id, const std::string &IDS_link_name) {
    if (H5Lexists(IDS_file_id, IDS_link_name.c_str(), H5P_DEFAULT) > 0) {
        if (H5Ldelete(IDS_file_id, IDS_link_name.c_str(), H5P_DEFAULT) < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to remove HDF5 link %s from IDS file.\n", IDS_link_name.c_str());
            throw ALBackendException(error_message, LOG);
        }
    }
}

void HDF5Utils::removeLinkFromMasterPulseFile(hid_t &file_id, const std::string &link_name) {
    if (H5Ldelete(file_id, link_name.c_str(), H5P_DEFAULT) < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to remove HDF5 link %s from master file.\n", link_name.c_str());
        throw ALBackendException(error_message, LOG);
    }
}

void HDF5Utils::setDefaultOptions(size_t *read_cache, size_t *write_cache, bool *readBuffering, bool *writeBuffering) {
	char* read_cache_value_str = getenv("HDF5_BACKEND_READ_CACHE");
    if (read_cache_value_str != NULL)
	   *read_cache = (size_t) (atof(read_cache_value_str) * 1024 * 1024);
	char* write_cache_value_str = getenv("HDF5_BACKEND_WRITE_CACHE");
    if (write_cache_value_str != NULL)
	   *write_cache = (size_t) (atof(write_cache_value_str) * 1024 * 1024);
	*readBuffering = true;
	*writeBuffering = true;
}

void HDF5Utils::readOptions(uri::Uri uri, bool *compression_enabled, bool *readBuffering, size_t *read_cache, bool *writeBuffering, size_t *write_cache, bool *debug) {

    uri::OptionalValue compression = uri.query.get("hdf5_compression");
    uri::OptionalValue read_buffering = uri.query.get("hdf5_read_buffering");
    uri::OptionalValue write_buffering = uri.query.get("hdf5_write_buffering");
    uri::OptionalValue debug_option = uri.query.get("hdf5_debug");
    uri::OptionalValue read_cache_option = uri.query.get("hdf5_read_cache");
    uri::OptionalValue write_cache_option = uri.query.get("hdf5_write_cache");

    *compression_enabled = true;
    if (compression) {
      std::string value = compression.value();
      if (value == "no" || value == "n")
         *compression_enabled = false;
    }

    *readBuffering = true;
    if (read_buffering) {
      std::string value = read_buffering.value();
      if (value == "no" || value == "n")
         *readBuffering = false;
    }

    *writeBuffering = true;
    if (write_buffering) {
      std::string value = write_buffering.value();
      if (value == "no" || value == "n")
         *writeBuffering = false;
    }

    *debug = false;
    if (debug_option) {
      std::string value = debug_option.value();
      if (value == "yes" || value == "y")
         *debug = true;
    }

    if (read_cache_option) {
      std::string value = read_cache_option.value();
      *read_cache = (size_t) (atof(value.c_str()) * 1024 * 1024);
    }

    if (write_cache_option) {
      std::string value = write_cache_option.value();
      *write_cache = (size_t) (atof(value.c_str()) * 1024 * 1024);
    }
}


