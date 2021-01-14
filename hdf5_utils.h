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
    std::string getPulseFilePath(PulseContext * ctx, int strategy, std::string & files_directory, std::string & relative_file_path);

  public:

     HDF5Utils();
    ~HDF5Utils();
     std::string pulseFilePathFactory(PulseContext * ctx, int strategy, std::string & files_directory, std::string & relative_file_path);

     std::string getShotNumber(PulseContext * ctx);
     std::string getFullShotNumber(PulseContext * ctx);
     std::string getRunNumber(PulseContext * ctx);

     std::string getIDSPulseFilePath(const std::string & files_directory, const std::string & relative_file_name, const std::string & ids_name);

    void writeUserBlock(const std::string & filePath, PulseContext * ctx);
    void writeHeader(PulseContext * ctx, hid_t file_id, std::string & filePath, std::string backend_version);

    hid_t searchDataSetId(const std::string & tensorized_path, std::unordered_map < std::string, hid_t > &opened_data_sets);

    hid_t createOrOpenHDF5Group(const std::string & path, const hid_t & parent_loc_id);
    hid_t createHDF5Group(const std::string & path, const hid_t & parent_loc_id, bool * group_already_exists);
    hid_t openHDF5Group(const std::string & path, const hid_t & parent_loc_id);
    int max(int num1, int num2);
    bool isTimed(Context * ctx, int *timed_AOS_index);
    void getAOSIndices(Context * ctx, std::vector < int >&indices, int *timedAOS_index);
    int getAOSIndicesSize(Context * ctx);
    void setTensorizedPaths(ArraystructContext * ctx, std::vector < std::string > &tensorized_paths);
    void showStatus(hid_t file_id);
    enum Files_paths_strategies { FULL_MDSPLUS_STRATEGY = 1, MODIFIED_MDSPLUS_STRATEGY = 2, FREE_PATH_STRATEGY = 3
    };

};

#endif
