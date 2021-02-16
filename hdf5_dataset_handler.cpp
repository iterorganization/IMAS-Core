#include "hdf5_dataset_handler.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include <math.h>


HDF5DataSetHandler::HDF5DataSetHandler():dataset_id(-1), dataset_rank(-1), AOSRank(0), immutable(true), slice_mode(false), slice_index(-1), slices_extension(0), timed_AOS_index(-1), isTimed(false), dataset_already_extended_by_slicing(false), use_core_driver(false), dtype_id(-1), dataspace_id(-1)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5DataSetHandler::~HDF5DataSetHandler()
{
    if (!immutable)
        H5Tclose(dtype_id);
    if (dataset_id != -1) {
        if (use_core_driver)
            copy_to_disk();
        assert(H5Dclose(dataset_id) >=0);
    }
    if (dataspace_id != -1) {
        H5Sclose(dataspace_id);
    }
}

void
 HDF5DataSetHandler::setSliceMode(Context * ctx, int homogeneous_time)
{
    HDF5Utils hdf5_utils;
    isTimed = hdf5_utils.isTimed(ctx, &timed_AOS_index);
    slice_mode = true;
}


void HDF5DataSetHandler::setNonSliceMode()
{
    isTimed = false;
    slice_index = -1;
    timed_AOS_index = -1;
    slice_mode = false;
}


void HDF5DataSetHandler::getAttributes(bool * isTimed, int *slice_index, int *timed_AOS_index) const
{
    *isTimed = this->isTimed;
    *slice_index = this->slice_index;
    *timed_AOS_index = this->timed_AOS_index;
}

int HDF5DataSetHandler::getSliceIndex() const
{
    return this->slice_index;
}

int HDF5DataSetHandler::getSlicesExtension() const
{
    return this->slices_extension;
}

std::string HDF5DataSetHandler::getName() const
{
    return this->tensorized_path;
}

void HDF5DataSetHandler::setSliceIndex()
{
    if (slice_mode) {
        if (this->timed_AOS_index != -1) {
            slice_index = dims[this->timed_AOS_index];
        } else {
            slice_index = dims[dataset_rank - 1];
        }
    }
    dataset_already_extended_by_slicing = false;
}

int HDF5DataSetHandler::getRank() const
{
    return dataset_rank;
}

void HDF5DataSetHandler::getDims(hsize_t * dataspace_dims) const
{
    memcpy(dataspace_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));
}

