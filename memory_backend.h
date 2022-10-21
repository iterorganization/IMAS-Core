#ifndef MEMORY_BACKEND_H
#define MEMORY_BACKEND_H 1

#include <string.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>

#include "ual_backend.h"
#include "ual_context.h"
#include "ual_defs.h"
#include "ual_const.h"

#define NODENAME_MANGLING  //Use IMAS mangling

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

//Global lock management (used only when opening databases)
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static void lock() {pthread_mutex_lock(&mutex);}
static void unlock() {pthread_mutex_unlock(&mutex);}


//Support classes for memory mapping

class LIBRARY_API UalData
{
    bool timed;
    int type;
    int mapState;
    //If timed, dimensionsV refers to a single slice
    std::vector<int> dimensionV;
    //If non timed bufV contains only one element
    std::vector <std::shared_ptr<unsigned char>> bufV;  
    std::string timebase;
    int getItemSize(int inType);


public:
    enum MAPPING { UNMAPPED = 1, MAPPED = 2, SLICE_MAPPED = 3 };
    UalData();
    bool isTimed() { return timed;}
    int getMapState() {return mapState;}

    std::vector<double> getDoubleVect();
    double getDouble();
    std::vector<int> getIntVect();
    double getInt();
    std::vector<unsigned char *> getCharVect();
    unsigned char *getChar();
    std::string getTimebase() { return timebase;}
    void setTimebase(std::string timebase);
    void deleteData();
    void writeData(int type, int numDims, int *dims, unsigned char *buf, std::string timebase);

//Called only when in state SLICE_MAPPED, that is, only when it contains only the most recent slices
    void prependData(int type, int numDims, int *dims, unsigned char *buf);
    void addSlice(int type, int numDims, int *dims, unsigned char *buf);
    void addSlice(UalData &slice);

    bool isEmpty() { return bufV.size() == 0;}
    void shrinkDimension() //TEMPORARY , MAY BE REMOVED LATER
    {
	if(dimensionV.size() > 0)
	    dimensionV.resize(dimensionV.size() - 1);
    }

