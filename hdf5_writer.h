#ifndef HDF5_WRITER_H
#define HDF5_WRITER_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include "hdf5_hs_selection_writer.h"
#include "hdf5_dataset_handler.h"

#include <memory>
#include <vector>
#include <set>
#include <list>
#include <unordered_map>

struct dataSetState {
    int mode;                   //SLICE or GLOBAL_OP
    int state;                  //0 = NEW, 1=UPDATED
    bool extended;              //dataset or 1 or several AOSs has/have been extended
    bool data_set_extended;     //dataset has been extended
};

class HDF5Writer {
  private:

    std::string backend_version;
    std::vector < std::string > tensorized_paths;
    std::unordered_map < std::string, hid_t > opened_data_sets;
    std::unordered_map < hid_t, std::unique_ptr < HDF5DataSetHandler >> dataset_handlers;
    std::unordered_map < std::string, std::unique_ptr < HDF5HsSelectionWriter >> selection_writers;
    int homogeneous_time;

     std::vector < int >current_arrctx_indices;
     std::vector < int >current_arrctx_shapes;
    hid_t IDS_group_id;
     std::string IDS_name;
    bool init_slice_index;
     std::set < hid_t > dynamic_aos_already_extended_by_slicing;
    int put_slice_count;


    void createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, std::string & timebasename, const struct dataSetState &ds_state, int timed_AOS_index);

    void createOrUpdateAOSShapesDataSet(Context * ctx);

    void readTimedAOSShape();

    void close_dataset(hid_t dataset_id, std::string & tensorized_path);


  public:

     HDF5Writer(std::string backend_version_);
    ~HDF5Writer();


    int slice_mode;

    static void createPulse(PulseContext * ctx, int mode, std::string & options, std::string backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual void closePulse(PulseContext * ctx, int mode, std::string & options, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path);
    virtual void deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    virtual void write_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data);
    virtual void beginWriteArraystructAction(ArraystructContext * ctx, int *size);

    void close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files);

    void create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);

    void open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path);
    void close_datasets();
    void close_group();
    void pop_back_stacks();
    void clear_stacks();
    void start_put_slice_operation();
    void end_put_slice_operation();

};

#endif
