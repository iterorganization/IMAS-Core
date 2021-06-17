#include "hdf5_hs_selection_reader.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_dataset_handler.h"

HDF5HsSelectionReader::HDF5HsSelectionReader(hid_t dataset_id_, int datatype_, int AOSRank_, int *dim):dataset_id(dataset_id_), immutable(true), datatype(datatype_), dataset_rank(-1), AOSRank(AOSRank_), dtype_id(-1), dataspace(-1), memspace(-1), buffer_size(0)
{
    dataspace = H5Dget_space(dataset_id);
    dataset_rank = H5Sget_simple_extent_ndims(dataspace);
    herr_t status = H5Sget_simple_extent_dims(dataspace, dataspace_dims, NULL);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to call H5Sget_simple_extent_dims for space id: %d\n", (int) dataspace);
        throw UALBackendException(error_message, LOG);
    }
    init(dataset_id, datatype_, AOSRank_, dim);
}

void HDF5HsSelectionReader::init(hid_t dataset_id, int datatype_, int AOSRank_, int *dim)
{
    switch (datatype) {

    case ualconst::integer_data:
        {
            dtype_id = H5T_NATIVE_INT;
            break;
        }
    case ualconst::double_data:
        {
            dtype_id = H5T_NATIVE_DOUBLE;
            break;
        }
	case ualconst::complex_data:
        {
            immutable = false;
            dtype_id = H5Dget_type(dataset_id);
            break;
        }
    case ualconst::char_data:
        {
            immutable = false;
            dtype_id = H5Dget_type(dataset_id);
            break;
        }
    }

    dtype_size = H5Tget_size(dtype_id);

    if (datatype != ualconst::char_data) {
        *dim = dataset_rank - AOSRank;
        for (int i = 0; i < dataset_rank - AOSRank; i++) {
            size[*dim - 1 - i] = (int) dataspace_dims[i + AOSRank];
        }
    } else {
        if (dataset_rank == AOSRank)    //handling strings (dim = 1)
        {
            *dim = 1;
            size[0] = 1;
        } else                  //handling list of strings (dim = 2)                        
        {
            *dim = 2;
            size[1] = dtype_size;
            size[0] = (int) dataspace_dims[AOSRank];
        }
    }
    setBufferSize();
    this->dim = *dim;

}

HDF5HsSelectionReader::~HDF5HsSelectionReader()
{
    if (dataspace != -1) {
		if (dataspace != H5S_ALL)
        	H5Sclose(dataspace);
	}
    if (memspace != -1) {
		if (memspace != H5S_ALL)
        	H5Sclose(memspace);
	}
    if (!immutable)
        H5Tclose(dtype_id);
}

void
 HDF5HsSelectionReader::setSize(int *size_, int dim)
{
    for (int i = 0; i < dim; i++) {
        size[i] = size_[i];
    }
    setBufferSize();
}


void HDF5HsSelectionReader::setBufferSize()
{

    buffer_size = dtype_size;

    if (datatype != ualconst::char_data) {
        for (int i = 0; i < dataset_rank - AOSRank; i++) {
            buffer_size *= (size_t) size[i];
        }
    } else {
        if (dataset_rank == AOSRank)    // strings (dim = 1)
        {
            //buffer_size == dtype_size;
        } else                  //list of strings (dim = 2)
        {
            buffer_size *= (size_t) size[0];
        }
    }
}

int HDF5HsSelectionReader::getDim() const
{
    return dim;
}

int HDF5HsSelectionReader::getRank() const
{
    return dataset_rank;
}

void HDF5HsSelectionReader::getSize(int *size, int slice_mode, bool is_dynamic) const
{

    if (slice_mode == GLOBAL_OP) {
        if (dim > 0) {
            for (int i = 0; i < dim; i++)
                size[i] = this->size[i];
        }
    } else {
        if (dim > 0) {
            for (int i = 0; i < dim; i++)
                size[i] = this->size[i];
            if (is_dynamic)
                size[dim - 1] = 1;
        }
    }
}

void HDF5HsSelectionReader::allocateGlobalOpBuffer(void **data)
{
    allocateBuffer(data, GLOBAL_OP, false, false, -1);
}

