#ifndef HDF5_UTILS_H
#define HDF5_UTILS_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include <memory>
#include <vector>
#include <list>
#include <unordered_map>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <Shlobj.h>
#else 
#include <pwd.h>
#endif 

extern "C" herr_t file_info(hid_t loc_id, const char *name, const H5L_info_t * linfo, void *opdata);

struct opdata {
    bool mode;                  //0=writing, 1=reading
     std::string files_directory;
     std::string relative_file_path;
    int count;
    hid_t file_ids[500];
    char *link_names[500];
};

class HDF5Utils {
  private:
    std::string getPulseFilePath(PulseContext * ctx, int mode, int strategy, std::string & files_directory, std::string & relative_file_path);
    void deleteIDSFiles(std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void createMasterFile(PulseContext * ctx, std::string &filePath, hid_t *file_id, std::string &backend_version);
    void openMasterFile(hid_t *file_id, std::string &filePath);
    void initExternalLinks(hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string &files_directory, std::string &relative_file_path);

  public:

     HDF5Utils();
    ~HDF5Utils();

    static int openPulse(PulseContext * ctx, int mode, std::string & options, std::string & backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path, std::string &pulseFilePath);

    static void createPulse(PulseContext * ctx, int mode, std::string & options, std::string backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path, std::string &pulseFilePath);

     std::string pulseFilePathFactory(PulseContext * ctx, int mode, int strategy, std::string & files_directory, std::string & relative_file_path);
     std::string getShotNumber(PulseContext * ctx);
     std::string getFullShotNumber(PulseContext * ctx);
     std::string getRunNumber(PulseContext * ctx);
     std::string getIDSPulseFilePath(const std::string & files_directory, const std::string & relative_file_name, const std::string & ids_name);

    void closeMasterFile(hid_t file_id);
    void openIDSFile(OperationContext * ctx, std::string &IDSpulseFile, hid_t *IDS_file_id);
    void createIDSFile(OperationContext * ctx, std::string &IDSpulseFile, std::string &backend_version, hid_t *IDS_file_id);
    void writeUserBlock(const std::string & filePath, PulseContext * ctx);
    void writeHeader(PulseContext * ctx, hid_t file_id, std::string & filePath, std::string backend_version);

    hid_t searchDataSetId(const std::string & tensorized_path, std::unordered_map < std::string, hid_t > &opened_data_sets);

    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, hid_t *IDS_group_id);
    hid_t createOrOpenHDF5Group(const std::string & path, const hid_t & parent_loc_id);
    hid_t createHDF5Group(const std::string & path, const hid_t & parent_loc_id, bool * group_already_exists);
    hid_t openHDF5Group(const std::string & path, const hid_t & parent_loc_id);
    int max(int num1, int num2);
	int min(int num1, int num2);
    bool isTimed(Context * ctx, int *timed_AOS_index);
    void getAOSIndices(Context * ctx, std::vector < int >&indices, int *timedAOS_index);
    int getAOSIndicesSize(Context * ctx);
    void setTensorizedPaths(ArraystructContext * ctx, std::vector < std::string > &tensorized_paths);
    void showStatus(hid_t file_id);
    enum Files_paths_strategies { FULL_MDSPLUS_STRATEGY = 1, MODIFIED_MDSPLUS_STRATEGY = 2, FREE_PATH_STRATEGY = 3
    };

};

#endif
