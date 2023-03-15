#ifndef MDSPLUS_BACKEND_H
#define MDSPLUS_BACKEND_H 1

#include <mdsobjects.h>
#include <string>
#include <unordered_map>

#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <Shlobj.h>
#else // WIN32
#include <pwd.h>
#endif // WIN32

#include "ual_context.h"
#include "ual_backend.h"
#include "ual_defs.h"
#include "ual_const.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus



class LIBRARY_API MDSplusBackend:public Backend 
{
  private:
	// Because LIBRARY_API required to explicitly delete the copy constructor (template std)
	MDSplusBackend(const MDSplusBackend&) = delete;
    MDSplusBackend& operator=(const MDSplusBackend&) = delete;

    MDSplus::Tree *tree;

    int sliceIdxs[2];
    double sliceTimes[2];

    //Keep for every top AoS the index (from 1) of the next free node to store dynamic AoS or time dependent fields
    std::unordered_map<std::string, int> timedNodeFreeIdxMap;
    std::unordered_map<std::string, int> timeaNodeFreeIdxMap;
    //Keep track of increasing idx assignment of times subAoS
    int maxAosTimedId;
    int maxAosTimeaId;

    //Keep for every timed AoS field or dynamic AoS the full path of the corresponding node 
    std::unordered_map<std::string, std::string> timedNodePathMap;
    std::unordered_map<std::string, std::string> timeaNodePathMap;

    //In ordert to increase efficiency of MDSplus find node
    std::unordered_map<std::string, MDSplus::TreeNode *> treeNodeMap;

    std::unordered_map<ArraystructContext *, MDSplus::Apd *> arrayStructCtxDataMap;
    //std::vector<ArraystructContext *>arrayStructContextV;
    //std::vector<MDSplus::Apd *>arrayStructDataV;

    MDSplus::Apd *getApdFromContext(ArraystructContext *);
    void addContextAndApd(ArraystructContext *arrStructCtx, MDSplus::Apd *arrStructData);
    void removeContextAndApd(ArraystructContext *arrStructCtx, MDSplus::Apd *arrStructData);
    
    std::string composePaths(std::string dataobjectPath, std::string path);    
    MDSplus::Data *assembleData(void *data, int datatype, int numDims, int *dims);
    MDSplus::Data *assembleStringData(void *data, int numDims, int *inDims, int expectedLen);
    void disassembleData(MDSplus::Data *data, void **retDataPtr, int *datatype, int *retNumDims, int *retDims);
    int getMdsShot(int shot, int run, bool translate, std::string strTree);
    void resetIdsPath(std::string strTree);
    void setDataEnv(DataEntryContext * ctx); 
    int getSliceNumItems(int numDims, int *dims);
    int getSliceSize(MDSplus::TreeNode *node, void *data, int datatype, int numDims, int *dims, bool isMultiple = true);
    void writeData(MDSplus::Tree *tree, std::string dataobjectPath, std::string timePath, void *dataPtr, int datatype, int numDims,
	int *dims);
    void writeTimedData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, std::string timebase, void *data, int datatype, int numDims,
	int *dims, bool isAos = false, bool isRefAos = false, bool append = false);
    void writeSlice(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, std::string timebase,  void *data, int datatype, int numDims, 
	int *dims, bool isAos = false, bool isRefAos = false);
    int readData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, void **dataPtr, int *datatype,
	int *numDims, int *dims, char *mdsPath = NULL);
    int readTimedData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, void **dataPtr, int *datatype,
	int *numDims, int *dims, char *mdsPath = NULL);
    int readTimedData(MDSplus::TreeNode *node, void **dataPtr, int *datatype, int *numDims, int *outDims);
    void deleteData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path);
    int readSlice(MDSplus::Tree *tree, bool isApd, std::string dataobjectPath, std::string path, std::string timebase, double time, int interpolation, void **data, int *datatype,
	int *numDims, int *dims, bool manglePath = true);

