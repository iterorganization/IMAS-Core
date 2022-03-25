#ifndef HDF5_READER_H
#define HDF5_READER_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include "hdf5_hs_selection_reader.h"
#include "hdf5_dataset_handler.h"

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>

class HDF5Reader {
  private:
    std::string backend_version;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > opened_data_sets;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > opened_shapes_data_sets;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > aos_opened_shapes_data_sets;
    std::unordered_map < std::string, hid_t > existing_data_sets;
    std::unordered_map < ArraystructContext *,  std::vector<std::string>> tensorized_paths_per_context;

    std::unordered_map < OperationContext *,  std::vector<std::string>> tensorized_paths_per_op_context;

    std::unordered_map < ArraystructContext *,  std::vector<int>> arrctx_shapes_per_context;
    
    int homogeneous_time;
    std::unordered_map < OperationContext *,  hid_t> IDS_group_id;

    int getSliceIndex(OperationContext * opCtx, std::unique_ptr < HDF5DataSetHandler > &data_set, int *slice_sup, 
                      double *linear_interpolation_factor, int timed_AOS_index, const std::vector < int > &current_arrctx_indices, bool *ignore_linear_interpolation);
    int getPersistentShapes(Context * ctx, hid_t gid, const std::string & tensorized_path, int datatype, int slice_mode, bool is_dynamic, bool isTimed, 
			    int slice_index, int dim, int *size, int timed_AOS_index, bool * zero_shape, hid_t * dataset_id_shapes, 
			    bool isOpenedShapesDataSet, const std::vector < int > &current_arrctx_indices);
    int readPersistentShapes(Context * ctx, hid_t gid, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed, 
			     int slice_index, int timed_AOS_index, bool *zero_shape, hid_t *dataset_id_shapes, bool isOpenedShapesDataSet,
			     const std::vector < int > &current_arrctx_indices
			    );
    int readPersistentShapes_Get(Context * ctx, hid_t gid, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, 
				 bool isTimed, int timed_AOS_index, int slice_index, bool *zero_shape, hid_t *dataset_id_shapes, bool isOpenedShapesDataSet,
				 const std::vector < int > &current_arrctx_indices
				);
    int readPersistentShapes_GetSlice(Context * ctx, hid_t gid, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed, 
				      int slice_index, int timed_AOS_index, bool * zero_shape, hid_t * dataset_id_shapes, bool isOpenedShapesDataSet,
				      const std::vector < int > &current_arrctx_indices
 				    );
    int readAOSPersistentShapes(Context * ctx, hid_t gid, const std::string & tensorized_path, int timed_AOS_index, int slice_index, void **shapes, const std::vector < int > &current_arrctx_indices);

    std::string getTimeVectorDataSetName(ArraystructContext * ctx, int timed_AOS_index, std::vector < std::string > &tensorized_paths);
    std::string getTimeVectorDataSetName(OperationContext * opCtx, std::string & timebasename, int timed_AOS_index);
    std::unique_ptr < HDF5DataSetHandler > getTimeVectorDataSet(hid_t gid, const std::string & dataset_name);
    

  public:

     HDF5Reader(std::string backend_version_);
    ~HDF5Reader();

    static bool useBuffering;
    int slice_mode;

    virtual void closePulse(PulseContext * ctx, int mode, std::string & options, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual int read_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, void **data, int *dim, int *size);
    virtual void beginReadArraystructAction(ArraystructContext * ctx, int *size);

    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);
    void close_datasets();
    void close_group(OperationContext *ctx);
    void endAction(Context * ctx);
};

#endif
