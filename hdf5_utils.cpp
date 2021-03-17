#include "hdf5_utils.h"

#include <string.h>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;


HDF5Utils::HDF5Utils()
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Utils::~HDF5Utils()
{
}

void HDF5Utils::writeHeader(PulseContext * ctx, hid_t file_id, std::string & filePath, std::string backend_version)
{

    const char *backend_version_attribute_name = "HDF5_BACKEND_VERSION";

    //write version to file
    hid_t dataspace_id = H5Screate(H5S_SCALAR);
    hid_t dtype_id = H5Tcreate(H5T_STRING, 10);
    herr_t tset = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
    if (tset < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", backend_version_attribute_name);
        throw UALBackendException(error_message);
    }
    hid_t att_id = H5Acreate2(file_id, backend_version_attribute_name, dtype_id,
                              dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (att_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create attribute: %s\n", backend_version_attribute_name);
        throw UALBackendException(error_message, LOG);
    }
    herr_t status = H5Awrite(att_id, dtype_id, backend_version.c_str());
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write attribute: %s\n", backend_version_attribute_name);
        throw UALBackendException(error_message, LOG);
    }

    const char *shot = "SHOT";
    H5Aclose(att_id);

    att_id = H5Acreate2(file_id, shot, H5T_NATIVE_INT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (att_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create attribute: %s\n", shot);
        throw UALBackendException(error_message, LOG);
    }
    int shotNumber = std::stoi(getShotNumber(ctx));
    status = H5Awrite(att_id, H5T_NATIVE_INT, &shotNumber);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write attribute: %s\n", shot);
        throw UALBackendException(error_message, LOG);
    }
    const char *run = "RUN";
    H5Aclose(att_id);
    att_id = H5Acreate2(file_id, run, H5T_NATIVE_INT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);

    if (att_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create attribute: %s\n", run);
        throw UALBackendException(error_message, LOG);
    }
    int runNumber = std::stoi(getRunNumber(ctx));
    status = H5Awrite(att_id, H5T_NATIVE_INT, &runNumber);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write attribute: %s\n", run);
        throw UALBackendException(error_message, LOG);
    }
    writeUserBlock(filePath.c_str(), ctx);
    H5Sclose(dataspace_id);
    H5Tclose(dtype_id);
    H5Aclose(att_id);
}

struct ShotRun {
    char shot_run[200];
};

void HDF5Utils::writeUserBlock(const std::string & filePath, PulseContext * ctx)
{
    std::ofstream file(filePath, std::ifstream::binary);
    file.seekp(0, std::ios::beg);
    ShotRun sr;
    strcpy(sr.shot_run, "shot=");
    strcat(sr.shot_run, std::to_string(ctx->getShot()).c_str());
    strcat(sr.shot_run, ";run=");
    strcat(sr.shot_run, std::to_string(ctx->getRun()).c_str());
    strcat(sr.shot_run, ";");
    file.write((char *) &sr, sizeof(ShotRun));
    file.close();
}

