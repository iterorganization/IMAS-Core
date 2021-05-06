#include "hdf5_dataset_handler.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include <math.h>


HDF5DataSetHandler::HDF5DataSetHandler():dataset_id(-1), dataset_rank(-1), AOSRank(0), datatype(-1), immutable(true), shape_dataset(false), slice_mode(false), slices_extension(0), timed_AOS_index(-1), isTimed(false), timeWriteOffset(0), dtype_id(-1), dataspace_id(-1)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5DataSetHandler::~HDF5DataSetHandler()
{
    if (!immutable)
        H5Tclose(dtype_id);
    if (dataspace_id != -1) {
        H5Sclose(dataspace_id);
    }
}

void
 HDF5DataSetHandler::setSliceMode(Context * ctx)
{
    HDF5Utils hdf5_utils;
    isTimed = hdf5_utils.isTimed(ctx, &timed_AOS_index);
    slice_mode = true;
}


void HDF5DataSetHandler::setNonSliceMode()
{
    isTimed = false;
    timed_AOS_index = -1;
    slice_mode = false;
}


void HDF5DataSetHandler::getAttributes(bool *isTimed, int *timed_AOS_index) const
{
    *isTimed = this->isTimed;
    *timed_AOS_index = this->timed_AOS_index;
}

int HDF5DataSetHandler::getSlicesExtension() const
{
    return this->slices_extension;
}

std::string HDF5DataSetHandler::getName() const
{
    return this->tensorized_path;
}

bool HDF5DataSetHandler::isShapeDataset() const 
{
	return shape_dataset;
}

void HDF5DataSetHandler::setTimedAOSShape(int timedAOS_shape) {
    assert(slice_mode);
    this->timedAOS_shape = timedAOS_shape;
}

int HDF5DataSetHandler::getTimedAOSShape() const {
    assert(slice_mode);
    return this->timedAOS_shape;
}


int HDF5DataSetHandler::getRank() const
{
    return dataset_rank;
}

hsize_t * HDF5DataSetHandler::getDims()
{
    return dims;
}

hid_t HDF5DataSetHandler::getDataSpace()
{
    return dataspace_id;
}

int HDF5DataSetHandler::getSize() const
{
	int s = 1;
	for (int i = 0; i < dataset_rank; i++) {
            s *= dims[i];
        }
	return s;
}

int HDF5DataSetHandler::getShape(int axis) const
{
	if (axis < dataset_rank)
	  return dims[axis];
	return -1;
}


int HDF5DataSetHandler::getTimedShape(int *timed_AOS_index_)
{
    int timed_current_shape = -1;
    if (timed_AOS_index != -1) {
        *timed_AOS_index_ = timed_AOS_index;
        timed_current_shape = dims[timed_AOS_index];
    }

    return timed_current_shape;
}

int HDF5DataSetHandler::getTimeWriteOffset() const {
    assert(slice_mode);
    return timeWriteOffset;
}

void HDF5DataSetHandler::open(const char *dataset_name, hid_t loc_id, hid_t * dataset_id, int dim, int *size, int datatype, bool shape_dataset) {
        
		this->tensorized_path = std::string(dataset_name);
		
		if (datatype != ualconst::char_data) {
			if (dim > 0 && !isTimed && timed_AOS_index == -1) {
				this->slices_extension = size[dim - 1];
			}
	    }
		*dataset_id = H5Dopen2(loc_id, dataset_name, H5P_DEFAULT);
        if (*dataset_id < 0) {
            char error_message[200];
			if (slice_mode) {
				if (H5Lexists(loc_id, dataset_name, H5P_DEFAULT) == 0) {
					sprintf(error_message, "Dataset: %s is not present in the pulse file. Have you set this field when calling put() for inserting the first slice ?\n", dataset_name);
				}
			}
			else {
				sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name);
			}
            throw UALBackendException(error_message, LOG);
        }
        this->dataset_id = *dataset_id;
        if (dataspace_id != -1)
            H5Sclose(dataspace_id);
        dataspace_id = H5Dget_space(*dataset_id);
		this->dataset_rank = H5Sget_simple_extent_ndims(dataspace_id);
        if (dataset_rank < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to call H5Sget_simple_extent_ndims for: %s\n", dataset_name);
            throw UALBackendException(error_message, LOG);
        }
        herr_t status = H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
		memcpy(largest_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to call H5Sget_simple_extent_dims for: %s\n", dataset_name);
            throw UALBackendException(error_message, LOG);
        }
		
		if (datatype != ualconst::char_data) {
    		this->AOSRank = this->dataset_rank - dim;
		}
		else {
			if (dim == 1) {
				this->AOSRank = this->dataset_rank;
			}
			else {
				this->AOSRank = this->dataset_rank - 1;
			}
		}

		this->datatype = datatype;
		setType();
		this->shape_dataset = shape_dataset;
}

