#ifndef HDF5_HSSELECTIONREADER_H
#define HDF5_HSSELECTIONREADER_H 1

#include <hdf5.h>
#include "al_backend.h"

#include <memory>
#include <vector>
#include <list>
#include <map>

class HDF5HsSelectionReader {
  private:
    hid_t dataset_id;
    bool immutable;
    int datatype;
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
    void init(hid_t dataset_id, int datatype_, int AOSRank_, int *dim);
    bool memSpaceHasChanged(hsize_t * dims);

  public:
     HDF5HsSelectionReader(int dataset_rank_, hid_t dataset_id_, hid_t dataspace_, hsize_t *dims, int datatype, int AOSRank_, int *dim);
    ~HDF5HsSelectionReader();

    hid_t dtype_id;
    size_t dtype_size;
    hid_t dataspace;
    hid_t memspace;
    size_t buffer_size;

    size_t getSize2();
    void setSize(int *size_, int dim);
    int getDim() const;
    int getRank() const;
    void getSize(int *size, int slice_mode, bool is_dynamic) const;
    const hsize_t* getDataSpaceDims();
    int getShape(int axis_index) const;
    bool isRequestInExtent(const std::vector < int >&current_arrctx_indices);
    void allocateGlobalOpBuffer(void **data);
    int allocateBuffer(void **data, int slice_mode, bool is_dynamic, bool isTimed, int slice_index);
    int allocateFullBuffer(void **data);
    void allocateInhomogeneousTimeDataSet(void **data, int timed_AOS_index);
    void setHyperSlabsGlobalOp(std::vector < int >current_arrctx_indices, int timed_AOS_index = -1, bool count_along_dynamic_aos = false);
    void setHyperSlabs(int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int timed_AOS_index, std::vector < int >current_arrctx_indices, bool count_along_dynamic_aos = false);
};

#endif
