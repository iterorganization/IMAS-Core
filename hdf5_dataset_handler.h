#ifndef HDF5_DATASETHANDLER_H
#define HDF5_DATASETHANDLER_H 1

#include <hdf5.h>
#include "ual_backend.h"
#include "hdf5_hs_selection_reader.h"

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#define UINT_DATA   61

#define READ_CHUNK_CACHE_SIZE 5*1024*1024
#define WRITE_CHUNK_CACHE_SIZE 100*1024*1024

typedef struct {
   double re;   /*real part */
   double im;   /*imaginary part */
} complex_t;

class HDF5DataSetHandler {
  private:

    std::string tensorized_path;        //full tensorized path

    int dataset_rank;
    int AOSRank;
    bool immutable;
	bool shape_dataset;

    bool slice_mode;
	int slices_extension;
    int timed_AOS_index;
    bool isTimed;
    int timedAOS_shape;
    int timeWriteOffset;

    hsize_t dims[H5S_MAX_RANK];
    hsize_t maxdims[H5S_MAX_RANK];
    hsize_t largest_dims[H5S_MAX_RANK];
    hsize_t chunk_dims[H5S_MAX_RANK];

    hsize_t initial_dims[H5S_MAX_RANK]; //dims stored at beginning of a put_slice

    int setType();
	void writeBuffer();
    void setBuffering(bool useBufferingOption);
    void disableBufferingIfNotSupported(int datatype, int dim);
    void appendStringToBuffer(const std::vector < int >&current_arrctx_indices, char **data);
    void appendInt0DToBuffer(const std::vector < int >&current_arrctx_indices, void *data);
    void appendIntNDToBuffer(const std::vector < int >&current_arrctx_indices, void *data, int dim);
    void appendDouble0DToBuffer(const std::vector < int >&current_arrctx_indices, void *data);
    void appendDoubleNDToBuffer(const std::vector < int >&current_arrctx_indices, void *data, int dim);

    //read operations
    void read0DStringsFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void create0DStringsBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void readInt0DFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void readIntNDFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void readDoubleNDFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void createIntBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void readDouble0DFromBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void createDoubleBuffer(HDF5HsSelectionReader & hsSelectionReader, const std::vector < int >&current_arrctx_indices, void **data);
    void readUsingHyperslabs(const std::vector < int >&current_arrctx_indices, int slice_mode, bool is_dynamic, bool isTimed, int timed_AOS_index, int slice_index, void **data, bool read_strings);

  public:

     HDF5DataSetHandler(bool writing_mode_, uri::Uri uri);
    ~HDF5DataSetHandler();

    int datatype;
    hid_t dataset_id;
    hid_t dtype_id;
    int request_dim;
    hid_t dataspace_id;
    hid_t IDS_group_id;
    void *buffer;
    int buffer_length;
    bool compression_enabled;
    bool useBuffering;
    size_t chunk_cache_size;
    size_t write_chunk_cache_size;
    std::vector <std::vector<int> > requests_arrctx_indices;
    std::vector <std::vector<int> > requests_shapes;

    std::vector<char *> data_sets_buffers;
    std::vector<int *> int_data_set_buffer;
    std::vector<double *> double_data_set_buffer;

    std::vector<char *> full_data_sets_buffers;
    int * full_int_data_set_buffer;
    double * full_double_data_set_buffer;

    std::unique_ptr < HDF5HsSelectionReader > selection_reader;

    bool operator==(const HDF5DataSetHandler &other) const;

	void showDims(std::string context);
	void showAOSIndices(std::string context, const std::vector<int> &AOS_indices);
	void showAOSShapes(std::string context, const std::vector<hsize_t> &AOS_shapes);

	void create(const char *dataset_name, hid_t * dataset_id, int datatype, hid_t loc_id, int dim, int *size, int AOSRank, int *AOSSize, bool shape_dataset, bool create_chunk_cache);
	void open(const char *dataset_name, hid_t loc_id, hid_t * dataset_id, int dim, int *size, int datatype, bool shape_dataset, bool create_chunk_cache, uri::Uri uri, int AOSRank=-1, int *AOSSize = NULL);
	void setCurrentShapesAndExtend(int *size, int *AOSShapes);
	void setCurrentShapes(int *size, int *AOSShapes);
	void setExtent();
    void storeInitialDims();
	void extendDataSpaceForTimeSlices(int *size, int *AOSShapes, int dynamic_AOS_slices_extension);
	void setTimeAxisOffset(const std::vector < int > &current_arrctx_indices, int dynamic_AOS_slices_extension);
    void updateTimeAxisOffset(const std::vector < int > &current_arrctx_indices);

	//AOS Dataset management
	void setCurrentShapesAndExtendForAOSDataSet(int *size, int *AOSShapes);
	void setCurrentShapesForAOSDataSet(int *size, int *AOSShapes);
    void extendDataSpaceForTimeSlicesForAOSDataSet(int *size, int *AOSShapes, int dynamic_AOS_slices_extension); 
	void setTimeAxisOffsetForAOSDataSet();
    void setTimedAOSShape(int timedAOS_shape);
    int getTimedAOSShape() const;
    int getTimeWriteOffset() const;

	std::string getName() const;
	int getSlicesExtension() const;
	bool isShapeDataset() const;
    int getRank() const;
    int getAOSRank() const;
    const hsize_t * getDims();
    std::vector<int> getDimsAsVector() const;
    int computeShapeFromDimsVector(std::vector<int> &v);
    hsize_t * getLargestDims();
    int getInitialOffset() const;
	int getSize() const;
    size_t getMaxShape(int axis) const;
	int getShape(int axis) const;
    hid_t getDataSpace() const;
    void setNonSliceMode();
    void setSliceMode(Context * ctx);
    int getTimedShape(int *timed_AOS_index_);
    void setTensorizedPath(std::string p) {
        tensorized_path = p;
    }
    void close();
    bool isRequestInExtent(const std::vector < int >&current_arrctx_indices);
    void write_buffers();
    void fillFullBuffers() ;

    void writeUsingHyperslabs(const std::vector < int >&current_arrctx_indices, int slice_mode, int dynamic_AOS_slices_extension, void *data);
    void appendToBuffer(const std::vector < int >&current_arrctx_indices, bool dataSetAlreadyOpened, int datatype, int dim, int slice_mode, int dynamic_AOS_slices_extension, char**p, void *data);

    //Reading operation
    void readData(const std::vector < int >&current_arrctx_indices, int datatype, int dim, int slice_mode, bool is_dynamic, bool isTimed, int timed_AOS_index, int slice_index, void **data);

};

#endif