    int readData(void **retDataPtr, int *datatype, int *retNumDims, int *retDims);
    int readSlice(int sliceIdx, void **retDataPtr, int *datatype, int *retNumDims, int *retDims);
    void readTimeSlice(double *times, int numTimes, double time, void **retDataPtr, int *datatype, int *retNumDims, int *retDims, int interpolation);
    UalData *clone();
    bool isCompatible(UalData *ualData)
    {
	if(type != ualData->type)
	    return  false;
	if(dimensionV.size() != ualData->dimensionV.size())
	    return false;
	for(size_t i = 0; i < dimensionV.size(); i++)
	{
	    if(dimensionV[i] != ualData->dimensionV[i])
	    	return false;
	}
	return true;
    }

//Interpolation method valid only for timed AoS and therefore where dimensionV.size() == 1 
    UalData *linearInterpol(UalData *ualData, double t, double t1, double t2)
    {
	UalData *retData = new UalData();
	if(bufV.size() != 1)
	{
	    std::cout << "INTERNAL ERROR: unexpected number of slices > 1 when interpolating AoS\n";
	    return retData;
	}
	double delta = (t - t1)/(t2 - t1);
	int numItems = 1;
	retData->type = type;
	retData->timed = timed;
	retData->mapState = mapState;
	retData->timebase = timebase;

	for (size_t i = 0; i < dimensionV.size(); i++)
	{
	    retData->dimensionV.push_back(dimensionV[i]);
	    numItems *= dimensionV[i];
	}
	switch (type)  {
	    case CHAR_DATA:
	    {
		char *data1 = (char *)bufV[0].get();
		char *data2 = (char *)ualData->bufV[0].get();
	    	char *currBuf = new char[numItems];
		for(int i = 0; i < numItems; i++)
		    currBuf[i] = ((char *)data1)[i] + delta * (((char *)data2)[i] - ((char *)data1)[i]);
		std::shared_ptr<unsigned char>sp((unsigned char *)currBuf);
		retData->bufV.push_back(sp);
		break;
	    }
	    case INTEGER_DATA:
	    {
		int *data1 = (int *)bufV[0].get();
		int *data2 = (int *)ualData->bufV[0].get();
	    	int *currBuf = new int[numItems];
		for(int i = 0; i < numItems; i++)
		    currBuf[i] = ((int *)data1)[i] + delta * (((int *)data2)[i] - ((int *)data1)[i]);
		std::shared_ptr<unsigned char>sp((unsigned char *)currBuf);
		retData->bufV.push_back(sp);
		break;
	    }
	    case DOUBLE_DATA:
	    {
		double *data1 = (double *)bufV[0].get();
		double *data2 = (double *)ualData->bufV[0].get();
	    	double *currBuf = new double[numItems];
		for(int i = 0; i < numItems; i++)
		    currBuf[i] = ((double *)data1)[i] + delta * (((double *)data2)[i] - ((double *)data1)[i]);
		std::shared_ptr<unsigned char>sp((unsigned char *)currBuf);
		retData->bufV.push_back(sp);
		break;
	    }
	    case COMPLEX_DATA:
	    {
		double *data1 = (double *)bufV[0].get();
		double *data2 = (double *)ualData->bufV[0].get();
	    	double *currBuf = new double[2*numItems];
		for(int i = 0; i < 2*numItems; i++)
		    currBuf[i] = ((double *)data1)[i] + delta * (((double *)data2)[i] - ((double *)data1)[i]);
		std::shared_ptr<unsigned char>sp((unsigned char *)currBuf);
		retData->bufV.push_back(sp);
		break;
	    }
	}
	return retData;
    }
    std::string toString()
    {
	if(mapState != MAPPED)
	    return "NOT MAPPED";
	int numItems = 1;
	for (size_t i = 0; i < dimensionV.size(); i++)
	    numItems *= dimensionV[i];
	std::string retStr = "";
	if(bufV.size() * numItems > 1)
	    retStr += "[";
	for(size_t i = 0; i < bufV.size(); i++)
	{
	    switch (type)  {
	    	case CHAR_DATA:
		{
		    char *data = (char *)bufV[i].get();
		    for(int j = 0; j < numItems; j++)
			retStr += data[j];
		    break;
		}
	    	case INTEGER_DATA:
		{
		    int *data = (int *)bufV[i].get();
		    for(int j = 0; j < numItems; j++)
		    {
			retStr += std::to_string(data[j]);
			if(j < numItems - 1)
			    retStr += ',';
		    }
		    break;
		}
	    	case DOUBLE_DATA:
		{
		    double *data = (double *)bufV[i].get();
		    for(int j = 0; j < numItems; j++)
		    {
			retStr += std::to_string(data[j]);
			if(j < numItems - 1)
			    retStr += ',';
		    }
		    break;
		}
	    	case COMPLEX_DATA:
		{
		    double *data = (double *)bufV[i].get();
		    for(int j = 0; j < numItems; j++)
		    {
			retStr += "(";
			retStr += std::to_string(data[2*j]);
			retStr += ",";
			retStr += std::to_string(data[2*j+1]);
			retStr += ")";
			if(j < numItems - 1)
			    retStr += ',';
		    }
		    break;
		}
	    }
	}
	if(bufV.size() * numItems > 1)
	    retStr += "]";
	return retStr;
    }
};

class UalStruct;
class LIBRARY_API UalAoS
{
public:
    std::string timebase;
    std::vector<UalStruct *> aos;
    void setTimebase(std::string timebase) {this->timebase = timebase;}
    void addSlice(UalAoS &sliceAos, ArraystructContext *ctx);
    void deleteData();
    UalAoS *clone();
    void dump(int tabs);
    ~UalAoS();
    UalAoS * linearInterpol(UalAoS *ualAos, double t, double t1, double t2); 
};