void HDF5DataSetHandler::showDims(std::string context) {
	std::cout << context << "-->showDims::dataset=" << getName() <<  std::endl;
    std::cout << context << "-->showDims::rank=" << dataset_rank <<  std::endl;
    std::cout << context << "-->showDims::AOSRank=" << AOSRank <<  std::endl;
    std::cout << context << "-->showDims::slices_extension=" << slices_extension <<  std::endl;
    //std::cout << context << "-->showDims::dynamic_AOS_slices_extension" << dynamic_AOS_slices_extension <<  std::endl;
    std::cout << context << "-->showDims::datatype=" << dtype_id <<  std::endl;
    for (int i = 0; i < dataset_rank; i++) {
       std::cout << context << "-->showDims::dims[" << i << "]=" << dims[i] << std::endl;
       std::cout << context << "-->showDims::largest_dims[" << i << "]=" << largest_dims[i] << std::endl;
    }		
}

void HDF5DataSetHandler::showAOSIndices(std::string context, std::vector<int> &AOS_indices) {
	std::cout << context << "-->showAOSIndices::dataset=" << getName() <<  std::endl;
	for (int i = 0; i < AOSRank; i++) {
		std::cout << context << "-->showAOSIndices::AOS_indices[" << i << "]=" << AOS_indices[i] << std::endl;
	}
}

void HDF5DataSetHandler::showAOSShapes(std::string context, std::vector<int> &AOS_shapes) {
	std::cout << context << "-->showAOSShapes::dataset=" << getName() <<  std::endl;
	for (int i = 0; i < AOSRank; i++) {
		std::cout << context << "-->showAOSShapes::AOS_shapes[" << i << "]=" << AOS_shapes[i] << std::endl;
	}
}

