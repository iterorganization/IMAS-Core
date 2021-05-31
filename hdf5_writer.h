#ifndef HDF5_WRITER_H
#define HDF5_WRITER_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include "hdf5_hs_selection_writer.h"
#include "hdf5_dataset_handler.h"

#include <memory>
#include <vector>
#include <unordered_map>

class HDF5Writer {
  private:

    std::string backend_version;
    std::vector < std::string > tensorized_paths;
    std::unordered_map < std::string, hid_t > opened_data_sets;
    std::unordered_map < hid_t, std::unique_ptr < HDF5DataSetHandler >> dataset_handlers;
    std::unordered_map < std::string, std::unique_ptr < HDF5HsSelectionWriter >> selection_writers;
    std::vector < int >current_arrctx_indices;
    std::vector < int >current_arrctx_shapes;

    int homogeneous_time; 
    hid_t IDS_group_id;
    bool init_slice_index;
    int put_slice_count;
	int dynamic_AOS_slices_extension;

    hid_t createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, std::string & timebasename, int timed_AOS_index);
    void createOrUpdateAOSShapesDataSet(ArraystructContext * ctx, hid_t loc_id, std::string & IDS_link_name, int timedAOS_shape);
    int readTimedAOSShape(hid_t loc_id);
 
  public:

     HDF5Writer(std::string backend_version_);
    ~HDF5Writer();

    static bool compression_enabled;

    int slice_mode;

    virtual void closePulse(PulseContext * ctx, int mode, std::string & options, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual void deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    virtual void write_ND_Data(Context * ctx, hid_t loc_id, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data);
    virtual void beginWriteArraystructAction(ArraystructContext * ctx, int *size, hid_t loc_id, std::string & IDS_link_name);

    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);
    void create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, int access_mode);
    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void close_datasets();
    void close_group();
    void pop_back_stacks();
    void clear_stacks();
    void start_put_slice_operation();
    void end_put_slice_operation();
};

#endif