hid_t HDF5DataSetHandler::getDataSpace()
{
    return dataspace_id;
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

void HDF5DataSetHandler::copy_to_disk()
{

//    hid_t dspace, dcpl, ftype, dst;
// 
//    assert((ftype = H5Dget_type(dataset_id)) >= 0);
// 
//    size_t dataset_size = H5Tget_size(ftype);
//    for (int i = 0; i < dataset_rank; ++i)
//     dataset_size *= largest_dims[i];
//    assert (dataset_size > 0);
//    
//    
//    char* buf;
// 
//     //std::cout << "copying dataset : " << tensorized_path.c_str() << std::endl;
// 
//    if (dataset_size < 65000) /* ~64K */
//     {
//      //std::cout << "copying dataset1 : " << tensorized_path.c_str() << std::endl;
//      if (H5Lexists(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
//         H5Ldelete(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT); 
//      }
//      
//       //std::cout << "creation of a compact dataset: " << tensorized_path.c_str() << std::endl;
//      /* create a compact destination and xfer the data */
//      assert((dcpl = H5Pcreate(H5P_DATASET_CREATE)) >= 0);
//      assert(H5Pset_layout(dcpl, H5D_COMPACT) >= 0);
//         assert(IDS_group_id >=0);
//      
//      /* we need to re-create the dataspace to get rid of potential
//              unlimited dimensions. */
//      assert((dspace = H5Screate_simple(dataset_rank, largest_dims, NULL)) >= 0);
//      assert((dst = H5Dcreate(IDS_group_id, tensorized_path.c_str(), ftype, dspace,
//                              H5P_DEFAULT, dcpl, H5P_DEFAULT)) >= 0);
//      assert(H5Sclose(dspace) >= 0);
//      assert(H5Pclose(dcpl) >= 0);
// 
//       try {
//         /* create a read/write buffer */
//          assert((buf = (char*) malloc(dataset_size)) != NULL);
//       } catch (const std::bad_alloc& e) {
//         std::cout << "Allocation failed: " << e.what() << '\n';
//       }
//       
//       //assert((mtype = H5Tget_native_type(ftype, H5T_DIR_ASCEND)) >= 0);
//       /* read the data from the source */
//       assert(H5Dread(dataset_id, ftype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) >= 0);
//       /* write the data to the destination */
//       assert(H5Dwrite(dst, ftype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) >= 0);
// 
//       free(buf);
//       assert(H5Dclose(dst) >= 0);
//     }
//   else
//     {
//      
//       /* nothing we can 4 now do other than copy */
//       assert(dataset_id >=0);
//       assert(IDS_group_id >=0);
//       if (slice_mode) {
//        
//         if (H5Lexists(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) { 
//            //std::cout << "deleting link:  " << tensorized_path.c_str() << std::endl;
//         H5Ldelete(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT);
//         }
//       }
//         std::string src_name = "/tmp/" + tensorized_path;
//              assert(H5Ocopy(IDS_core_file_id, src_name.c_str() , IDS_group_id, tensorized_path.c_str() ,
//                      H5P_DEFAULT, H5P_DEFAULT) >= 0);
//       
//     }
// assert(H5Tclose(ftype) >= 0);
//std::cout << "end of copying dataset " << tensorized_path.c_str() << std::endl;

    assert(dataset_id >= 0);
    assert(IDS_group_id >= 0);
    if (slice_mode) {

        if (H5Lexists(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
            //std::cout << "deleting link:  " << tensorized_path.c_str() << std::endl;
            H5Ldelete(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT);
        }
    }
    std::string src_name = "/tmp/" + tensorized_path;
    assert(H5Ocopy(IDS_core_file_id, src_name.c_str(), IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT, H5P_DEFAULT) >= 0);

}

void HDF5DataSetHandler::updateAOSShapesTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes)
{
    HDF5Utils hdf5_utils;
    hsize_t dataspace_dims[H5S_MAX_RANK];
    memcpy(dataspace_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));

    if (*dataset_id == -1) {
        *dataset_id = H5Dopen2(loc_id, dataset_name.c_str(), H5P_DEFAULT);
        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
        if (dataspace_id != -1) {
            H5Sclose(dataspace_id);
        }
        dataspace_id = H5Dget_space(*dataset_id);
        herr_t status = H5Sget_simple_extent_dims(dataspace_id, dataspace_dims, NULL);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to call H5Sget_simple_extent_dims on dataset:%s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }

        dataset_rank = H5Sget_simple_extent_ndims(dataspace_id);
    }

    for (int i = 0; i < dataset_rank - AOSRank; i++)
        size[i] = hdf5_utils.max(size[i], (int) dataspace_dims[i + AOSRank]);

    int AOSSize_[H5S_MAX_RANK];
    for (int i = 0; i < AOSRank; i++)
        AOSSize_[i] = hdf5_utils.max(AOSShapes[i], (int) dataspace_dims[i]);


    if (slice_mode) {
        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSShapes);
    } else {
        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSSize_);
    }
    setExtent();
}

void HDF5DataSetHandler::updateTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, std::vector < int >&current_arrctx_indices, std::set < hid_t > &dynamic_aos_already_extended_by_slicing, bool shape_dataset, int dynamic_AOS_slices_extension)
{

    HDF5Utils hdf5_utils;
    hsize_t dataspace_dims[H5S_MAX_RANK];
    memcpy(dataspace_dims, dims, H5S_MAX_RANK * sizeof(hsize_t));

    if (*dataset_id == -1) {
        *dataset_id = H5Dopen2(loc_id, dataset_name.c_str(), H5P_DEFAULT);
        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }

        if (dataspace_id != -1) {
            H5Sclose(dataspace_id);
        }
        dataspace_id = H5Dget_space(*dataset_id);
        herr_t status = H5Sget_simple_extent_dims(dataspace_id, dataspace_dims, NULL);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to call H5Sget_simple_extent_dims on dataset:%s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
		memcpy(dims, dataspace_dims, H5S_MAX_RANK * sizeof(hsize_t));
        dataset_rank = H5Sget_simple_extent_ndims(dataspace_id);
    }

	//size is an array with updated shapes (because of a time slice or a change of AOS element)
	if (slice_mode) {
		if (dim > 0)
			slices_extension = size[dim - 1];
    }

	for (int i = 0; i < dataset_rank - AOSRank; i++) {
		size[i] = hdf5_utils.max(size[i], (int) dataspace_dims[i + AOSRank]); //we take the maximum extension in order to increase the size of the tensor if required
		if (i == dataset_rank - AOSRank - 1 && slice_mode)
			size[i] = dataspace_dims[i + AOSRank];
    }

    int AOSSize_[H5S_MAX_RANK];

    for (int i = 0; i < AOSRank; i++) {
        AOSSize_[i] = hdf5_utils.max(AOSShapes[i], (int) dataspace_dims[i]);
		if (i ==  timed_AOS_index && slice_mode)
			AOSSize_[i] = (int) dataspace_dims[i];
    }


    bool dynamic_aos_already_extended = false;

    if (!shape_dataset && slice_mode) {
        dynamic_aos_already_extended = dynamic_aos_already_extended_by_slicing.find(*dataset_id) != dynamic_aos_already_extended_by_slicing.end();
    }

    if (!shape_dataset && slice_mode && (!dataset_already_extended_by_slicing || !dynamic_aos_already_extended)) {
        if (!dataset_already_extended_by_slicing && timed_AOS_index == -1) {
			size[dim - 1] += slices_extension; //slicing is outside a dynamic AOS
			//std::cout << "increasing dimension of latest dimension to: " << size[dim -1] << std::endl;
        } else if (!dynamic_aos_already_extended && timed_AOS_index != -1 && timed_AOS_index < AOSRank) {       //slicing is inside a dynamic AOS
            AOSShapes[timed_AOS_index] += dynamic_AOS_slices_extension;
            dynamic_aos_already_extended_by_slicing.insert(*dataset_id);
        }
        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSShapes);
        dataset_already_extended_by_slicing = true;
    } else {
        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSSize_);
    }
}

