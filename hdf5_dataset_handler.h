#ifndef HDF5_DATASETHANDLER_H
#define HDF5_DATASETHANDLER_H 1

#include <hdf5.h>
#include "ual_backend.h"

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#define UINT_DATA   61

namespace hdf5const {
    const int unsigned_integer_data = UINT_DATA;
}

class HDF5DataSetHandler {
  private:
    std::string tensorized_path;        //full tensorized path

    hid_t dataset_id;
    int dataset_rank;
    int AOSRank;
    bool immutable;

    bool slice_mode;
	int slices_extension;
    int timed_AOS_index;
    bool isTimed;
    bool use_core_driver;

    hsize_t dims[H5S_MAX_RANK];
    hsize_t maxdims[H5S_MAX_RANK];
    hsize_t largest_dims[H5S_MAX_RANK];
    hsize_t chunk_dims[H5S_MAX_RANK];

    void copy_to_disk();

  public:

     HDF5DataSetHandler();
    ~HDF5DataSetHandler();

    hid_t dtype_id;
    hid_t dataspace_id;

    hid_t IDS_group_id;
    hid_t IDS_core_file_id;

    void createOrOpenTensorizedDataSet(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index, bool compression_enabled);

	void createOrOpenTensorizedDataSet2(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index, bool compression_enabled);

    void updateAOSShapesTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes);

    void updateTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, std::vector < int >&current_arrctx_indices, std::set < hid_t > &dynamic_aos_already_extended_by_slicing, bool shape_dataset, int dynamic_AOS_slices_extension);

    void extendTensorizedDataSet(int datatype, int dim, int *size, hid_t loc_id, hid_t dataset_id, int AOSRank, int *AOSSize);

    void getAttributes(bool * isTimed, int *timed_AOS_index) const;

	int getSlicesExtension() const;

     std::string getName() const;

    int getRank() const;

    hsize_t * getDims();

	int getSize() const;

    hid_t getDataSpace();

    void setNonSliceMode();

    void setSliceMode(Context * ctx, int homogeneous_time);

    int getTimedShape(int *timed_AOS_index_);

    void setExtent();

    void setTensorizedPath(std::string p) {
        tensorized_path = p;
}};

#endif