//Array of structures stuff	
     void writeTimedApd(MDSplus::Apd *apd, std::string dataobjectPath, std::string path, std::string timebase); 
    void writeApdSlice(MDSplus::Apd *inApd, std::string aosPath, std::string timebasePath, double time);
    MDSplus::Apd *readApd(MDSplus::Tree *tree, std::string dataobjectPath, std::string path);
    MDSplus::Apd *readDynamicApd(std::string dataobjectPath, std::string path);
    MDSplus::Apd *readDynamicLazyApd(MDSplus::TreeNode *node);
    MDSplus::Apd *readSliceApd(MDSplus::TreeNode *inNode, std::string timebase, double time, int interpolation, std::string currPath, ArraystructContext *ctx);
    MDSplus::Apd *getApdSliceAt(MDSplus::TreeNode *node, int idx);
    MDSplus::Data *getFromApd(MDSplus::Apd *apd, int idx, std::string path, ArraystructContext *ctx = NULL);
    void fillApdSlicesAroundIdx(MDSplus::Apd *apd, int sliceIdx);
    void getIndexesInTimebaseExpr(MDSplus::Data *timebase, int &idx1, int &idx2);
    bool checkStruct(MDSplus::Apd *apd1, MDSplus::Apd *apd2);
    bool checkStructRec(MDSplus::Apd *apd1, MDSplus::Apd *apd2);
    bool checkStructItem(MDSplus::Data *item1, MDSplus::Data *item2);
    MDSplus::Apd *interpolateStruct(MDSplus::Apd *apd1, MDSplus::Apd *apd2, double t, double t1, double t2, std::string currPath, ArraystructContext *ctx);
    MDSplus::Apd *interpolateStructRec(MDSplus::Apd *apd1, MDSplus::Apd *apd2, double t, double t1, double t2, std::string currPath, bool firstRec, ArraystructContext *ctx);
    MDSplus::Data *interpolateStructItem(MDSplus::Data *item1, MDSplus::Data *item2, double t, double t1, double t2, std::string currPath, ArraystructContext *ctx);
    MDSplus::Apd *resolveApdSliceFields(MDSplus::Apd *apd, double time, int interplolation, std::string timebasePath, std::string dataobjectPath);
    MDSplus::Apd *resolveApdTimedFields(MDSplus::Apd *apd);
    void resolveApdField(MDSplus::Apd *apd, ArraystructContext *ctx);
    void writeStaticApd(MDSplus::Apd *apd, std::string dataobjectPath, std::string path);
    void writeDynamicApd(MDSplus::Apd *apd, std::string aosPath, std::string timebasePath, bool append = false);
    MDSplus::Apd *readDynamicApd(MDSplus::TreeNode *node);
    void insertNewInApd(ArraystructContext *ctx,std::string rootName, MDSplus::Apd *apd, int idx, std::string path, std::string timebasePath, bool isSlice, MDSplus::Data * mdsData, void* data = NULL, int datatype = 0, int dim = 0, int* size = NULL);
    void insertInApd(ArraystructContext *ctx,MDSplus::Apd *apd, int idx, std::string path, std::string timebasePath, bool isSlice, MDSplus::Data *mdsData, void* data = NULL, int datatype = 0, int dim = 0, int* size = NULL);
    std::string getTopAoSPath(ArraystructContext *ctx);
    std::string getAoSFullPath(ArraystructContext *ctx, int idx);  
    std::string getNodePathFor(std::string aosPath, std::string aosFullPath, std::string aosName, bool isAos);
    std::string getTimedNode(ArraystructContext *ctx, std::string field, int idx, bool isAos);
    std::string getTimedNode(ArraystructContext *ctx, std::string fullAosPath, bool isAos);
    std::string getTopAoSName(ArraystructContext *ctx);
    std::string  toLower(std::string s);
     std::string relativeToAbsolutePath(ArraystructContext *ctx, std::string relPath, int idx = 0);
    void resetNodePath();
    MDSplus::TreeNode *getNode(const char *, bool makeNew = false);
    void freeNodes();

    int getTimebaseIdx(std::string timebase, std::string dataobjectPath, double time);
    int getSegmentIdx(MDSplus::TreeNode *node, int timebaseIdx);
    double *getSegmentIdxAndDim(MDSplus::TreeNode *node, std::string dataobjectPath, std::string timebase, double time, int &segIdx, int &nDim);