class LIBRARY_API UalStruct
{
	// Because LIBRARY_API required to explicitly delete the copy constructor (template std)
	UalStruct(const UalStruct&) = delete;
    UalStruct& operator=(const UalStruct&) = delete;
	
public:
     std::unordered_map<std::string, UalData *> dataFields;
    std::unordered_map<std::string, UalAoS *>aosFields;

    UalData *getData(std::string path);
 //   void setData(std::string path, UalData &data);
    void deleteData();
    UalAoS *getSubAoS(std::string path);
    void addSlice(UalStruct &ualSlice, ArraystructContext *ctx);
    bool isAoSMapped(std::string path);
    UalStruct *clone();
    UalStruct()
	{
	}
    ~UalStruct()
    {
  	for ( auto it = dataFields.cbegin(); it != dataFields.cend(); ++it )
	{
	    delete it->second;
	}
  	for ( auto it = aosFields.cbegin(); it != aosFields.cend(); ++it )
	{
	    delete it->second;
	}
    }

    void dump(int tabs = 0)
    {
  	for ( auto it = dataFields.cbegin(); it != dataFields.cend(); ++it )
	{
	    for(int i = 0; i < tabs; i++)
		std::cout << "  ";
	    std::cout << it->first << ": " << it->second->toString() << std::endl;
	}

  	for ( auto it = aosFields.cbegin(); it != aosFields.cend(); ++it )
	{
	    for(int i = 0; i < tabs; i++)
		std::cout << "  ";
	    std::cout << "AOS - -" << it->first << ": " << std::endl;
	    it->second->dump(tabs + 1);
	}
    }
    UalStruct *linearInterpol(UalStruct *ualStruct, double t, double t1, double t2);
	
};

class LIBRARY_API StructPath
{
public:
    UalStruct *ualStruct;
    std::string path;
    StructPath(UalStruct *ualStruct, std::string path)
    {
	this->ualStruct = ualStruct;
	this->path = path;
    }
};


class LIBRARY_API IdsInfo
{
public:
    std::string idsPath;
    UalStruct *ids;
    IdsInfo(std::string idsPath, UalStruct *ids)
    {
	this->idsPath = idsPath;
	this->ids = ids;
    }
};


class InternalCtx  
{
public:
    std::unordered_map<std::string, UalStruct *> idsMap;
    std::unordered_map<unsigned long int, IdsInfo *> idsInfoMap;
    pthread_mutex_t mutex;
    int refCount;
    std::string fullName;
    InternalCtx()
    {
	refCount = 1;
//Mutex must be recursive as readData can be called recursively
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &attr);
    }
    ~InternalCtx()
    {
	pthread_mutex_destroy(&mutex);
    }
    void lock()
    {
	pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
	pthread_mutex_lock(&mutex);
    }
   
};



//Hash Context, addressed by exp name+shot+run. Same behavior as pulse files: new: creates a new one, open: open an existing memory content, if any report error otherwise. 
    static std::unordered_map<std::string, InternalCtx * > ctxMap;




class LIBRARY_API MemoryBackend:public Backend 
{
	// Because LIBRARY_API required to explicitly delete the copy constructor (template std)
	MemoryBackend(const MemoryBackend&) = delete;
    MemoryBackend& operator=(const MemoryBackend&) = delete;

    bool isCreated;
    InternalCtx *internalCtx;
    //std::unordered_map<std::string, UalStruct *> idsMap;
    //std::unordered_map<unsigned long int, IdsInfo *> idsInfoMap;

//currentAoS will containg the fields being written when assembing a new AoS (or AoS slice)
    UalAoS currentAos;


    //Get the full pathname (internal) of the IDS root 
    std::string getIdsPath(OperationContext *ctx);

    //Get the IDS (in UalStruct) 
    UalStruct *getIds(OperationContext *ctx);

