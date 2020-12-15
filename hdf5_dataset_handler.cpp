#include "hdf5_dataset_handler.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include <math.h>


HDF5DataSetHandler::HDF5DataSetHandler():dataset_id(-1), dataset_rank(-1), AOSRank(0), immutable(true), slice_mode(false), slice_index(-1), timed_AOS_index(-1), isTimed(false), dataset_already_extended_by_slicing(false), hasBeenExtended(false), dataSetExtended(false), dtype_id(-1), dataspace_id(-1)
{
    resetExtensionState();
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5DataSetHandler::~HDF5DataSetHandler()
{
    if (!immutable)
        H5Tclose(dtype_id);
    if (dataset_id != -1)
        H5Dclose(dataset_id);
    if (dataspace_id != -1) {
        H5Sclose(dataspace_id);
    }
}

void
 HDF5DataSetHandler::setSliceMode(Context * ctx, hid_t loc_id, int homogeneous_time)
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

std::string HDF5DataSetHandler::getName() const
{
    return this->dataset_name;
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


int HDF5DataSetHandler::getTimedShape(int *timed_AOS_index_)
{
    int timed_current_shape = -1;
    if (timed_AOS_index != -1) {
        *timed_AOS_index_ = timed_AOS_index;
        timed_current_shape = dims[timed_AOS_index];
    }

    return timed_current_shape;
}

void HDF5DataSetHandler::updateAOSShapesTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, int shapes_slice_index)
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

void HDF5DataSetHandler::updateTensorizedDataSet(Context * ctx, const std::string & dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSShapes, std::vector < int >&current_arrctx_indices, std::set < hid_t > &dynamic_aos_already_extended_by_slicing)
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


    bool dynamic_aos_already_extended = false;

    if (slice_mode) {
        dynamic_aos_already_extended = dynamic_aos_already_extended_by_slicing.find(*dataset_id) != dynamic_aos_already_extended_by_slicing.end();
    }

    if (slice_mode && (!dataset_already_extended_by_slicing || !dynamic_aos_already_extended)) {
        if (!dataset_already_extended_by_slicing && timed_AOS_index == -1) {
            //std::cout << "increasing dimension of latest dimension to: " << size[dim -1] << std::endl;          
            size[dim - 1]++;    //slicing is outside a dynamic AOS

        } else if (!dynamic_aos_already_extended && timed_AOS_index != -1 && timed_AOS_index < AOSRank) {       //slicing is inside a dynamic AOS
            AOSShapes[timed_AOS_index] = slice_index + 1;
            dynamic_aos_already_extended_by_slicing.insert(*dataset_id);
        }

        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSShapes);
        dataset_already_extended_by_slicing = true;
    } else {
        extendTensorizedDataSet(datatype, dim, size, loc_id, *dataset_id, AOSRank, AOSSize_);
    }
}

void HDF5DataSetHandler::createOrOpenTensorizedDataSet(const char *dataset_name, int datatype, int dim, int *size, hid_t loc_id, hid_t * dataset_id, int AOSRank, int *AOSSize, bool create, bool shape_dataset, int timed_AOS_index)
{
    this->dataset_name = std::string(dataset_name);
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


        size_t M = (size_t) 2 * 1024 * 1024;
        size_t vmax = (size_t) floor(M / type_size);

        //std::cout << "vmax = "  << vmax << std::endl;

        size_t vp = 1;
        size_t vn = 1;
        float chunk_dims_fraction_factor = 0.01;
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
                        chunk_dims[i] /= max(1, round(max(1, pow(2.0, AOSRank))));
                        if (chunk_dims[i] <= 0 || chunk_dims[i] <= chunk_dims_min[i]) {
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
                            chunk_dims[i] /= max(1, round(max(1, pow(2.0, (dataset_rank - AOSRank)))));
                            if (chunk_dims[i] <= 0 || chunk_dims[i] <= chunk_dims_min[i]) {
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

        /*std::cout << ">>>------------------<<<" << std::endl;

           std::cout << "cs = " << vp*vn << std::endl;

           for (int i = 0; i < AOSRank; i++)
           {
           std::cout << "chunk_dims_AOS[" << i << "]=" << chunk_dims[i] << std::endl;

           }
           for (int i = AOSRank; i < dataset_rank; i++)
           {
           std::cout << "chunk_dims[" << i << "]=" << chunk_dims[i] << std::endl;

           } */


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

        hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
        H5Pset_chunk_cache(dapl, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, 3 * M, H5D_CHUNK_CACHE_W0_DEFAULT);

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

            hasBeenExtended = true;
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

                hasBeenExtended = true;
                dataSetExtended = true;
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

                hasBeenExtended = true;
                dataSetExtended = true;
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

void HDF5DataSetHandler::resetExtensionState()
{
    hasBeenExtended = false;
    dataSetExtended = false;
}


/**
* Find maximum between two numbers.
*/
int HDF5DataSetHandler::max(int num1, int num2)
{
    return (num1 > num2) ? num1 : num2;
}
