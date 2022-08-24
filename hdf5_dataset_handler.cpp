#include "hdf5_dataset_handler.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_hs_selection_writer.h"
#include <math.h>


HDF5DataSetHandler::HDF5DataSetHandler(bool writing_mode_, const std::string &options_):dataset_rank(-1), AOSRank(0), immutable(true), 
shape_dataset(false), slice_mode(false), slices_extension(0), timed_AOS_index(-1), isTimed(false), timeWriteOffset(0), datatype(-1), dataset_id(-1), 
dtype_id(-1), request_dim(-1), dataspace_id(-1), compression_enabled(true), useBuffering(true), chunk_cache_size(READ_CHUNK_CACHE_SIZE), write_chunk_cache_size(WRITE_CHUNK_CACHE_SIZE), requests_arrctx_indices(), requests_shapes(), full_int_data_set_buffer(NULL), full_double_data_set_buffer(NULL)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    HDF5Utils hdf5_utils;
    bool readBuffering;
	bool writeBuffering;
	hdf5_utils.setDefaultOptions(&chunk_cache_size, &write_chunk_cache_size, &readBuffering, &writeBuffering);
	hdf5_utils.readOptions(options_, &compression_enabled, &readBuffering, &chunk_cache_size,  &writeBuffering,  &write_chunk_cache_size, &HDF5Utils::debug);
	if (writing_mode_) {
		useBuffering = writeBuffering;
	}
	else {
		useBuffering = readBuffering;
	}
}

HDF5DataSetHandler::~HDF5DataSetHandler()
{
    if (!immutable)
        H5Tclose(dtype_id);
    if (dataspace_id != -1) {

        /*if (shape_dataset && tensorized_path.find("AOS") == std::string::npos) {
            //std::cout << "calling writeBuffer for " << getName() << std::endl;
            writeBuffer();
        }*/
        H5Sclose(dataspace_id);
    }
}

bool HDF5DataSetHandler::operator==(const HDF5DataSetHandler &other) const {
    return dataset_id != -1 && dataset_id == other.dataset_id;
   }

void HDF5DataSetHandler::writeBuffer() {
    herr_t status = H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
    
            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to write HDF5 dataset for SHAPE: %s\n", tensorized_path.c_str());
                throw UALBackendException(error_message, LOG);
            }
            free(buffer);
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

int HDF5DataSetHandler::getAOSRank() const
{
   return AOSRank;
}

const hsize_t * HDF5DataSetHandler::getDims()
{
    return dims;
}

std::vector<int> HDF5DataSetHandler::getDimsAsVector() const
{
    std::vector<int> v;
    if ( (datatype != ualconst::char_data && request_dim == 0) || (datatype == ualconst::char_data && request_dim == 1) ) {
        return v;
    }
    v.assign(dims, dims + dataset_rank);
    return v;
}

int HDF5DataSetHandler::computeShapeFromDimsVector(std::vector<int> &v) {
    int shape = 1;
    for (int i = 0; i < request_dim; i++)
       shape *= v[dataset_rank - i - 1];
    return shape;
}

hsize_t * HDF5DataSetHandler::getLargestDims() 
{
    return largest_dims;
}

hid_t HDF5DataSetHandler::getDataSpace() const
{
    return dataspace_id;
}

int HDF5DataSetHandler::getSize() const
{
	size_t s = 1;
	for (int i = 0; i < dataset_rank; i++)
        s *= largest_dims[i];
	return s;
}

size_t HDF5DataSetHandler::getMaxShape(int axis) const
{
	assert(axis < dataset_rank);
	return largest_dims[axis];
}