    //Get the  AoS referred to the passed ArrayStructContext. If isCurrent, then the currentAoS is considered, otherwise the corresponding AoS in the main IDS UalStruct is condiered.
    UalAoS *getAoS(ArraystructContext *ctx, bool isCurrent = false);

    UalData *getData(ArraystructContext *ctx, int idx, std::string path, bool isCurrent);


//Optimization info
    std::string lastIdsPath; 



//Check if the passed context refers to an AoS that is mapped in memory (i.e. for which deleteData or putData has been issued)
    bool isMappedAoS(ArraystructContext *ctx);
	
public:
  // virtual desctructor
    MemoryBackend() 
    {
	internalCtx = NULL;
    }
    ~MemoryBackend()
    {
// Gabriele Sept 2020 Deallocate and remove from ctxMap the InternalCtx instance if refCount reaches 0;
	lock();
	internalCtx->refCount--;
	if(internalCtx->refCount <= 0)
	{
  	//for ( auto it = idsMap.cbegin(); it != idsMap.cend(); ++it )
  	    for ( auto it = internalCtx->idsMap.cbegin(); it != internalCtx->idsMap.cend(); ++it )
	    {
	    	delete it->second;
	    }
	}
	ctxMap.erase(internalCtx->fullName);
	delete internalCtx;
	unlock();
    } 
    void dump(std::string ids)
    {
//	idsMap[ids]->dump();
        internalCtx->lock();
	internalCtx->idsMap[ids]->dump();
	internalCtx->unlock();
    }



  /**
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @throw BackendException
  */
    virtual void openPulse(DataEntryContext *ctx,
			 int mode)
    {
	isCreated = (mode == ualconst::create_pulse || mode == ualconst::force_create_pulse);

    std::string fullName = ctx->getURI().query.get("path").value();
	
	lock();  //Global Lock
	try {
	    internalCtx = ctxMap.at(fullName);
	    if(mode == ualconst::create_pulse)
	    {
		unlock();
		throw  UALBackendException("CreatePulse: a pulse file already exists",LOG);
	    }
	    internalCtx->refCount++;
	} catch (const std::out_of_range& oor) 
	{
	    if(mode == ualconst::create_pulse || mode == ualconst::force_create_pulse || mode == ualconst::force_open_pulse)
	    {
		internalCtx = new InternalCtx;
		internalCtx->fullName = fullName;
		ctxMap[fullName] = internalCtx;
	    }
            else
	    {
		unlock();
		throw  UALBackendException("Missing pulse",LOG);
	    }
	}
	if (mode == ualconst::force_create_pulse)  //Empty previous content, if any
	{
   	    internalCtx->lock();

	    for ( auto it = internalCtx->idsMap.cbegin(); it != internalCtx->idsMap.cend(); ++it )
	    {
		delete it->second;
	    }  
	    internalCtx->idsMap.clear();
	    internalCtx->unlock();
	}
	unlock();
    }

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse
     @throw BackendException
  */
    virtual void closePulse(DataEntryContext *ctx,
			  int mode)

    {
    }

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
    
   virtual int readData(OperationContext *ctx,
			  std::string fieldname,
			  std::string timebase,
			  void** data,
			  int* datatype,
			  int* dim,
			  int* size);
    virtual int readData(Context *ctx,
			  std::string fieldname,
			  std::string timebase,
			  void** data,
			  int* datatype,
			  int* dim,
			  int* size)
    {
	if(ctx->getType() == CTX_ARRAYSTRUCT_TYPE)
	    return getFromArraystruct((ArraystructContext *)ctx, fieldname, timebase, 
				    ((ArraystructContext *)ctx)->getIndex(), data, datatype, dim, size);
	else
    	    return readData((OperationContext *)ctx, fieldname, timebase, data, datatype, dim, size);
    }
  /*
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx pointer on operation context 
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @throw BackendException
  **/
  virtual void deleteData(OperationContext *ctx,
			  std::string path);

