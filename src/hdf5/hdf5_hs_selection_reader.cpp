#include "hdf5_hs_selection_reader.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_dataset_handler.h"
#include <assert.h>

HDF5HsSelectionReader::HDF5HsSelectionReader(int dataset_rank_, hid_t dataset_id_, hid_t dataspace_, hsize_t *dims, int datatype_, int AOSRank_, int *dim):dataset_id(dataset_id_), immutable(true), datatype(datatype_), dataset_rank(dataset_rank_), AOSRank(AOSRank_), dtype_id(-1), dataspace(dataspace_), memspace(-1), buffer_size(0)
{
    memcpy(dataspace_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));
    init(dataset_id, datatype_, dim);
}

void HDF5HsSelectionReader::init(hid_t dataset_id, int datatype_, int *dim)
{
    switch (datatype) {

    case alconst::integer_data:
        {
            dtype_id = H5T_NATIVE_INT;
            break;
        }
    case alconst::double_data:
        {
            dtype_id = H5T_NATIVE_DOUBLE;
            break;
        }
	case alconst::complex_data:
        {
            immutable = false;
            dtype_id = H5Dget_type(dataset_id);
            break;
        }
    case alconst::char_data:
        {
            immutable = false;
            dtype_id = H5Dget_type(dataset_id);
            break;
        }
    }

    dtype_size = H5Tget_size(dtype_id);

    if (datatype != alconst::char_data) {
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
    this->time_range.enabled = false;

}

HDF5HsSelectionReader::~HDF5HsSelectionReader()
{
    if (memspace != -1) {
		if (memspace != H5S_ALL)
        	H5Sclose(memspace);
	}
    if (!immutable)
        H5Tclose(dtype_id);
}

const hsize_t* HDF5HsSelectionReader::getDataSpaceDims() {
    return this->dataspace_dims;
}

void
 HDF5HsSelectionReader::setSize(int *size_, int dim)
{
    for (int i = 0; i < dim; i++) {
        size[i] = size_[i];
    }
    setBufferSize();
}

void
 HDF5HsSelectionReader::setTimeRange(int dim, int timeRange)
{
    size[dim - 1] = timeRange;
    setBufferSize();
}


void HDF5HsSelectionReader::setBufferSize()
{

    buffer_size = dtype_size;

    if (datatype != alconst::char_data) {
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

void HDF5HsSelectionReader::allocateInhomogeneousTimeDataSet(void **data, int timed_AOS_index)
{
    if (timed_AOS_index != -1)
        *data = malloc(dtype_size*dataspace_dims[timed_AOS_index]);
    else
        allocateGlobalOpBuffer(data);
}

int HDF5HsSelectionReader::allocateBuffer(void **data, int slice_mode, bool is_dynamic, bool isTimed, int slice_index)
{
    size_t buffer = buffer_size;
    
    if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1)  //Slicing
    {
        buffer /= this->size[dim - 1];
    }

    //printf("buffer size = %d\n", buffer);

    if (datatype != alconst::char_data) {
        *data = malloc(buffer);
        if (*data == nullptr) {
             char error_message[200];
             sprintf(error_message, "Unable to allocate memory.\n");
             throw ALBackendException(error_message, LOG);
        }
    } else {
        data = (void **) malloc(sizeof(char *) * this->size[0]);
        if (data == nullptr) {
             char error_message[200];
             sprintf(error_message, "Unable to allocate memory.\n");
             throw ALBackendException(error_message, LOG);
        }
    }
    return buffer;
}

int HDF5HsSelectionReader::allocateFullBuffer(void **data)
{
    int size = 1;
    for (int i = 0; i < dataset_rank; i++) {
        size *= dataspace_dims[i];
    }
	//std::cout << "total buffer size = " << buffer * dtype_size << std::endl;
    if (datatype != alconst::char_data) {
        *data = malloc(size * dtype_size);
    }
    else {
        *data = malloc(size * sizeof(char*));
    }
    return size;
}

size_t HDF5HsSelectionReader::getSize2() {
    size_t size = 1;
    for (int i = 0; i < dataset_rank; i++) {
        size *= dataspace_dims[i];
    }
    return size;
}

void HDF5HsSelectionReader::setHyperSlabsGlobalOp(std::vector < int > current_arrctx_indices, int timed_AOS_index, bool count_along_dynamic_aos)
{
    setHyperSlabs(GLOBAL_OP, false, false, -1, timed_AOS_index, current_arrctx_indices, count_along_dynamic_aos);
}

void HDF5HsSelectionReader::setHyperSlabs(int slice_mode, bool is_dynamic, bool isTimed, int slice_index, 
int timed_AOS_index, std::vector < int > current_arrctx_indices, bool count_along_dynamic_aos)
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
    else if (time_range.enabled && time_range.dtime != -1 && isTimed && timed_AOS_index < (int) current_arrctx_indices.size()) {
        current_arrctx_indices[timed_AOS_index] = slice_index;
    }

    //printf("HDF5HsSelectionReader::setHyperSlabs::timed_AOS_index=%d, AOSRank=%d\n", timed_AOS_index, AOSRank);

    for (int i = 0; i < AOSRank; i++) {

        if (current_arrctx_indices.size() == 0 || count_along_dynamic_aos) {       //data are not located in an AOS
            offset[i] = 0;
        } else {
            offset[i] = current_arrctx_indices[i];
        }

        hsize_t aos_elements_count = 1;
        if (timed_AOS_index != -1 && count_along_dynamic_aos) {
            if (i == timed_AOS_index)
                aos_elements_count = dataspace_dims[timed_AOS_index];
        }
        count[i] = aos_elements_count;
    }

    for (int i = 0; i < dataset_rank - AOSRank; i++) {
        
        if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1 && (i == 0)) {
            offset[i + AOSRank] = slice_index;  // when sliced and not timed, offset_out = slice_index, count_out = 1
            count[i + AOSRank] = 1;
        } else {
            if (time_range.enabled && timed_AOS_index == -1) {
                offset[i + AOSRank] = time_range.tmin_index;
            }
            else {
                offset[i + AOSRank] = 0;
            }
            count[i + AOSRank] = size[dim - i - 1];
            int j = i + AOSRank;
            if ( (count[j] + offset[j]) > dataspace_dims[j])
                count[j] = dataspace_dims[j] - offset[j];
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

    herr_t status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);

	/*int n = H5Sget_select_npoints( dataspace );
       std::cout << "n(dataspace) = " << n << std::endl;*/

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create dataspace HDF5 hyperslab.\n");
        throw ALBackendException(error_message, LOG);
    }

    hsize_t dims[H5S_MAX_RANK];
    hsize_t maxdims[H5S_MAX_RANK];

    //creating selection in the memory dataspace
    for (int i = 0; i < AOSRank; i++) {
        dims[i] = 1;
        offset_out[i] = 0;
        count_out[i] = 1;
    }

    bool slicing = (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1);

    if (timed_AOS_index != -1 && count_along_dynamic_aos) { //used for getting the inhomogeneous time dataset
            dims[0] = dataspace_dims[timed_AOS_index];
            count_out[0] = dims[0];
    }
    else {
        for (int i = 0; i < dataset_rank - AOSRank; i++) {

            dims[i + AOSRank] = (hsize_t) size[dim - i - 1];
    
            if (slicing && i == 0)       //Taking the slice on the latest dimension (e.g time dimension) if required
            {
                offset_out[i + AOSRank] = 0;        // when sliced and not timed, offset_out = 0, count_out = 1
                count_out[i + AOSRank] = 1;
                dims[i + AOSRank] = 1;
            } else {
                offset_out[i + AOSRank] = 0;
                /*count_out[i + AOSRank] = size[dim - i - 1];*/
                count_out[i + AOSRank] = count[i + AOSRank];
            }
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
       std::cout << "---------memory:: dataspace dimensions in setHyperSlabs" << std::endl;*/


    bool msHasChanged = memSpaceHasChanged(dims);
    if (memspace == -1 || msHasChanged) {
        if (memspace != -1)
            H5Sclose(memspace);
		//patch for HDF51.8.6: see https://forum.hdfgroup.org/t/dimensions-of-length-zero/2130/7
		for (int i = 0; i < dataset_rank; i++) {
			maxdims[i] = dims[i];
			if (dims[i] == 0)
			   maxdims[i] = H5S_UNLIMITED;
		}
        memspace = H5Screate_simple(dataset_rank, dims, maxdims);
        memcpy(memspace_dims_copy, dims, H5S_MAX_RANK * sizeof(hsize_t));
    }

    status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL, count_out, NULL);
		/*	int n2 = H5Sget_select_npoints( dataspace );
       std::cout << "n(memspace) = " << n2 << std::endl;*/

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create memory HDF5 hyperslab\n");
        throw ALBackendException(error_message, LOG);
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

int HDF5HsSelectionReader::getShape(int axis_index) const
{
    return dataspace_dims[axis_index];
}

bool HDF5HsSelectionReader::isRequestInExtent(const std::vector < int >&current_arrctx_indices)
{
    if (current_arrctx_indices.size() == 0)
        return true;
    for (int i = 0; i < AOSRank; i++) {
        if (current_arrctx_indices[i] > int (dataspace_dims[i] - 1)) {
            //printf("current_arrctx_indices[%d]=%d, largest_dims[%d] - 1 = %d\n", i, current_arrctx_indices[i], i, int (dataspace_dims[i] - 1));
            return false;
        }
    }
    return true;
}
