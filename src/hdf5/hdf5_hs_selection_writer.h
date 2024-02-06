#ifndef HDF5_HSSELECTIONWRITER_H
#define HDF5_HSSELECTIONWRITER_H 1

#include <hdf5.h>
#include <vector>
#include <string>
#include "hdf5_dataset_handler.h"


class HDF5HsSelectionWriter {
  private:

    int dataset_rank;

    hsize_t offset[H5S_MAX_RANK];
    hsize_t count[H5S_MAX_RANK];

    hsize_t dims[H5S_MAX_RANK];
    hsize_t offset_out[H5S_MAX_RANK];
    hsize_t count_out[H5S_MAX_RANK];

    hsize_t memspace_dims_copy[H5S_MAX_RANK];

    bool memSpaceHasChanged(hsize_t * dims);

  public:

     HDF5HsSelectionWriter();
    ~HDF5HsSelectionWriter();

    hid_t memspace;
    void setHyperSlabs(hid_t dataset_id, const std::vector < int >&current_arrctx_indices, int slice_mode, hid_t dataspace, int rank, bool isShapeDataSet, bool isTimed, int timed_AOS_index, int timeWriteOffset, const hsize_t *dataspace_dims, int dynamic_AOS_slices_extension);

};

#endif