  /**
     Starts writing a new array of structures.
     This function initiates the writing of a new top-level or nested array of structure.
     @param[in] ctx pointer on array of structure context
     @param[in] size specify the size of the array (number of elements)
     @throw BackendException
  */
  virtual void beginWriteArraystruct(ArraystructContext *ctx,
				     int size);

  /**
     Starts reading a new array of structures.
     This function initiates the reading of a new top-level or nested array of structure.
     @param[in] ctx pointer on array of structure context
     @param[out] size specify the size of the array (number of elements)
     @throw BackendException
  */
  virtual void beginReadArraystruct(ArraystructContext *ctx,
				    int* size);

  /**
     Writes data in a struct_array.
     This function stores data in an element of an array of structures.
     @param[in] ctx pointer on array of structure context
     @param[in] fieldname field name (relative to array's root)
     @param[in] idx index of targeted element in the array
     @param[in] data pointer on the data to be stored
     @param[in] datatype type of data to be stored:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (NULL is dim=0)
     @throw BackendException
  */
  virtual void putInArraystruct(ArraystructContext *ctx,
				std::string fieldname,
				std::string timebase,  //Gabriele 2017: Added to handle time dependent signals as firlds of static AoS
				int idx,
				void* data,
				int datatype,
				int dim,
				int* size);

  /**
     Reads data from a struct_array.
     This function extract data from an array of structure.
     @param[in] ctx pointer on array of structure context
     @param[in] fieldname field name (relative to array's root)
     @param[in] idx index of targeted element in the array
     @param[out] data returned pointer on the read data 
     @param[out] datatype returned type of read data:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[out] dim returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[out] size returned array of the size of each dimension (NULL is dim=0)
     @throw BackendException
  */
    virtual int getFromArraystruct(ArraystructContext *ctx,
				  std::string fieldname,
				  std::string timebase,
				  int idx,
				  void** data,
				  int* datatype,
				  int* dim,
				  int* size);
    virtual void beginArraystructAction(ArraystructContext *ctx, int *size)
    {
        if(ctx->getOperationContext()->getAccessmode() == READ_OP)
	   beginReadArraystruct(ctx, size);
	else
	   beginWriteArraystruct(ctx, *size);
    }
  	
    virtual void endAction(Context *ctx); 
		
    virtual void beginAction(OperationContext *ctx);

    std::pair<int,int> getVersion(DataEntryContext *ctx) override;


    void flush(DataEntryContext *ctx, std::string dataobjectName);
    void flushAoS(OperationContext *ctx, std::string fieldName, UalAoS &ualAos);
    void recFlushAoS(UalAoS &ualAoS, OperationContext *opCtx, ArraystructContext *ctx);
    int getFromAoS(ArraystructContext *ctx,
					std::string fieldname,
					int idx,
					void** data,
					int* datatype,
					int* dim,
					int* size);
    int getSliceFromAoS(ArraystructContext *ctx,
					std::string fieldname,
					int idx,
					void** data,
					int* datatype,
					int* dim,
					int* size);
	
    void prepareSlice(ArraystructContext *ctx);
    UalStruct *prepareSliceRec(ArraystructContext *ctx, UalStruct &ualStruct, UalStruct &ids, double time, std::vector<StructPath> &parentV, UalAoS *aos);
	std::vector<double> getTimebaseVect(std::string path, std::vector<StructPath> &ctxV, UalAoS *aos = NULL);
    void getSliceIdxs(std::string path, double time, std::vector<StructPath> &ctxV, int &sliceIdx1, int &sliceIdx2, UalAoS *aos = NULL);
    UalData *getUalSlice(ArraystructContext *ctx, UalData &inData, double time);
    UalData *getUalSlice(ArraystructContext *ctx, UalData &inData, double time, std::vector<double> timebaseV);
};

#endif

#endif // MEMORY_BACKEND_H