int HDF5HsSelectionReader::allocateBuffer(void **data, int slice_mode, bool is_dynamic, bool isTimed, int slice_index)
{
    int buffer = buffer_size;

    if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1)  //Slicing
    {
        buffer /= this->size[dim - 1];
    }

    if (datatype != ualconst::char_data) {
        *data = malloc(buffer);
    } else {
        data = (void **) malloc(sizeof(char *) * this->size[0]);
    }
    return buffer;
}

void HDF5HsSelectionReader::allocateFullBuffer(void **data)
{
    int buffer = dataspace_dims[0];
    for (int i = 1; i < dataset_rank; i++) {
        buffer *= dataspace_dims[i];
    }
	//std::cout << "total buffer size = " << buffer * dtype_size << std::endl;
    *data = malloc(buffer * dtype_size);
}

void HDF5HsSelectionReader::setHyperSlabsGlobalOp(std::vector < int >&current_arrctx_indices)
{
    setHyperSlabs(GLOBAL_OP, false, false, -1, -1, current_arrctx_indices);
}

void HDF5HsSelectionReader::setHyperSlabs(int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int timed_AOS_index, std::vector < int >&current_arrctx_indices)
{
    //Create hyperslabs
    //creating selection in the dataspace

	if (dataset_rank == 0) {
		dataspace = H5S_ALL;
		memspace = H5S_ALL;
		return;
	}

    int dim = dataset_rank - AOSRank;

    if (slice_mode == SLICE_OP && isTimed && timed_AOS_index < (int) current_arrctx_indices.size()) {
        current_arrctx_indices[timed_AOS_index] = slice_index;
    }

    for (int i = 0; i < AOSRank; i++) {

        if (current_arrctx_indices.size() == 0) {       //data is not located in an AOS
            offset[i] = 0;
        } else {
            offset[i] = current_arrctx_indices[i];
        }
        count[i] = 1;
    }

    for (int i = 0; i < dataset_rank - AOSRank; i++) {
        
        if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1 && (i == 0)) {
            offset[i + AOSRank] = slice_index;  // when sliced and not timed, offset_out = slice_index, count_out = 1
            count[i + AOSRank] = 1;
        } else {
            offset[i + AOSRank] = 0;
            count[i + AOSRank] = size[dim - i - 1];
        }
    }

/*std::cout << "-------->file:: dataspace dimensions in setHyperSlabs for AOSs" << std::endl;
      for (int i = 0; i < AOSRank; i++) {
      std::cout << "dataspace_dims[" << i << "] = " <<  dataspace_dims[i] << std::endl;
      std::cout << "offset[" << i << "] = " <<  offset[i] << std::endl;
      std::cout << "count[" << i << "] = " <<  count[i] << std::endl;
 }
 std::cout << "-------->file:: end of dataspace dimensions in setHyperSlabs for AOSs" << std::endl;
 
 std::cout << "-------->file:: dataspace dimensions in setHyperSlabs for the array" << std::endl;
 for (int i = 0; i <  dataset_rank - AOSRank; i++) {
   std::cout << "dataspace_dims[" << i + AOSRank << "] = " <<  dataspace_dims[i + AOSRank] << std::endl;
   std::cout << "offset[" << i + AOSRank << "] = " <<  offset[i + AOSRank] << std::endl;
   std::cout << "count[" << i + AOSRank << "] = " <<  count[i + AOSRank] << std::endl;
 }
 std::cout << "---------file:: dataspace dimensions in setHyperSlabs" << std::endl;*/


    herr_t status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count,
                                        NULL);
	/*int n = H5Sget_select_npoints( dataspace );
       std::cout << "n(dataspace) = " << n << std::endl;*/

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create dataspace HDF5 hyperslab.\n");
        throw UALBackendException(error_message, LOG);
    }

    hsize_t dims[H5S_MAX_RANK];