/////Public section - Implementation of Backend interface		
      public:
	
    MDSplusBackend()
    {
	tree = NULL;
	sliceIdxs[0] = sliceIdxs[1] = -1;
	sliceTimes[0] = sliceTimes[1] = 0;
    }	
    virtual ~MDSplusBackend(){freeNodes();}	

    static Backend* initBackend(int id);

    virtual void openPulse(DataEntryContext *ctx,
			   int mode);
      
    virtual void closePulse(DataEntryContext *ctx,
			    int mode);

    virtual void beginAction(OperationContext *ctx);

    virtual void endAction(Context *ctx);

    virtual void writeData(OperationContext *ctx,
			   std::string fieldname,
			   std::string timebase,
			   void* data,
			   int datatype,
			   int dim,
			   int* size);
    virtual void writeData(Context *ctx,
			   std::string fieldname,
			   std::string timebase,
			   void* data,
			   int datatype,
			   int dim,
			   int* size)
    {
	if(ctx->getType() == CTX_ARRAYSTRUCT_TYPE)
	    putInArraystruct((ArraystructContext *)ctx,fieldname,timebase, //Relative to the current AoS root. Empty for non time dependent fields
			((ArraystructContext *)ctx)->getIndex(), data, datatype, dim, size);
	else
	    writeData((OperationContext *)ctx, fieldname, timebase, data, datatype, dim, size);
    }
    
    virtual int readData(Context *ctx,
			  std::string fieldname,
			  std::string timebase,
			  void** data,
			  int* datatype,
			  int* dim,
			  int* size)
    {
	if(ctx->getType() == CTX_ARRAYSTRUCT_TYPE)
	  return getFromArraystruct((ArraystructContext *)ctx, fieldname,
			     ((ArraystructContext *)ctx)->getIndex(), data, datatype, dim, size);
	else
	  return readData((OperationContext *)ctx, fieldname, timebase, data, datatype, dim, size);
    }

    virtual int readData(OperationContext *ctx,
			std::string fieldname,
			std::string timebase,
			void** data,
			int* datatype,
			int* dim,
			int* size);  

    virtual void deleteData(OperationContext *ctx,
			    std::string fieldname);
			
			
    virtual void beginWriteArraystruct(ArraystructContext *ctx,
				       int size);

    virtual void beginReadArraystruct(ArraystructContext *ctx,
				      int* size);

    virtual void putInArraystruct(ArraystructContext *ctx,
				  std::string fieldname,
				  std::string timebasename,  //Relative to the current AoS root. Empty for non time dependent fields
				  int idx,
				  void* data,
				  int datatype,
				  int dim,
				  int* size);

    virtual int getFromArraystruct(ArraystructContext *ctx,
				    std::string fieldname,
				    int idx,
				    void** data,
				    int* datatype,
				    int* dim,
				    int* size);


    virtual void beginArraystructAction(ArraystructContext *ctx,
				      int *size)
    {
        if(ctx->getOperationContext()->getAccessmode() == READ_OP)
	   beginReadArraystruct(ctx, size);
	else
	   beginWriteArraystruct(ctx, *size);
    }
 /**
     Returns version of the backend (pair <major,minor>), to be used for compatibility checks.
     Version number needs to be bumped when:
     - non-backward compatible changes are being introduced --> major
     - non-forward compatible changes are being introduced --> minor
     @param[in] ctx pointer on pulse context, if NULL then returns the version of the backend,
     otherwise returns the version stored in associated pulse file
     @result std::pair<int,int> for <major,minor> version numbers
  */
    virtual std::pair<int,int> getVersion(DataEntryContext *ctx);

//Timebase cache
    double *getCachedTimebase(std::string timebasePath, int &nSamples);

    void updateCachedTimebase(std::string timebasePath, double *timebase, int nSamples);
    std::unordered_map<std::string, std::vector<double>> timebaseMap;
    std::unordered_map<int, std::string> timebasePathMap;


    class SegmentDescriptor
    {
	public:
	    int segmentIdx, startIdx, endIdx;
	    SegmentDescriptor(int segmentIdx, int startIdx, int endIdx)
	    {
		this->segmentIdx = segmentIdx;
		this->startIdx = startIdx;
		this->endIdx = endIdx;
	    }
    };
    std::unordered_map<int, std::vector<SegmentDescriptor>> segmentIdxMap;
    void getSegmentIdxFromSliceIdx(MDSplus::TreeNode *node, int sliceIdx, int &retSegmentIdx, int &retStartIdx, int &retEndIdx);
    //Temporary
    void fullResetNodePath();

    static void printFileVersionInfo(int shot, int run, std::string usr, std::string tok, std::string ver);

};

#endif

#endif // MDSPLUS_BACKEND_H
