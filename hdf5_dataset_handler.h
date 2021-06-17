#ifndef HDF5_DATASETHANDLER_H
#define HDF5_DATASETHANDLER_H 1

#include <hdf5.h>
#include "ual_backend.h"

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#define UINT_DATA   61

typedef struct {
   double re;   /*real part */
   double im;   /*imaginary part */
} complex_t;

class HDF5DataSetHandler {
  private:
    std::string tensorized_path;        //full tensorized path
    hid_t dataset_id;
    int dataset_rank;
    int AOSRank;
	int datatype;
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
	

  public:

     HDF5DataSetHandler();
    ~HDF5DataSetHandler();

    hid_t dtype_id;
    hid_t dataspace_id;
    hid_t IDS_group_id;
	void showDims(std::string context);
	void showAOSIndices(std::string context, std::vector<int> &AOS_indices);
	void showAOSShapes(std::string context, std::vector<int> &AOS_shapes);

	void create(const char *dataset_name, hid_t * dataset_id, int datatype, hid_t loc_id, int dim, int *size, int AOSRank, int *AOSSize, bool shape_dataset, bool compression_enabled);
	void open(const char *dataset_name, hid_t loc_id, hid_t * dataset_id, int dim, int *size, int datatype, bool shape_dataset);
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
    void getAttributes(bool * isTimed, int *timed_AOS_index) const;
	int getSlicesExtension() const;
	bool isShapeDataset() const;
     
    int getRank() const;
    hsize_t * getDims();
    int getInitialOffset() const;
	int getSize() const;
	int getShape(int axis) const;
    hid_t getDataSpace();
    void setNonSliceMode();
    void setSliceMode(Context * ctx);
    int getTimedShape(int *timed_AOS_index_);
    void setTensorizedPath(std::string p) {
        tensorized_path = p;
}};

#endif