void HDF5DataSetHandler::create(const char *dataset_name, hid_t * dataset_id, int datatype, hid_t loc_id, int dim, int *size, int AOSRank, int *AOSSize, bool shape_dataset, bool compression_enabled) {
	
	assert(!slice_mode);

	this->tensorized_path = std::string(dataset_name);
    this->dataset_rank = dim + AOSRank;
    this->AOSRank = AOSRank;
	this->datatype = datatype;
	this->shape_dataset = shape_dataset;

	for (int i = 0; i < AOSRank; i++) { //AOS axis creation
			dims[i] = (hsize_t) AOSSize[i];
            largest_dims[i] = dims[i];
            maxdims[i] = H5S_UNLIMITED;
    }
	if (datatype != ualconst::char_data) {
        for (int i = 0; i < dim; i++) { //data axis creation
			dims[i + AOSRank] = (hsize_t) size[dim - i - 1];
            largest_dims[i + AOSRank] = dims[i + AOSRank];
            maxdims[i + AOSRank] = H5S_UNLIMITED;
        }
    } else {
        if (dim == 1) {
            this->dataset_rank = this->AOSRank;
        } else {
            //can be only dim = 2 for IMAS with char type
            this->dataset_rank = this->AOSRank + 1;
            dims[AOSRank] = (hsize_t) size[0]; //number of strings
            largest_dims[AOSRank] = dims[AOSRank];
			maxdims[AOSRank] = H5S_UNLIMITED;
        }
    }
	size_t type_size = setType();

	if (dataset_rank > 0) {
        	dataspace_id = H5Screate_simple(dataset_rank, dims, maxdims);
		}
		else {
			dataspace_id = H5Screate(H5S_SCALAR);
		}

	if (dataset_rank > 0) {
			float M = 2 * 1024 * 1024;
			size_t vmax = (size_t) floor(M / type_size);
			size_t vp = 1;
			size_t vn = 1;
			hsize_t chunk_dims_min[H5S_MAX_RANK];
	
			for (int i = 0; i < AOSRank; i++) {
				chunk_dims[i] = dims[i];
				chunk_dims_min[i] = 1;
				vp *= chunk_dims[i];
			}
	
			for (int i = AOSRank; i < dataset_rank; i++) {
				chunk_dims[i] = dims[i];
				chunk_dims_min[i] = 10;
				vn *= chunk_dims[i];
			}
	
			if (vp * vn > vmax) {
				//std::cout << "vp*vn > vmax"  << std::endl;
				if (vn < vmax) {
					while (vp > vmax / vn) {
						size_t v = 1;
						for (int i = 0; i < AOSRank; i++) {
							float cs = (float) chunk_dims[i];
							cs /= pow(2.0, AOSRank);
							chunk_dims[i] = (int) cs;
							if (chunk_dims[i] < chunk_dims_min[i]) {
								chunk_dims[i] = chunk_dims_min[i];
								break;
							}
							v *= chunk_dims[i];
						}
						vp = v;
					}
					//std::cout << "vp = "  << vp <<  std::endl;
				} else {
					for (int i = 0; i < AOSRank; i++) {
						chunk_dims[i] = 1;
					}
					if (dataset_rank > AOSRank) {
						while (vn > vmax) {
							size_t v = 1;
							for (int i = AOSRank; i < dataset_rank; i++) {
								float cs = (float) chunk_dims[i];
								cs /= pow(2.0, dataset_rank - AOSRank);
								chunk_dims[i] = (int) cs;
								if (chunk_dims[i] < chunk_dims_min[i]) {
									chunk_dims[i] = chunk_dims_min[i];
									break;
								}
								v *= chunk_dims[i];
							}
							vn = v;
						}
						//std::cout << "vn = "  << vn <<  std::endl;
					}
				}
			}
			//size_t cs = vp*vn;
			hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
			H5Pset_chunk(dcpl_id, dataset_rank, chunk_dims);
			if (compression_enabled)
				H5Pset_deflate (dcpl_id, 1);
	
			if (!shape_dataset) {
				if (datatype == ualconst::integer_data) {
					int default_value = 999999999;
					H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
				} else if (datatype == ualconst::double_data) {
					double default_value = -9.0E40;
					H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
				}
			}
            else {
                int default_value = 0;
                H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
            }
			
			size_t rdcc_nbytes = 1000*1024*1024;
			size_t rdcc_nslots = H5D_CHUNK_CACHE_NSLOTS_DEFAULT;
			hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
			H5Pset_chunk_cache(dapl, rdcc_nslots, rdcc_nbytes, H5D_CHUNK_CACHE_W0_DEFAULT);
	
			*dataset_id = H5Dcreate2(loc_id, dataset_name, dtype_id, dataspace_id, H5P_DEFAULT, dcpl_id, dapl);
			H5Pclose(dapl);
			H5Pclose(dcpl_id);
		}
	else { //dataset_rank = 0
			*dataset_id = H5Dcreate2(loc_id, dataset_name, dtype_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	}

        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create HDF5 dataset: %s\n", dataset_name);
            throw UALBackendException(error_message);
        }
        this->dataset_id = *dataset_id;

	if (dataset_rank > 0)
        setExtent();
}