herr_t file_info(hid_t loc_id, const char *IDS_link_name, const H5L_info_t * linfo, void *opdata1)
{
    HDF5Utils hdf5_utils;

    struct opdata *od = (struct opdata *) opdata1;
    std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(od->files_directory, od->relative_file_path, std::string(IDS_link_name));
    hid_t IDS_file_id = H5Fopen(IDSpulseFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (!od->mode) {
        //herr_t status = 
        if (H5Lexists(IDS_file_id, IDS_link_name, H5P_DEFAULT) > 0)
            H5Ldelete(IDS_file_id, IDS_link_name, H5P_DEFAULT);
    }

    H5Fclose(IDS_file_id);      //closing the IDS file
    od->link_names[od->count] = (char *) malloc(100);
    strcpy(od->link_names[od->count], IDS_link_name);
    od->count++;

    return 0;
}


std::string HDF5Utils::pulseFilePathFactory(PulseContext * ctx, int strategy, std::string & files_directory, std::string & relative_file_path)
{
    switch (strategy) {
    case FULL_MDSPLUS_STRATEGY:
        return getPulseFilePath(ctx, strategy, files_directory, relative_file_path);
        break;
    case MODIFIED_MDSPLUS_STRATEGY:
        return getPulseFilePath(ctx, strategy, files_directory, relative_file_path);
        break;
    case FREE_PATH_STRATEGY:
        throw UALBackendException("Strategy for pulse files path location not yet implemented", LOG);
        break;
    default:
        throw UALBackendException("Unknow strategy for pulse files path location", LOG);
    }
}

std::string HDF5Utils::getPulseFilePath(PulseContext * ctx, int strategy, std::string & files_directory, std::string & relative_file_path)
{
    std::string filePath;
    std::string user = ctx->getUser();
    std::string tokamak = ctx->getTokamak();
    std::string version = ctx->getVersion();

    if (!strcmp(user.c_str(), "public")) {
        char *home = getenv("IMAS_HOME");
        filePath += home;
        filePath += "/shared/imasdb/";
        filePath += tokamak;
        filePath += "/";
        filePath += version;
    } else if (user.rfind("/", 0) == 0) {
        filePath += user;
        filePath += "/imasdb/";
        filePath += tokamak;
        filePath += "/";
        filePath += version;
    } else {
        std::string userHome = std::string(getenv("HOME"));
        std::string parentUserHome = userHome.substr(0, userHome.find_last_of("/\\"));
        filePath += parentUserHome;
        filePath += "/";
        filePath += user;
        filePath += "/public/imasdb/";
        filePath += tokamak;
        filePath += "/";
        filePath += version;
    }
    const int run = ctx->getRun();
    std::string runNumber = std::to_string(run);

    if (strategy == FULL_MDSPLUS_STRATEGY) {
        int r = run / 10000;
        filePath += "/" + std::to_string(r) + "/";
    } else {
        filePath += "/";
    }

    files_directory = filePath;
    files_directory += getShotNumber(ctx);
    try {
        create_directories(files_directory.c_str());
    }
    catch(std::exception & e) {
        std::string message("Unable to create pulse files shot directory: ");
        message += files_directory;
        throw UALBackendException(message, LOG);
    }

    files_directory += "/" + getRunNumber(ctx);
    try {
        create_directories(files_directory.c_str());
    }
    catch(std::exception & e) {
        std::string message("Unable to create pulse files run directory: ");
        message += files_directory;
        throw UALBackendException(message, LOG);
    }

    relative_file_path = "master.h5";
    return files_directory + "/" + relative_file_path;
}

std::string HDF5Utils::getShotNumber(PulseContext * ctx)
{
    return std::to_string(ctx->getShot());
}

std::string HDF5Utils::getRunNumber(PulseContext * ctx)
{
    return std::to_string(ctx->getRun());
}

std::string HDF5Utils::getFullShotNumber(PulseContext * ctx)
{
    return std::to_string(ctx->getShot()) + std::to_string(ctx->getRun());
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
        int i = indices.size() - 1;
        ArraystructContext *arrCtx = dynamic_cast < ArraystructContext * >(ctx);
        if (arrCtx->getTimed())
            *timedAOS_index = i;
        indices[i] = arrCtx->getIndex();
        i--;
        ArraystructContext *parent = arrCtx->getParent();
        while (parent != NULL) {
            if (parent->getTimed())
                *timedAOS_index = i;
            indices[i] = parent->getIndex();
            parent = parent->getParent();
            i--;
        }
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
            throw UALBackendException(error_message, LOG);
        }
    } else {
        loc_id = H5Gopen2(parent_loc_id, att_name.c_str(), H5P_DEFAULT);
        if (loc_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open HDF5 group: %s\n", att_name.c_str());
            throw UALBackendException(error_message, LOG);
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
            throw UALBackendException(error_message, LOG);
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
    return loc_id;
}


hid_t HDF5Utils::searchDataSetId(const std::string & tensorized_path, std::unordered_map < std::string, hid_t > &opened_data_sets)
{
    hid_t dataset_id = -1;

    if (opened_data_sets.find(tensorized_path) != opened_data_sets.end())
        dataset_id = opened_data_sets[tensorized_path];

    return dataset_id;
}

void HDF5Utils::showStatus(hid_t file_id) {
  size_t n = H5Fget_obj_count (file_id, H5F_OBJ_GROUP);
 std::cout << "number of groups opened: " << n << std::endl;
  n = H5Fget_obj_count (file_id, H5F_OBJ_DATASET);
  std::cout << "number of datasets opened: " << n << std::endl;
  n = H5Fget_obj_count (file_id, H5F_OBJ_FILE);
  std::cout << "number of files opened: " << n << std::endl;
   n = H5Fget_obj_count (file_id, H5F_OBJ_ALL);
  std::cout << "number of objects opened: " << n << std::endl;
}
