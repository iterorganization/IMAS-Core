#ifndef HDF5_READER_H
#define HDF5_READER_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include "hdf5_hs_selection_reader.h"

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>

class HDF5Reader {
  private:
    std::string backend_version;
    std::vector < std::string > tensorized_paths;
    std::unordered_map < std::string, hid_t > opened_data_sets;
    std::unordered_map < std::string, hid_t > opened_shapes_data_sets;
	std::unordered_map < std::string, hid_t > aos_opened_shapes_data_sets;
    std::unordered_map < hid_t, void *>datasets_data;

    std::unordered_map < hid_t, std::unique_ptr < HDF5HsSelectionReader >> shapes_selection_readers;
	std::unordered_map < std::string, std::unique_ptr < HDF5HsSelectionReader >> aos_shapes_selection_readers;

    std::unordered_map < std::string, hid_t > non_existing_data_sets;
    std::unordered_map < std::string, std::unique_ptr < HDF5HsSelectionReader >> selection_readers;

    int homogeneous_time;
    bool ignore_linear_interpolation;

    std::vector < int >current_arrctx_indices;
    std::vector < int >current_arrctx_shapes;
    hid_t IDS_group_id;

    int getSliceIndex(OperationContext * opCtx, std::string & att_name, std::string & timebasename, int *slice_sup, double *linear_interpolation_factor, int timed_AOS_index);
    int getPersistentShapes(Context * ctx, const std::string & tensorized_path, int datatype, int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int dim, int *size, int timed_AOS_index, bool * zero_shape, hid_t * dataset_id_shapes);
	int readPersistentShapes(Context * ctx, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed,int slice_index, int timed_AOS_index, bool *zero_shape, hid_t *dataset_id_shapes);
	int readPersistentShapes_Get(Context * ctx, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed, int timed_AOS_index, int slice_index, bool *zero_shape, hid_t *dataset_id_shapes);
    int readPersistentShapes_GetSlice(Context * ctx, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int timed_AOS_index, bool * zero_shape, hid_t * dataset_id_shapes);
    int readAOSPersistentShapes(Context * ctx, const std::string & tensorized_path, int timed_AOS_index, int slice_index, void **shapes);

  public:

     HDF5Reader(std::string backend_version_);
    ~HDF5Reader();
    int slice_mode;

    virtual void closePulse(PulseContext * ctx, int mode, std::string & options, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual int read_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, void **data, int *dim, int *size);
    virtual void beginReadArraystructAction(ArraystructContext * ctx, int *size);

    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);
    void close_datasets();
    void close_group();
    void pop_back_stacks();
    void clear_stacks();
};

#endif
