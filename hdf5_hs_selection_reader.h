#ifndef HDF5_HSSELECTIONREADER_H
#define HDF5_HSSELECTIONREADER_H 1

#include <hdf5.h>
#include "ual_backend.h"

#include <memory>
#include <vector>
#include <list>
#include <map>

class HDF5HsSelectionReader {
  private:
    hid_t dataset_id;
    bool immutable;
    hid_t datatype;
    int dataset_rank;

    int AOSRank;

    hsize_t size[MAXDIM];
    int dim;

    hsize_t dataspace_dims[H5S_MAX_RANK];
    hsize_t offset[H5S_MAX_RANK];
    hsize_t count[H5S_MAX_RANK];
    hsize_t memspace_dims_copy[H5S_MAX_RANK];

    hsize_t offset_out[H5S_MAX_RANK];
    hsize_t count_out[H5S_MAX_RANK];

    void setBufferSize();
    void init(hid_t dataset_id, hid_t datatype_, int AOSRank_, int *dim);
    bool memSpaceHasChanged(hsize_t * dims);

  public:
     HDF5HsSelectionReader(hid_t dataset_id, hid_t datatype, int AOSRank_, int *dim);
    ~HDF5HsSelectionReader();

    hid_t dtype_id;
    size_t dtype_size;
    hid_t dataspace;
    hid_t memspace;
    size_t buffer_size;

    void setSize(int *size_, int dim);
    int getDim() const;
    int getRank() const;
    void getDims(hsize_t * dataspace_dims, size_t n) const;
    void getSize(int *size, int slice_mode, bool is_dynamic) const;
    void getDataIndex(std::vector < int >current_arrctx_indices, std::vector < int >&index);
    int getShape(int axis_index) const;
    bool isRequestInExtent(std::vector < int >&current_arrctx_indices);
    void updateDims(std::vector < int >&current_arrctx_shapes);
    void allocateGlobalOpBuffer(void **data);
    void allocateBuffer(void **data, int slice_mode, bool is_dynamic, bool isTimed, int slice_index);
    void allocateFullBuffer(void **data);
    void setHyperSlabsGlobalOp(std::vector < int >&current_arrctx_indices);
    void setHyperSlabs(int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int timed_AOS_index, std::vector < int >&current_arrctx_indices);
};

#endif