int HDF5DataSetHandler::setType() {

	int type_size;
	int dim = dataset_rank - AOSRank;

	switch (datatype) {

    case ualconst::integer_data:
        {
            dtype_id = H5T_NATIVE_INT;
            type_size = H5Tget_size(dtype_id);
            break;
        }
    case ualconst::double_data:
        {
            dtype_id = H5T_NATIVE_DOUBLE;
            type_size = H5Tget_size(dtype_id);
            break;
        }
	case ualconst::complex_data:
        {
			immutable = false;
			complex_t tmp;  /*used only to compute offsets */
            dtype_id = H5Tcreate (H5T_COMPOUND, sizeof tmp);
			H5Tinsert (dtype_id, "real", HOFFSET(complex_t,re), H5T_NATIVE_DOUBLE);
			H5Tinsert (dtype_id, "imaginary", HOFFSET(complex_t,im),H5T_NATIVE_DOUBLE);
            type_size = H5Tget_size(dtype_id);
            break;
        }
    case ualconst::char_data:
        {
            immutable = false;
            dtype_id = H5Tcreate(H5T_STRING, H5T_VARIABLE);
            type_size = H5Tget_size(dtype_id);
			
            if (dim == 1) {
            } else if (dim == 2) {
                type_size *= (hsize_t) dims[AOSRank];
            }
            herr_t tset = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
            if (tset < 0) {
                char error_message[100];
                sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", getName().c_str());
                throw UALBackendException(error_message);
            }
            break;
        }
	
    default:
        throw UALBackendException("Data type not supported", LOG);
    }
	return type_size;
}

void HDF5DataSetHandler::storeInitialDims() {
    assert(slice_mode);
    memcpy(initial_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));
}

void HDF5DataSetHandler::setTimeAxisOffset(const std::vector < int > &current_arrctx_indices, int dynamic_AOS_slices_extension) {
    assert(slice_mode);
    assert(dataset_id > 0);
    int dim = dataset_rank - AOSRank;
    if (!shape_dataset && datatype != ualconst::char_data && dynamic_AOS_slices_extension == 0 && (dim > 0 && !isTimed && timed_AOS_index == -1)) { //datasets for shapes arrays management are not concerned by this update
        timeWriteOffset = initial_dims[AOSRank];
        dims[AOSRank] = slices_extension; //increase coordinate along the time axis, done once after opening a dataset in slice mode
    }
    else if (!shape_dataset && datatype != ualconst::char_data && dynamic_AOS_slices_extension != 0 && (dim > 0 && !isTimed && timed_AOS_index == -1)) { //datasets for shapes arrays management are not concerned by this update
        timeWriteOffset = initial_dims[AOSRank];
        dims[AOSRank] = dynamic_AOS_slices_extension;
    }
    else if (isTimed && timed_AOS_index != -1 && timed_AOS_index <= AOSRank) { // shapes datasets are also concerned by this 
        int slice_index = current_arrctx_indices[timed_AOS_index];
        dims[timed_AOS_index] = initial_dims[timed_AOS_index] + slice_index + 1; //update coordinate along the time axis, initial_dims were the dims at beginning of the put_slice
    }
}

void HDF5DataSetHandler::updateTimeAxisOffset(const std::vector < int > &current_arrctx_indices) {
	assert(slice_mode);
	assert(dataset_id > 0);
	if (isTimed && timed_AOS_index != -1 && timed_AOS_index <= AOSRank) { // shapes datasets are also concerned by this 
        int slice_index = current_arrctx_indices[timed_AOS_index];
        dims[timed_AOS_index] = initial_dims[timed_AOS_index] + slice_index + 1; //update coordinate along the time axis, initial_dims were the dims at beginning of the put_slice
    }
}

void HDF5DataSetHandler::setTimeAxisOffsetForAOSDataSet() {
	assert(slice_mode);
	assert(dataset_id > 0);
	if (isTimed && timed_AOS_index != -1 && timed_AOS_index < AOSRank) { // shapes datasets are also concerned by this update
		dims[timed_AOS_index]++; //increase coordinate along the time axis, done once after opening a dataset in slice mode
	}
}

void HDF5DataSetHandler::extendDataSpaceForTimeSlices(int *size, int *AOSShapes, int dynamic_AOS_slices_extension) {
	assert(slice_mode);
	setCurrentShapes(size, AOSShapes);
	int dim = dataset_rank - AOSRank;

    if (!shape_dataset && datatype != ualconst::char_data && (dim > 0 && !isTimed && timed_AOS_index == -1)) {
        largest_dims[AOSRank] = dims[AOSRank] + slices_extension;
        //std::cout << "slices_extension = " << slices_extension << std::endl;
    } else if (isTimed && timed_AOS_index != -1 && timed_AOS_index <= AOSRank) {
        largest_dims[timed_AOS_index] = dims[timed_AOS_index] + dynamic_AOS_slices_extension;
    } 
    else{
        //std::cout << "no extension has occurred for " << getName() << std::endl;
    }
	if (dataset_rank > 0)
	   setExtent();
}