void HDF5DataSetHandler::createOrOpenTensorizedDataSet2(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index)
{
    this->tensorized_path = std::string(dataset_name);
    dataset_rank = dim + AOSRank;
    this->AOSRank = AOSRank;
    this->timed_AOS_index = timed_AOS_index;


    if (datatype != ualconst::char_data) {
        for (int i = 0; i < AOSRank; i++) {
            dims[i] = (hsize_t) AOSSize[i];
            largest_dims[i] = dims[i];
            maxdims[i] = H5S_UNLIMITED;

        }

        for (int i = 0; i < dim; i++) {
            dims[i + AOSRank] = (hsize_t) size[i];
            largest_dims[i + AOSRank] = dims[i + AOSRank];
            maxdims[i + AOSRank] = H5S_UNLIMITED;
        }
    } else {
        if (dim == 1) {
            dataset_rank = AOSRank;     //we take a HDF5 space of rank 0 for dim=1, so it gives a rank equals to AOSRank for the tensorized dataset
            for (int i = 0; i < AOSRank; i++) {
                dims[i] = (hsize_t) AOSSize[i];
                largest_dims[i] = dims[i];
                maxdims[i] = H5S_UNLIMITED;

            }
        } else {
            //can be only dim = 2 for IMAS with char type (we take a HDF5 space of rank 1, so it gives AOSRank + 1 for the rank of this tensorized dataset)
            dataset_rank = AOSRank + 1;
            for (int i = 0; i < AOSRank; i++) {
                dims[i] = (hsize_t) AOSSize[i];
                largest_dims[i] = dims[i];
                maxdims[i] = H5S_UNLIMITED;

            }
            maxdims[AOSRank] = H5S_UNLIMITED;
            dims[AOSRank] = (hsize_t) size[0];
            largest_dims[AOSRank] = dims[AOSRank];
        }
    }

    if (create) {
        if (dataspace_id != -1) {
            H5Sclose(dataspace_id);
        }
        dataspace_id = H5Screate_simple(dataset_rank, dims, maxdims);
    } else {
        if (*dataset_id == -1)
            *dataset_id = H5Dopen2(loc_id, dataset_name, H5P_DEFAULT);
        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name);
            throw UALBackendException(error_message, LOG);
        }
        this->dataset_id = *dataset_id;
        if (dataspace_id != -1)
            H5Sclose(dataspace_id);
        dataspace_id = H5Dget_space(*dataset_id);
        herr_t status = H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to call H5Sget_simple_extent_dims for: %s\n", dataset_name);
            throw UALBackendException(error_message, LOG);
        }
    }

    size_t type_size;

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
                sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", dataset_name);
                throw UALBackendException(error_message);
            }
            break;
        }

    default:
        throw UALBackendException("Data type not supported", LOG);
    }

    if (create) {


        float M = 200 * 1024 * 1024;
        size_t vmax = (size_t) floor(M / type_size);
        size_t vp = 1;
        size_t vn = 1;
        float chunk_dims_fraction_factor = 0.1;
        hsize_t chunk_dims_min[H5S_MAX_RANK];

        for (int i = 0; i < AOSRank; i++) {
            //chunk_dims[i] = dims[i];
            chunk_dims[i] = std::max(1, (int) floor(dims[i] * chunk_dims_fraction_factor));
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
                            //chunk_dims[i] /= std::max(1, round(std::max(1, (int) pow(2.0, (dataset_rank - AOSRank)))));
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
		size_t cs = vp*vn;
        hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(dcpl_id, dataset_rank, chunk_dims);
        //H5Pset_deflate (dcpl_id, 1);

        if (!shape_dataset) {
            if (datatype == ualconst::integer_data) {
                int default_value = 999999999;
                H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
            } else if (datatype == ualconst::double_data) {
                double default_value = -9.0E40;
                H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
            }
        }
		
		size_t rdcc_nbytes = 500*1024*1024; //1GB
		size_t chunk_size = cs*type_size;
		if (chunk_size > rdcc_nbytes)
			rdcc_nbytes = 3*chunk_size;
        size_t rdcc_nslots = (size_t) 100*((float) rdcc_nbytes/ (float) chunk_size);
		if (rdcc_nslots > 300)
			rdcc_nslots = 300;
        hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
        H5Pset_chunk_cache(dapl, rdcc_nslots, rdcc_nbytes, H5D_CHUNK_CACHE_W0_DEFAULT);

        *dataset_id = H5Dcreate2(loc_id, dataset_name, dtype_id, dataspace_id, H5P_DEFAULT, dcpl_id, dapl);
        H5Pclose(dapl);
        H5Pclose(dcpl_id);

        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create HDF5 dataset: %s\n", dataset_name);
            throw UALBackendException(error_message);
        }
        this->dataset_id = *dataset_id;

    } else {
        setExtent();
    }
}

