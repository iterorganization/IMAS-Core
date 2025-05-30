#ifndef HDF5_READER_H
#define HDF5_READER_H 1

#include <hdf5.h>
#include "al_backend.h"
#include "hdf5_hs_selection_reader.h"
#include "hdf5_dataset_handler.h"
#include "data_interpolation.h"

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>


class HDF5Reader {
  private:

    typedef herr_t(* H5L_iterate1_t) (hid_t group, const char *name, const H5L_info_t *info, void *op_data);
    static herr_t iterate_callback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callback_data);
    
    std::string backend_version;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > opened_data_sets;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > opened_shapes_data_sets;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > aos_opened_shapes_data_sets;
    std::unordered_map < std::string, hid_t > existing_data_sets;
    std::unordered_map < ArraystructContext *,  std::vector<std::string>> tensorized_paths_per_context;

    std::unordered_map < OperationContext *,  std::vector<std::string>> tensorized_paths_per_op_context;

    std::unordered_map < ArraystructContext *,  std::vector<int>> arrctx_shapes_per_context;
    
    std::unordered_map < OperationContext *,  hid_t> IDS_group_id;
    
    int slice_mode;

    DataInterpolation data_interpolation_component;

    void configureTimeRange(OperationContext *opctx, const std::string &timebasename, 
      HDF5HsSelectionReader &hsSelectionReader, const std::vector <int> &current_arrctx_indices, hid_t gid, 
      bool is_dynamic, bool isTimed, int timed_AOS_index, const std::vector<double> &time_basis_vector, int *size, int dim);

    void getTimeVector(OperationContext * opCtx, std::unique_ptr < HDF5DataSetHandler > &data_set, const std::string & timebasename, int timed_AOS_index, 
    const std::vector < int > &current_arrctx_indices, std::vector<double> &time_basis_vector, bool resampling = false);
    int checkSlicesShapes(Context *ctx, hid_t gid, const std::string &tensorized_path, int datatype, int slice_mode,
                                    bool is_dynamic, int slice_min, int slice_sup, int dim, int timed_AOS_index,
                                    hid_t *dataset_id_shapes, const std::vector<int> &current_arrctx_indices, hid_t dataset_id, bool isOpenedShapesDataSet, HDF5HsSelectionReader &hsSelectionReader);
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

    std::string getTimeVectorDataSetName(const std::string &timebasePath, int timed_AOS_index, std::vector < std::string > &tensorized_paths);
    std::string getTimeVectorDataSetName(OperationContext * opCtx, std::string timebasename, int timed_AOS_index);
    std::unique_ptr < HDF5DataSetHandler > getTimeVectorDataSet(OperationContext *opCtx, hid_t gid, const std::string & dataset_name, int time_vector_dim);

    int exit_request(std::unique_ptr < HDF5DataSetHandler > &data_set, int exit_status);
    DataEntryContext* getDataEntryContext(Context * ctx);

    int fixCharDataSet(int datatype, void **data, int *size, int dim);
    static bool isMatchingDefaultValue(double value);
    static bool ends_with(const std::string &str, const std::string &suffix);

    bool INTERPOLATION_WARNING;
    std::vector<double> time_basis_vector;

  public:

     HDF5Reader(std::string backend_version_);
    ~HDF5Reader();

    int homogeneous_time;

    virtual void closePulse(DataEntryContext * ctx, int mode, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual int read_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, void **data, int *dim, int *size);
    virtual void beginReadArraystructAction(ArraystructContext * ctx, int *size);
    virtual void get_occurrences(const char* ids_name, int** occurrences_list, int* size, hid_t master_file_id);

    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);
    void close_datasets();
    void close_group(OperationContext *ctx);
    void endAction(Context * ctx);
    void setSliceMode(OperationContext *ctx);
};

#endif