void HDF5DataSetHandler::extendDataSpaceForTimeSlicesForAOSDataSet(int *size, int *AOSShapes, int dynamic_AOS_slices_extension) {
	assert(slice_mode);
	setCurrentShapes(size, AOSShapes);

    if (isTimed && timed_AOS_index != -1 && timed_AOS_index < AOSRank) {
		largest_dims[timed_AOS_index] = dims[timed_AOS_index] + dynamic_AOS_slices_extension;
	} else{
		//std::cout << "no extension has occurred for " << getName() << std::endl;
	}
	if (dataset_rank > 0)
	   setExtent();
}

void HDF5DataSetHandler::setCurrentShapesAndExtend(int *size, int *AOSShapes) {
	setCurrentShapes(size, AOSShapes);
	if (dataset_rank > 0)
	   setExtent();
}

void HDF5DataSetHandler::setCurrentShapes(int *size, int *AOSShapes) {
	
	for (int i = 0; i < AOSRank; i++) {
			if (! (slice_mode && isTimed && i == timed_AOS_index)) { //we don't update the time axis in slice mode of a timed AOS
				dims[i] = (hsize_t) AOSShapes[i];
			}
			if (dims[i] > largest_dims[i]) {
				largest_dims[i] = dims[i];
			}
    }

	if (datatype != ualconst::char_data) {

        int dim = dataset_rank - AOSRank;

        for (int i = 0; i < dim; i++) {
			if (! (slice_mode && !isTimed && i == 0)) { //we don't update the time axis in slice mode of a dynamic array
				dims[i + AOSRank] = (hsize_t) size[dim - i - 1];
			}
			if (dims[i + AOSRank] > largest_dims[i + AOSRank]) {
				largest_dims[i + AOSRank] = dims[i + AOSRank];
			}
		}
    } else {
        if (dataset_rank == AOSRank) { //STR_0D type
        } else if (dataset_rank == AOSRank + 1) { //STR_1D type 
            dims[AOSRank] = (hsize_t) size[0];
			if (dims[AOSRank] > largest_dims[AOSRank]) {
				largest_dims[AOSRank] = dims[AOSRank];
			}
		}
    }
}

void HDF5DataSetHandler::setCurrentShapesAndExtendForAOSDataSet(int *size, int *AOSShapes) {
	setCurrentShapesForAOSDataSet(size, AOSShapes);
	if (dataset_rank > 0)
	   setExtent();
}

void HDF5DataSetHandler::setCurrentShapesForAOSDataSet(int *size, int *AOSShapes) {
	
	for (int i = 0; i < AOSRank; i++) {
			if (! (slice_mode && isTimed && i == timed_AOS_index)) { //we don't update the time axis in slice mode of a timed AOS
				dims[i] = (hsize_t) AOSShapes[i];
			}
			if (dims[i] > largest_dims[i]) {
				largest_dims[i] = dims[i];
			}
    }

    int dim = dataset_rank - AOSRank;

    for (int i = 0; i < dim; i++) {
		if (! (slice_mode && !isTimed && i == 0)) { //we don't update the time axis in slice mode of a dynamic array
			dims[i + AOSRank] = (hsize_t) size[dim - i - 1];
		}
		if (dims[i + AOSRank] > largest_dims[i + AOSRank]) {
			largest_dims[i + AOSRank] = dims[i + AOSRank];
		}
	}
}

void HDF5DataSetHandler::setExtent()
{
	assert(dataset_rank > 0);
	herr_t status = (int) H5Dset_extent(dataset_id, largest_dims);

    if (status < 0) {
        char error_message[100];
        sprintf(error_message, "Unable to extend the existing dataset: %d\n", (int) dataset_id);
        throw UALBackendException(error_message);
    }
    if (dataspace_id != -1) {
        H5Sclose(dataspace_id);
    }
    dataspace_id = H5Dget_space(dataset_id);
}
