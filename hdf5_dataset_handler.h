#ifndef HDF5_DATASETHANDLER_H
#define HDF5_DATASETHANDLER_H 1

#include <hdf5.h>
#include "ual_backend.h"

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#define UINT_DATA   61

class HDF5DataSetHandler {
  private:
    std::string dataset_name;   //full tensorized path
    hid_t dataset_id;
    int dataset_rank;
    int AOSRank;
    bool immutable;

    bool slice_mode;
    int slice_index;
    int timed_AOS_index;
    bool isTimed;
    bool dataset_already_extended_by_slicing;

    hsize_t dims[H5S_MAX_RANK];
    hsize_t maxdims[H5S_MAX_RANK];
    hsize_t largest_dims[H5S_MAX_RANK];
    hsize_t chunk_dims[H5S_MAX_RANK];

    int max(int num1, int num2);


  public:

     HDF5DataSetHandler();
    ~HDF5DataSetHandler();

    bool hasBeenExtended;
    bool dataSetExtended;

    hid_t dtype_id;
    hid_t dataspace_id;


    void setSliceIndex();

    void createOrOpenTensorizedDataSet(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index);

    void updateAOSShapesTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, int shapes_slice_index);

    void updateTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, std::vector < int >&current_arrctx_indices, std::set < hid_t > &dynamic_aos_already_extended_by_slicing);

    void extendTensorizedDataSet(int datatype, int dim, int *size, hid_t loc_id, hid_t dataset_id, int AOSRank, int *AOSSize);

    void resetExtensionState();
    void getAttributes(bool * isTimed, int *slice_index, int *timed_AOS_index) const;

    int getSliceIndex() const;

     std::string getName() const;

    int getRank() const;

    void getDims(hsize_t * dataspace_dims) const;

    void setNonSliceMode();

    void setSliceMode(Context * ctx, hid_t loc_id, int homogeneous_time);

    int getTimedShape(int *timed_AOS_index_);

    void setExtent();


};

#endif