int HDF5DataSetHandler::getShape(int axis) const
{
    assert(axis < dataset_rank);
    return dims[axis];
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

void HDF5DataSetHandler::open(const char *dataset_name, hid_t loc_id, hid_t * dataset_id, int dim, 
int *size, int datatype, bool shape_dataset, bool create_chunk_cache, const std::string &options, int AOSRank, int *AOSSize) {
        
		this->tensorized_path = std::string(dataset_name);
        this->request_dim = dim;
        
        disableBufferingIfNotSupported(datatype, dim);
		
		if (datatype != ualconst::char_data) {
			if (dim > 0 && !isTimed && timed_AOS_index == -1 && size) {
				this->slices_extension = size[dim - 1];
			}
	    }
        
        if (create_chunk_cache) {
            hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
            size_t rdcc_nbytes = chunk_cache_size;
            size_t rdcc_nslots = H5D_CHUNK_CACHE_NSLOTS_DEFAULT;
            H5Pset_chunk_cache(dapl, rdcc_nslots, rdcc_nbytes, H5D_CHUNK_CACHE_W0_DEFAULT);
            //printf("opening %s with chunk cache size = %d\n", dataset_name, (int) rdcc_nbytes);
		    *dataset_id = H5Dopen2(loc_id, dataset_name, dapl);
            H5Pclose(dapl);
        }
        else {
			//printf("opening %s without chunk cache size\n", dataset_name);
            *dataset_id = H5Dopen2(loc_id, dataset_name, H5P_DEFAULT);
        }
        
        if (*dataset_id < 0) {
            char error_message[200];
			if (slice_mode && H5Lexists(loc_id, dataset_name, H5P_DEFAULT) == 0) {
					assert(AOSRank != -1);
					assert(!shape_dataset);
					assert(AOSSize != NULL);
					std::unique_ptr < HDF5DataSetHandler > data_set(new HDF5DataSetHandler(true, options));
					data_set->setNonSliceMode();
					data_set->create(dataset_name, dataset_id, datatype, loc_id, dim, size, AOSRank, AOSSize, shape_dataset, create_chunk_cache);
			}
			else {
				sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name);
				throw UALBackendException(error_message, LOG);
			}
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

void HDF5DataSetHandler::disableBufferingIfNotSupported(int datatype, int dim) {
	if (dim > 1)
	   useBuffering = false;
}

void HDF5DataSetHandler::create(const char *dataset_name, hid_t * dataset_id, int datatype, hid_t loc_id, int dim, int *size, int AOSRank, int *AOSSize, bool shape_dataset, bool create_chunk_cache) {
	
	assert(!slice_mode);

	this->tensorized_path = std::string(dataset_name);
    this->dataset_rank = dim + AOSRank;
    this->AOSRank = AOSRank;
	this->datatype = datatype;
	this->shape_dataset = shape_dataset;
    this->request_dim = dim;
    
    disableBufferingIfNotSupported(datatype, dim);

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
        size_t rdcc_nbytes = write_chunk_cache_size;
		size_t rdcc_nslots = H5D_CHUNK_CACHE_NSLOTS_DEFAULT;
		size_t vp = 1;
		size_t vn = 1; //volume of the chunk, product of chunk size in each dimension
		hsize_t chunk_dims_min[H5S_MAX_RANK];
        float M = 2. * 1024. * 1024.; //in MBytes
        
        float Mmin = 0.1 * 1024.; //1 KByte
		size_t vmin = (size_t) floor(Mmin / type_size);

        size_t vmax = (size_t) floor(M / type_size);

		for (int i = 0; i < AOSRank; i++) {
			chunk_dims[i] = largest_dims[i];
			chunk_dims_min[i] = 1;
			vp *= chunk_dims[i];
		}

		for (int i = AOSRank; i < dataset_rank; i++) {
			chunk_dims[i] = largest_dims[i];
			chunk_dims_min[i] = 1;
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
		
		size_t v  = vp*vn;
		//printf("dataset_name=%s, volume=%d\n", dataset_name, v);
		if (v < vmin) {
			vn = (size_t) vmin/vp;
			for (int i = AOSRank; i < dataset_rank; i++) {
				float cs = pow((float) vn, 1./((float)dataset_rank - AOSRank));
				chunk_dims[i] = (int) cs;
			}
			
		}
		
		hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
		H5Pset_chunk(dcpl_id, dataset_rank, chunk_dims);
		if (compression_enabled) {
			H5Pset_shuffle(dcpl_id);
			H5Pset_deflate (dcpl_id, 1);
		}

		if (!shape_dataset) {
			if (datatype == ualconst::integer_data) {
				int default_value = -999999999;
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
		
        if (!create_chunk_cache) {
            rdcc_nbytes = 0;
        }
		hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
		//printf("using rdcc_nbytes=%d for dataset=%s\n", (int) rdcc_nbytes, getName().c_str());
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
            dtype_id = H5Tcopy (H5T_C_S1);
            H5Tset_size(dtype_id, H5T_VARIABLE);
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

void HDF5DataSetHandler::close()
{
    HDF5Utils hdf5_utils;
    if (useBuffering) {
        switch(datatype) {
            case ualconst::integer_data:
            {
                if (full_int_data_set_buffer != NULL)
                    free(full_int_data_set_buffer);
                int_data_set_buffer.clear();
                break;
            }
            case ualconst::double_data:
            {
                if (full_double_data_set_buffer != NULL)
                    free(full_double_data_set_buffer);
                double_data_set_buffer.clear();
                break;
            }
            case ualconst::char_data:
            {
                full_data_sets_buffers.clear();
                data_sets_buffers.clear();
                break;
            }
        }
    }
    assert(H5Dclose (dataset_id) >=0);
}

void HDF5DataSetHandler::setBuffering(bool useBufferingOption) {

    if (useBufferingOption && !slice_mode) {
        if (request_dim == 0 && datatype == ualconst::integer_data ) {
                useBuffering = true;
            }
            else if (request_dim == 0 && datatype == ualconst::double_data) {
                useBuffering = true;
            }
            else if (request_dim == 1 && datatype == ualconst::integer_data) {
                useBuffering = true;
            }
            else if (request_dim == 1 && datatype == ualconst::double_data) {
                useBuffering = true;
            }
            else if (request_dim == 1 && datatype == ualconst::char_data) {
                useBuffering = true;
            }
    }
    else
        useBuffering = false;
}

void HDF5DataSetHandler::write_buffers() {

    if (!useBuffering) return;

    fillFullBuffers();

    herr_t status = 0;
    switch(datatype) {

        case ualconst::integer_data:
            {
                int *v = full_int_data_set_buffer;
                status = H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
                break;
            }
    
        case ualconst::double_data:
            {
                double *v = full_double_data_set_buffer;
                status = H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
                break;
            }

        case ualconst::char_data:
            {
                std::vector<char *> &values = full_data_sets_buffers;
                char** v = &values[0];
                status = H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
                break;
            }
    }
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write HDF5 dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5DataSetHandler::fillFullBuffers() {

    HDF5Utils hdf5_utils;
    switch (datatype) {
        
    case ualconst::integer_data: 
        {   
            std::vector<int *> &buffers = int_data_set_buffer;
            if (buffers.size() == 0)
                break;

            assert(requests_arrctx_indices.size() == buffers.size());
            assert(requests_arrctx_indices.size() == requests_shapes.size());

            int* v = (int*) malloc(getSize()* sizeof(int));
            herr_t status = H5Dread(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to create full buffer for HDF5 dataset: %s\n", getName().c_str());
                throw UALBackendException(error_message, LOG);
            }

            auto it = requests_arrctx_indices.begin();
            int request = 0;
            HDF5Utils hdf5_utils;
            while (it != requests_arrctx_indices.end()) {
                assert(request < (int) buffers.size());
                const std::vector<int> &request_arrctx_indices = *it;
                int* buffer = buffers[request];
                int shape = computeShapeFromDimsVector(requests_shapes[request]);
                if (getRank() != request_dim) {
                    std::vector < int >index;
                    hdf5_utils.getDataIndex(getRank(), getLargestDims(), request_arrctx_indices, index);
                    for (int i = 0; i < shape; i++) 
                        v[index[i]] = buffer[i];
                }
                else {
                    if (request_dim == 0)
                        *v = *buffer;
                    else
                        memcpy(v, buffer, shape*sizeof(int));
                }
                ++it;
                request++;
                free(buffer);
            }
            full_int_data_set_buffer = v;
            break;
        }
    case ualconst::double_data: 
        {
            std::vector<double *> &buffers = double_data_set_buffer;
            if (buffers.size() == 0)
                break;

            assert(requests_arrctx_indices.size() == buffers.size());
            assert(requests_arrctx_indices.size() == requests_shapes.size());

            double* v = (double*) malloc(getSize()* sizeof(double));
            herr_t status = H5Dread(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to create full buffer for HDF5 dataset: %s\n", getName().c_str());
                throw UALBackendException(error_message, LOG);
            }

            auto it = requests_arrctx_indices.begin();
            int request = 0;
            HDF5Utils hdf5_utils;
            while (it != requests_arrctx_indices.end()) {
                const std::vector<int> &request_arrctx_indices = *it;
                double* buffer = buffers[request];
                int shape = computeShapeFromDimsVector(requests_shapes[request]);
                if (getRank() != request_dim) {
                    std::vector < int >index;
                    hdf5_utils.getDataIndex(getRank(), getLargestDims(), request_arrctx_indices, index);
                    for (int i = 0; i < shape; i++) 
                        v[index[i]] = buffer[i];
                }
                else {
                    if (request_dim == 0) {
                        *v = *buffer;
                    }
                    else {
                        memcpy(v, buffer, shape*sizeof(double));
                    }
                }
                ++it;
                request++;
                free(buffer);
            }
            full_double_data_set_buffer = v;
            break;
        }

    case ualconst::char_data: 
        {   
            std::vector<char *> &buffers = data_sets_buffers;
            if (buffers.size() == 0)
                break;

            assert(requests_arrctx_indices.size() == buffers.size());

            std::vector<char *> v;
            v.resize(getSize());
            auto it = requests_arrctx_indices.begin();
            int request = 0;
            HDF5Utils hdf5_utils;
            while (it != requests_arrctx_indices.end()) {
                const std::vector<int> &request_arrctx_indices = *it;
                assert(request < (int) buffers.size());
                char* buffer = buffers[request];
                assert(buffer != NULL);
                if (getRank() != 0) {
                    std::vector < int >index;
                    hdf5_utils.getDataIndex(getRank(), getLargestDims(), request_arrctx_indices, index);
                    v[index[0]] = buffer;
                }
                else {
                    v[0] = buffer;
                }
                ++it;
                request++;
            }
            full_data_sets_buffers = v;
            break;
        }
    }
}


void HDF5DataSetHandler::appendStringToBuffer(const std::vector < int >&current_arrctx_indices, char **data) {
    std::vector<char *> &v = data_sets_buffers;
    char *c = strdup((char*) *data);
    v.push_back(c);
}

void HDF5DataSetHandler::appendInt0DToBuffer(const std::vector < int >&current_arrctx_indices, void *data) {
    std::vector<int *> &v = int_data_set_buffer;
    int* data_int = (int*) data;
    int* p = (int*) malloc(sizeof(int));
    *p = *data_int;
    v.push_back(p);
}

void HDF5DataSetHandler::appendIntNDToBuffer(const std::vector < int >&current_arrctx_indices, void *data, int dim) {
    std::vector<int *> &v = int_data_set_buffer;
    int* data_int = (int*) data;
    size_t shape = 1;
    for (int i = 0; i < request_dim; i++)
        shape *= (size_t) getShape(getRank() - i - 1);
    int* p = (int*) malloc(sizeof(int)*shape);
    memcpy(p, data_int, shape*sizeof(int));
    v.push_back(p);
}

void HDF5DataSetHandler::appendDouble0DToBuffer(const std::vector < int >&current_arrctx_indices, void *data) {
    std::vector<double *> &v = double_data_set_buffer;
    double* data_double = (double*) data;
    double* p = (double*) malloc(sizeof(double));
    *p = *data_double;
    v.push_back(p);
}

void HDF5DataSetHandler::appendDoubleNDToBuffer(const std::vector < int >&current_arrctx_indices, void *data, int dim) {
    std::vector<double *> &v = double_data_set_buffer;
    double* data_double = (double*) data;
    size_t shape = 1;
    for (int i = 0; i < request_dim; i++)
        shape *= (size_t) getShape(getRank() - i - 1);
    double* p = (double*) malloc(sizeof(double)*shape);
    memcpy(p, data_double, shape*sizeof(double));
    v.push_back(p);
}

void HDF5DataSetHandler::writeUsingHyperslabs(const std::vector < int >&current_arrctx_indices, int slice_mode, int dynamic_AOS_slices_extension, void *data) {
    HDF5HsSelectionWriter hsSelectionWriter;
    hsSelectionWriter.setHyperSlabs(dataset_id, current_arrctx_indices, slice_mode, getDataSpace(), getRank(), shape_dataset, isTimed, timed_AOS_index, timeWriteOffset, getDims(), dynamic_AOS_slices_extension);
    herr_t status = H5Dwrite(dataset_id, dtype_id, hsSelectionWriter.memspace, getDataSpace(), H5P_DEFAULT, data);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write HDF5 dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5DataSetHandler::appendToBuffer(const std::vector < int >&current_arrctx_indices, bool dataSetAlreadyOpened, int datatype, int dim, int slice_mode, int dynamic_AOS_slices_extension, char**p, void *data) {

    requests_arrctx_indices.push_back(current_arrctx_indices);

    if (dim == 0 && datatype == ualconst::integer_data && slice_mode != SLICE_OP) {
            appendInt0DToBuffer(current_arrctx_indices, data);
        }
        else if (dim == 0 && datatype == ualconst::double_data && slice_mode != SLICE_OP) {
            appendDouble0DToBuffer(current_arrctx_indices, data);
        }
        else if (dim == 1 && datatype == ualconst::integer_data && slice_mode != SLICE_OP) {
            appendIntNDToBuffer(current_arrctx_indices, data, dim);
        }
        else if (dim == 1 && datatype == ualconst::double_data && slice_mode != SLICE_OP) {
            appendDoubleNDToBuffer(current_arrctx_indices, data, dim);
        }
        else if (dim == 1 && datatype == ualconst::char_data && slice_mode != SLICE_OP) {
            appendStringToBuffer(current_arrctx_indices, p);
        }
        else {
            writeUsingHyperslabs(current_arrctx_indices, slice_mode, dynamic_AOS_slices_extension, data);
        }
}

void HDF5DataSetHandler::read0DStringsFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    if (hsSelectionReader.getRank() != 0) {
        std::vector < int >index;
        HDF5Utils hdf5_utils;
        hdf5_utils.getDataIndex(hsSelectionReader.getRank(), hsSelectionReader.getDataSpaceDims(), current_arrctx_indices, index);
        *data = full_data_sets_buffers[index[0]];
    }
    else {
        *data = full_data_sets_buffers[0];
    }
}

void HDF5DataSetHandler::create0DStringsBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    size_t s = hsSelectionReader.getSize2();
    std::vector<char *> values;
    values.resize(s);
    char** v = &values[0];
    herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
    full_data_sets_buffers = values;
}

void HDF5DataSetHandler::readInt0DFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    int *v = full_int_data_set_buffer;
    assert(full_int_data_set_buffer != NULL);
    *data = (void*) malloc(sizeof(int));
    int* data_int = (int*) *data;
    if (hsSelectionReader.getRank() != 0) {
        std::vector < int >index;
        HDF5Utils hdf5_utils;
        hdf5_utils.getDataIndex(hsSelectionReader.getRank(), hsSelectionReader.getDataSpaceDims(), current_arrctx_indices, index);
        *data_int = v[index[0]];
    }
    else {
        *data_int = *v;
    }
}

void HDF5DataSetHandler::readIntNDFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    int *v = full_int_data_set_buffer;
    assert(full_int_data_set_buffer != NULL);
    int dim = hsSelectionReader.getDim();
    int rank = hsSelectionReader.getRank();
    size_t shape = 1;
    for (int i = 0; i < dim; i++)
        shape *= (size_t) hsSelectionReader.getShape(rank - i - 1);
    *data = (void*) malloc(shape*sizeof(int));
    int* data_int = (int*) *data;
    if (rank != dim) {
        std::vector < int >index;
        HDF5Utils hdf5_utils;
        hdf5_utils.getDataIndex(hsSelectionReader.getRank(), hsSelectionReader.getDataSpaceDims(), current_arrctx_indices, index);
        for (size_t i = 0; i < shape; i++) 
            data_int[i] = v[index[i]];
    }
    else {
        memcpy(data_int, v, shape*sizeof(int));
    }
}

void HDF5DataSetHandler::readDoubleNDFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    double *v = full_double_data_set_buffer;
    assert(full_double_data_set_buffer != NULL);
    int dim = hsSelectionReader.getDim();
    int rank = hsSelectionReader.getRank();
    size_t shape = 1;
    for (int i = 0; i < dim; i++)
        shape *= (size_t) hsSelectionReader.getShape(rank - i - 1);
    *data = (void*) malloc(shape*sizeof(double));
    double* data_double = (double*) *data;
    if (rank != dim) {
        std::vector < int >index;
        HDF5Utils hdf5_utils;
        hdf5_utils.getDataIndex(hsSelectionReader.getRank(), hsSelectionReader.getDataSpaceDims(), current_arrctx_indices, index);
        for (size_t i = 0; i < shape; i++) 
            data_double[i] = v[index[i]];
    }
    else {
        memcpy(data_double, v, shape*sizeof(double));
    }
}

void HDF5DataSetHandler::createIntBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    size_t s = hsSelectionReader.getSize2();
    full_int_data_set_buffer = (int*) malloc(s* sizeof(int));
    herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, full_int_data_set_buffer);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5DataSetHandler::readDouble0DFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    *data = (void*) malloc(sizeof(double));
    double* data_double = (double*) *data;
    if (hsSelectionReader.getRank() != 0) {
        std::vector < int >index;
        HDF5Utils hdf5_utils;
        hdf5_utils.getDataIndex(hsSelectionReader.getRank(), hsSelectionReader.getDataSpaceDims(), current_arrctx_indices, index);
        *data_double = full_double_data_set_buffer[index[0]];
    }
    else {
        *data_double = *full_double_data_set_buffer;
    }
}

void HDF5DataSetHandler::createDoubleBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data) {
    size_t s = hsSelectionReader.getSize2();
    //printf("calling createDoubleBuffer for %s, id=%d, size(MB)=%f\n", getName().c_str(), (int) dataset_id, (double) ((s * sizeof(double))/1024./1024.));
    full_double_data_set_buffer = (double*) malloc(s* sizeof(double));
    herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, full_double_data_set_buffer);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5DataSetHandler::readUsingHyperslabs(const std::vector < int >&current_arrctx_indices, int slice_mode, bool is_dynamic, bool isTimed, int timed_AOS_index, int slice_index, void **data, bool read_strings) {
    HDF5HsSelectionReader & hsSelectionReader = *selection_reader;
    hsSelectionReader.setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, current_arrctx_indices);
    hsSelectionReader.allocateBuffer(data, slice_mode, is_dynamic, isTimed, slice_index);
    herr_t status = -1;
    if (!read_strings)
        status = H5Dread(dataset_id, hsSelectionReader.dtype_id, hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, *data);
    else
        status = H5Dread(dataset_id, hsSelectionReader.dtype_id, hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, (char **) data);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read dataset: %s\n", getName().c_str());
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5DataSetHandler::readData(const std::vector < int >&current_arrctx_indices, int datatype, int dim, int slice_mode, bool is_dynamic, bool isTimed, int timed_AOS_index, int slice_index, void **data) {
    if (useBuffering) {
        HDF5HsSelectionReader & hsSelectionReader = *selection_reader;
        if (datatype != ualconst::char_data) {
            if (dim == 0 && datatype == ualconst::integer_data && slice_mode != SLICE_OP) {
                if (full_int_data_set_buffer==NULL)
                    createIntBuffer(hsSelectionReader, current_arrctx_indices, data);
                readInt0DFromBuffer(hsSelectionReader, current_arrctx_indices, data);
            }
            else if (dim == 0 && datatype == ualconst::double_data && slice_mode != SLICE_OP) {
                if (full_double_data_set_buffer==NULL)
                    createDoubleBuffer(hsSelectionReader, current_arrctx_indices, data);
                readDouble0DFromBuffer(hsSelectionReader, current_arrctx_indices, data);
            }
            else if (dim == 1 && datatype == ualconst::integer_data && slice_mode != SLICE_OP) {
                if (full_int_data_set_buffer==NULL)
                    createIntBuffer(hsSelectionReader, current_arrctx_indices, data);
                readIntNDFromBuffer(hsSelectionReader, current_arrctx_indices, data);
            }
            else if (dim == 1 && datatype == ualconst::double_data && slice_mode != SLICE_OP) {
                if (full_double_data_set_buffer==NULL)
                    createDoubleBuffer(hsSelectionReader, current_arrctx_indices, data);
                readDoubleNDFromBuffer(hsSelectionReader, current_arrctx_indices, data);
            }
            else {
                readUsingHyperslabs(current_arrctx_indices, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, data, false);
            }
        } else {
            if (dim == 1 && slice_mode != SLICE_OP) {
                if (full_data_sets_buffers.size() == 0)
                    create0DStringsBuffer(hsSelectionReader, current_arrctx_indices, data);
                read0DStringsFromBuffer(hsSelectionReader, current_arrctx_indices, data);
            }
            else {
                readUsingHyperslabs(current_arrctx_indices, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, data, datatype == ualconst::char_data);
            }
        }
    }
    else {
        readUsingHyperslabs(current_arrctx_indices, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, data, datatype == ualconst::char_data);
    }
}

bool HDF5DataSetHandler::isRequestInExtent(const std::vector < int >&current_arrctx_indices)
{
    if (current_arrctx_indices.size() == 0)
        return true;
    for (int i = 0; i < AOSRank; i++) {
        if (current_arrctx_indices[i] > int (largest_dims[i] - 1)) {
            return false;
		}
    }
    return true;
}