//creating selection in the memory dataspace
    for (int i = 0; i < AOSRank; i++) {
        dims[i] = 1;
        offset_out[i] = 0;
        count_out[i] = 1;
    }

    bool slicing = (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1);

    for (int i = 0; i < dataset_rank - AOSRank; i++) {
        dims[i + AOSRank] = (hsize_t) size[dim - i - 1];

        if (slicing && i == 0)       //Taking the slice on the latest dimension (e.g time dimension) if required
        {
            offset_out[i + AOSRank] = 0;        // when sliced and not timed, offset_out = 0, count_out = 1
            count_out[i + AOSRank] = 1;
            dims[i + AOSRank] = 1;
        } else {
            offset_out[i + AOSRank] = 0;
            count_out[i + AOSRank] = size[dim - i - 1];
        }
    }

    /*std::cout << "-------->memory:: dataspace dimensions in setHyperSlabs for AOSs" << std::endl;
       for (int i = 0; i < AOSRank; i++) {
       std::cout << "dims[" << i << "] = " <<  dims[i] << std::endl;
       std::cout << "offset[" << i << "] = " <<  offset_out[i] << std::endl;
       std::cout << "count[" << i << "] = " <<  count_out[i] << std::endl;
       }
       std::cout << "-------->memory:: end of dataspace dimensions in setHyperSlabs for AOSs" << std::endl;

       std::cout << "-------->memory:: dataspace dimensions in setHyperSlabs for the array" << std::endl;
       for (int i = 0; i <  dataset_rank - AOSRank; i++) {
       std::cout << "dims[" << i + AOSRank << "] = " <<  dims[i + AOSRank] << std::endl;
       std::cout << "offset[" << i + AOSRank << "] = " <<  offset_out[i + AOSRank] << std::endl;
       std::cout << "count[" << i + AOSRank << "] = " <<  count_out[i + AOSRank] << std::endl;
       }
       std::cout << "---------memory:: dataspace dimensions in setHyperSlabs" << std::endl; 
*/

    bool msHasChanged = memSpaceHasChanged(dims);
    if (memspace == -1 || msHasChanged) {
        if (memspace != -1)
            H5Sclose(memspace);
        memspace = H5Screate_simple(dataset_rank, dims, NULL);
        memcpy(memspace_dims_copy, dims, H5S_MAX_RANK * sizeof(hsize_t));
    }

    status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL, count_out, NULL);
		/*	int n2 = H5Sget_select_npoints( dataspace );
       std::cout << "n(memspace) = " << n2 << std::endl;*/

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create memory HDF5 hyperslab\n");
        throw UALBackendException(error_message, LOG);
    }

}

bool HDF5HsSelectionReader::memSpaceHasChanged(hsize_t * dims)
{
    for (int i = 0; i < dataset_rank; i++) {
        if (dims[i] != memspace_dims_copy[i]) {
            return true;
        }
    }
    return false;
}

/*bool HDF5HsSelectionReader::fileSpaceHasChanged() {
  for (int i = 0; i < dataset_rank; i++) {
    if (dataspace_dims[i] != filespace_dims_copy[i]) {
      return true;
    }
  }
  return false;
}*/

void HDF5HsSelectionReader::getDataIndex(std::vector < int >current_arrctx_indices, std::vector < int >&index)
{
    int n = dataspace_dims[dataset_rank - 1];   //n is the length of the vector of shapes
    std::vector < int >basis_tmp;
    basis_tmp.reserve(dataset_rank);
    basis_tmp.push_back(1);

    for (int i = 1; i < dataset_rank; i++) {
        basis_tmp[i] = basis_tmp[i - 1] * dataspace_dims[dataset_rank - i];
    }
    std::vector < int >basis;
    basis.reserve(dataset_rank);
    for (int i = 0; i < dataset_rank; i++) {
        basis[i] = basis_tmp[dataset_rank - i - 1];
    }
    current_arrctx_indices.push_back(0);        //adding the shapes axis; v targets to the first component of the shape vector
    index.reserve(n);           //index[i] is the index of the ith component of the shapes vector in the linearized buffer

    for (size_t j = 0; j < (size_t) n; j++) {
        index[j] = 0;
        for (size_t i = 0; i < (size_t) dataset_rank; i++) {
            index[j] += current_arrctx_indices[i] * basis[i];
        }
        current_arrctx_indices.back() += 1;     //increasing component along shapes axis
    }
}

int HDF5HsSelectionReader::getShape(int axis_index) const
{
    return dataspace_dims[axis_index];
}

bool HDF5HsSelectionReader::isRequestInExtent(std::vector < int >&current_arrctx_indices)
{
    if (current_arrctx_indices.size() == 0)
        return true;
    for (int i = 0; i < AOSRank; i++) {
        if (current_arrctx_indices[i] > int (dataspace_dims[i] - 1))
            return false;
    }
    return true;
}