void HDF5DataSetHandler::createOrOpenTensorizedDataSet(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index)
{
    this->tensorized_path = std::string(dataset_name);
    dataset_rank = dim + AOSRank;
    this->AOSRank = AOSRank;
    this->timed_AOS_index = timed_AOS_index;
	if (dim > 0)
		slices_extension = size[dim - 1];

	if (!create) {
		if (*dataset_id == -1)
				*dataset_id = H5Dopen2(loc_id, dataset_name, H5P_DEFAULT);
			if (*dataset_id < 0) {
				char error_message[200];
				sprintf(error_message, "Unable to open HDF5 dataset: %s\n", dataset_name);
				throw UALBackendException(error_message, LOG);
			}
			this->dataset_id = *dataset_id;
			if (dataspace_id != -1)
				H5Sclose(dataspace_id);
			dataspace_id = H5Dget_space(*dataset_id);
			herr_t status = H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
			if (status < 0) {
				char error_message[200];
				sprintf(error_message, "Unable to call H5Sget_simple_extent_dims for: %s\n", dataset_name);
				throw UALBackendException(error_message, LOG);
        }
    }

    if (datatype != ualconst::char_data) {
        for (int i = 0; i < AOSRank; i++) {
            dims[i] = (hsize_t) AOSSize[i];
            largest_dims[i] = dims[i];
            maxdims[i] = H5S_UNLIMITED;

        }
        for (int i = 0; i < dim; i++) {
            dims[i + AOSRank] = (hsize_t) size[i];
            largest_dims[i + AOSRank] = dims[i + AOSRank];
            maxdims[i + AOSRank] = H5S_UNLIMITED;
        }
    } else {
        if (dim == 1) {
            dataset_rank = AOSRank;     //we take a HDF5 space of rank 0 for dim=1, so it gives a rank equals to AOSRank for the tensorized dataset
            for (int i = 0; i < AOSRank; i++) {
                dims[i] = (hsize_t) AOSSize[i];
                largest_dims[i] = dims[i];
                maxdims[i] = H5S_UNLIMITED;

            }
        } else {
            //can be only dim = 2 for IMAS with char type (we take a HDF5 space of rank 1, so it gives AOSRank + 1 for the rank of this tensorized dataset)
            dataset_rank = AOSRank + 1;
            for (int i = 0; i < AOSRank; i++) {
                dims[i] = (hsize_t) AOSSize[i];
                largest_dims[i] = dims[i];
                maxdims[i] = H5S_UNLIMITED;

            }
            maxdims[AOSRank] = H5S_UNLIMITED;
            dims[AOSRank] = (hsize_t) size[0];
            largest_dims[AOSRank] = dims[AOSRank];
        }
    }

    if (create) {
        if (dataspace_id != -1) {
            H5Sclose(dataspace_id);
        }
        dataspace_id = H5Screate_simple(dataset_rank, dims, maxdims);
    } 
    size_t type_size;

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
                sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", dataset_name);
                throw UALBackendException(error_message);
            }
            break;
        }

    default:
        throw UALBackendException("Data type not supported", LOG);
    }

    if (create) {


        float M = 200 * 1024 * 1024;
        size_t vmax = (size_t) floor(M / type_size);
        size_t vp = 1;
        size_t vn = 1;
        float chunk_dims_fraction_factor = 0.1;
        hsize_t chunk_dims_min[H5S_MAX_RANK];

        for (int i = 0; i < AOSRank; i++) {
            //chunk_dims[i] = dims[i];
            chunk_dims[i] = std::max(1, (int) floor(dims[i] * chunk_dims_fraction_factor));
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
                            //chunk_dims[i] /= std::max(1, round(std::max(1, (int) pow(2.0, (dataset_rank - AOSRank)))));
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
		size_t cs = vp*vn;
        hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(dcpl_id, dataset_rank, chunk_dims);
        //H5Pset_deflate (dcpl_id, 1);

        if (!shape_dataset) {
            if (datatype == ualconst::integer_data) {
                int default_value = 999999999;
                H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
            } else if (datatype == ualconst::double_data) {
                double default_value = -9.0E40;
                H5Pset_fill_value(dcpl_id, dtype_id, &default_value);
            }
        }
		
		size_t rdcc_nbytes = 500*1024*1024; //0.5GB
		size_t chunk_size = cs*type_size;
		if (chunk_size > rdcc_nbytes)
			rdcc_nbytes = 3*chunk_size;
        size_t rdcc_nslots = (size_t) 100*((float) rdcc_nbytes/ (float) chunk_size);
		if (rdcc_nslots > 300)
			rdcc_nslots = 300;

        hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
        H5Pset_chunk_cache(dapl, rdcc_nslots, rdcc_nbytes, H5D_CHUNK_CACHE_W0_DEFAULT);

        *dataset_id = H5Dcreate2(loc_id, dataset_name, dtype_id, dataspace_id, H5P_DEFAULT, dcpl_id, dapl);
        H5Pclose(dapl);
        H5Pclose(dcpl_id);

        if (*dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create HDF5 dataset: %s\n", dataset_name);
            throw UALBackendException(error_message);
        }
        this->dataset_id = *dataset_id;

    } else {
        setExtent();
    }


}

