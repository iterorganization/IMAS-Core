#ifndef HDF5_WRITER_H
#define HDF5_WRITER_H 1

#include <hdf5.h>
#include "al_backend.h"
#include "hdf5_dataset_handler.h"

#include <memory>
#include <vector>
#include <unordered_map>

class HDF5Writer {
  private:

    std::string backend_version;
    std::unordered_map < std::string, std::unique_ptr < HDF5DataSetHandler > > opened_data_sets;
    std::unordered_map < std::string, hid_t > existing_data_sets;
    std::unordered_map < ArraystructContext *, std::vector < std::string >> tensorized_paths_per_context;
    std::unordered_map < ArraystructContext *,  std::vector<int>> arrctx_shapes_per_context;
    std::unordered_map < ArraystructContext *, int> dynamic_AOS_slices_extension;
    
    int homogeneous_time;
    std::unordered_map < OperationContext *,  hid_t> IDS_group_id;
    
    int slice_mode;
    
    hid_t createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, 
				      std::string & timebasename, int timed_AOS_index, const std::vector < int > &arrctx_indices, const std::vector < int > &arrctx_shapes);
    void createOrUpdateAOSShapesDataSet(ArraystructContext * ctx, hid_t loc_id, int timedAOS_shape, const std::vector < int > &arrctx_indices, const std::vector < int > &arrctx_shapes);
    int readTimedAOSShape(Context * ctx, hid_t loc_id, const std::vector < int > &current_arrctx_indices);
    int readTimedAOSShape(hid_t loc_id, std::string &tensorized_path, const std::vector < int > &current_arrctx_indices, uri::Uri uri);
    int getDynamic_AOS_slices_extension(Context *ctx);
    int getDynamic_slices_extension(Context *ctx, int timed_AOS_index, int time_vector_length);
    ArraystructContext* getDynamicAOS(Context * ctx);
 
  public:

     HDF5Writer(std::string backend_version_);
    ~HDF5Writer();

    static bool compression_enabled;
    static bool useBuffering;
    static size_t read_chunk_cache_size;
    static size_t write_chunk_cache_size;

    virtual void closePulse(DataEntryContext * ctx, int mode, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual void deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    virtual void write_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data);
    virtual void beginWriteArraystructAction(ArraystructContext * ctx, int *size);

	void setSliceMode(int slice_mode);
    void write_buffers();
    void read_homogeneous_time(int* homogenenous_time, hid_t gid);
    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);
    void create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, int access_mode);
    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, hid_t *loc_id);
    void close_datasets();
    void close_group(OperationContext *ctx);
    void endAction(Context * ctx);
};

#endif