void HDF5DataSetHandler::extendTensorizedDataSet(int datatype, int dim, int *size, hid_t loc_id, hid_t dataset_id, int AOSRank, int *AOSSize)
{
    bool setExtentRequest = false;      //control if H5Dset_extent will be called for this dataset

    for (int i = 0; i < AOSRank; i++) {
        if ((hsize_t) AOSSize[i] != dims[i]) {
            dims[i] = (hsize_t) AOSSize[i];     //however, if it is a "compression", we allow it by changing the persistent shape to the new value

            if (dims[i] > largest_dims[i]) {    //we currently allow only an "expansion" of the dataset in the file
                setExtentRequest = true;
                largest_dims[i] = dims[i];
            }
        }
    }


    if (datatype != ualconst::char_data) {

        for (int i = 0; i < dim; i++) {
            if ((hsize_t) size[i] != dims[i + AOSRank]) {
                dims[i + AOSRank] = (hsize_t) size[i];
                if (dims[i + AOSRank] > largest_dims[i + AOSRank]) {
                    setExtentRequest = true;
                    largest_dims[i + AOSRank] = dims[i + AOSRank];
                }

            }
        }

    } else {
        if (dim == 1) {

        } else {
            //can be only dim = 2 for IMAS with char type (we take a HDF5 space of rank 1, so it gives AOSRank + 1 for the rank of this tensorized dataset)
            if ((hsize_t) size[0] != dims[AOSRank]) {
                dims[AOSRank] = (hsize_t) size[0];
                if (dims[AOSRank] > largest_dims[AOSRank]) {
                    setExtentRequest = true;
                    largest_dims[AOSRank] = dims[AOSRank];
                }

            }
        }
    }


    if (setExtentRequest) {
        setExtent();
    }
}

void HDF5DataSetHandler::setExtent()
{
    //std::cout << "calling setExtent !!!! " << std::endl;
    herr_t status = (int) H5Dset_extent(dataset_id, dims);
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
