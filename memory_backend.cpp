#include "memory_backend.h"

#define MAX_DIM 64


    void UalAoS::addSlice(UalAoS &sliceAos, ArraystructContext *ctx)
    {
//	if(ctx->getTimebasePath().length() > 0)
	if(timebase != "")
	{
	    if(sliceAos.aos.size() !=1 )
	    {
	    	std::cout << "INTERNAL ERROR IN MEMORY BACKEND: addSlice for an AoS with more than one field" << std::endl;
	    	return;
	    }
//	    aos.push_back(sliceAos.aos[0]);
	    aos.push_back(sliceAos.aos[0]->clone());
	}
	else //The top AoS is not  a timed AoS
	{
	    if(aos.size() == 0) //First time a slice is added
	    {
		for(size_t i = 0; i < sliceAos.aos.size(); i++)
		    aos.push_back(sliceAos.aos[i]->clone());
	    }
	    else
	    {
		if(aos.size() != sliceAos.aos.size())
		{
			std::cout << "INTERNAL ERROR IN MEMORY BACKEND: addSlice for an AoS with non consistent static AoS" << std::endl;
			return;
		}
		for(size_t i = 0; i < aos.size(); i++)
		{
			aos[i]->addSlice(*sliceAos.aos[i], ctx);
//			aos[i]->addSlice(*sliceAos.aos[i]->clone(), ctx);
		}
	    }
	}
    }

    void UalStruct::addSlice(UalStruct &ualSlice, ArraystructContext *ctx)
    {
	//First step: check time dependent fields that are not AoS
	for(auto &field: dataFields)
	{
	    if(field.second->isTimed())
	    	field.second->addSlice(*ualSlice.dataFields[field.first]);
	}
	//Second step: check time dependent ApS fields	
	for(auto &aosField: aosFields)
	{
//Check if the AoS to be added has been declared in the slice
	    if(ualSlice.aosFields[aosField.first])
	    	aosField.second->addSlice(*ualSlice.aosFields[aosField.first], ctx);
	}
    }



  /**
     Writes data.
     This function writes a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name 
     @param[in] timebasename time base field name
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (NULL is dim=0)
     @throw BackendException
  */
    void MemoryBackend::writeData(OperationContext *ctx,
			 std::string fieldname,
			 std::string timebasename, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size)
    {
/*if(datatype == DOUBLE_DATA)
    std::cout << "WRITE DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << *(double *)data << std::endl;
else if (datatype == INTEGER_DATA || datatype == COMPLEX_DATA)
    std::cout << "WRITE DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << *(int *)data << std::endl;
else
    std::cout << "WRITE DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << (char *)data << std::endl;
*/

	UalStruct *ids = getIds(ctx);
	UalData *ualData = ids->getData(fieldname);
	if(ctx->getRangemode() == GLOBAL_OP)
	    ualData->writeData(datatype, dim, size, (unsigned char *)data, timebasename);
	else
	    ualData->addSlice(datatype, dim, size, (unsigned char *)data);	
	//ids->setData(fieldname, ualData);
    }
   /**
     Reads data.
     This function reads a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name
     @param[in] istimed specify the time-dependency of the field
     @param[out] data returned pointer on the read data 
     @param[out] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[out] dim returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[out] size array returned with elements filled at the size of each dimension 
     @throw BackendException
  */
    int MemoryBackend::readData(OperationContext *ctx,
			std::string fieldname,
			std::string timebase,
			void** data,
			int* datatype,
			int* dim,
			int* size)
    {

	UalStruct *ids = getIds(ctx);
	UalData *ualData = ids->getData(fieldname);
	int status = 1;
	if(ctx->getRangemode() == GLOBAL_OP)
	{
	    switch(ualData->getMapState())
	    {
	    	case UalData::MAPPED:
		    status= ualData->readData(data, datatype, dim, size);
		    break;
	    	case UalData::UNMAPPED:
		    targetB->readData(ctx, fieldname, timebase, data, datatype, dim, size);
		    ualData->writeData(*datatype, *dim, size, (unsigned char *)*data, timebase);
		    break;
		case UalData::SLICE_MAPPED:
		    targetB->readData(ctx, fieldname, timebase, data, datatype, dim, size);
		    ualData->prependData(*datatype, *dim, size, (unsigned char *)*data);
		    ualData->readData(data, datatype, dim, size);
		    break;
		default: {}//Nothing
	    }
	    
	}
	else // SLICE_OP
	{
	  //First make sure the cache is aligned
	    //Fake ctx to target backend in order to force global operation
 	    OperationContext newCtx(*ctx, ctx->getDataobjectName(), READ_OP, GLOBAL_OP, 0, 0);
 	    if(ualData->getMapState() == UalData::UNMAPPED)
	    {
		targetB->readData(&newCtx, fieldname, timebase, data, datatype, dim, size);
		ualData->writeData(*datatype, *dim, size, *(unsigned char **)data, timebase);
	    }
	    else if(ualData->getMapState() == UalData::SLICE_MAPPED) 
	    {
		targetB->readData(&newCtx, fieldname, timebase, data, datatype, dim, size);
		ualData->prependData(*datatype, *dim, size, *(unsigned char **)data);
	    }		
	    //At this point all the info is in cache, we need to get time
	    double *timeData;
	    int timeDatatype;
	    int timeNumDims;
	    int timeDims[16];
	    if(timebase.length() == 0)  //handle empty time and /time
	    {
		return ualData->readData(data, datatype, dim, size);  //Not time dependent
	    }
	    else
	    {
		if(timebase[0] == '/')
    	    	    status = readData(&newCtx, timebase.substr(1), timebase.substr(1), (void **)&timeData, &timeDatatype, &timeNumDims, timeDims);
		else
    	    	    status = readData(&newCtx, timebase, timebase, (void **)&timeData, &timeDatatype, &timeNumDims, timeDims);
	    }
	    if(!status) return 0;
	    //Check timebase consistency
	    if(timeDatatype != ualconst::double_data || timeNumDims != 1)
	    {
		std::cout << "INTERNAL ERROR: Inconsistent timebase information " << ctx->getDataobjectName() << "  " << timebase << std::endl;
		throw  UALBackendException("Internal error: Inconsistent timebase information",LOG);
	    }
	    ualData->readTimeSlice(timeData, timeDims[0], ctx->getTime(), data, datatype, dim, size, ctx->getInterpmode());
	    free((char *)timeData);
	}
/* if(*datatype == DOUBLE_DATA)
    std::cout << "READ DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << **(double **)data << std::endl;
else if (*datatype == INTEGER_DATA || *datatype == COMPLEX_DATA)
    std::cout << "READ DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << **(int **)data << std::endl;
else
    std::cout << "READ DATA IDS:" << ctx->getDataobjectName() << "   FIELD: " << fieldname << *(char **)data << std::endl;
*/
        return status;
    }
  /*
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx pointer on operation context 
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @throw BackendException
  **/
    void MemoryBackend::deleteData(OperationContext *ctx,
			  std::string fieldname)
    {
	UalStruct *ids = getIds(ctx);
	if(ctx->getType() == CTX_ARRAYSTRUCT_TYPE) //Only in this case actions are required 
        {
	    UalAoS *aos = ids->getSubAoS(fieldname);
	    aos->deleteData();
	}
	else
	{
	    if(ids->isAoSMapped(fieldname))
	    {
		UalAoS *aos = ids->getSubAoS(fieldname);
		aos->deleteData();
	    }
	    else
	    {
	    	UalData *ualData = ids->getData(fieldname);
	    	ualData->deleteData();
	    }
	}
    }


  /**
     Starts writing a new array of structures.
     This function initiates the writing of a new top-level or nested array of structure.
     @param[in] ctx pointer on array of structure context
     @param[in] size specify the size of the array (number of elements)
     @throw BackendException
  */
    void MemoryBackend::beginWriteArraystruct(ArraystructContext *ctx,
				     int size)
    {
//If we are startng a writeArrayStruct for a SLICE_OP and the AoS is NOT mapped, take NO action
	if(ctx->getRangemode() == SLICE_OP && !isMappedAoS(ctx))
	{
//	    targetB->beginWriteArraystruct(ctx, size);
	    targetB->beginArraystructAction(ctx, &size);
	    return;
	}

//If it is to topomost ArrayStructContext, prepare currentAos 
	if(!ctx->getParent()) //If it is the topmost AoS
	{
	    currentAos.deleteData();
	//prepare empty structure if not existing
	    UalStruct *ids = getIds(ctx);	
	    UalAoS *aos = ids->getSubAoS(ctx->getPath());
	    if(ctx->getRangemode() == GLOBAL_OP) 
	    	aos->deleteData();
	}



//	targetB->beginWriteArraystruct(ctx, size);
//No write propagated to target backend
    }
  /**
     Starts reading a new array of structures.
     This function initiates the reading of a new top-level or nested array of structure.
     @param[in] ctx pointer on array of structure context
     @param[out] size specify the size of the array (number of elements)
     @throw BackendException
  */
    void MemoryBackend::beginReadArraystruct(ArraystructContext *ctx,
				    int* size)
    {

//std::cout << "BEGIN READ ARRAYSTUCT " << ctx->getDataobjectName() << std::endl;

//If  the AoS is NOT mapped, take NO action, just pass it to the target backend
	if(!isMappedAoS(ctx))
	{
//	    targetB->beginReadArraystruct(ctx, size);
	    targetB->beginArraystructAction(ctx, size);
	    return;
	}
//If it is a slice, then size = 1
	if(ctx->getRangemode() == SLICE_OP && !ctx->getParent())
	{
	    UalAoS *aos = getAoS(ctx, false);  //Gabriele Jan 2019
            if(aos->timebase == "")
	        *size = aos->aos.size();  //Static AoS
	    else
		*size = 1;
	    prepareSlice(ctx);
	}
	else
	{
    //Get the  AoS referred to the passed ArrayStructContext. If isCurrent, then the currentAoS is considered, otherwise the corresponding AoS in the main IDS UalStruct is condiered.
    	    UalAoS *aos = getAoS(ctx, false);
	    *size = aos->aos.size();
	}
    }

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
  void MemoryBackend::putInArraystruct(ArraystructContext *ctx,
				std::string fieldname,
				std::string timebase,  //Added to handle time dependent signals as firlds of static AoS
				int idx,
				void* data,
				int datatype,
				int dim,
				int* size)

    {
//If the AoS is NOT mapped, take NO action
	if(!isMappedAoS(ctx))
	{
//	    targetB->putInArraystruct(ctx, fieldname, timebase, idx, data, datatype, dim, size);
	    targetB->writeData(ctx, fieldname, timebase, data, datatype, dim, size);
	    return;
	}
	UalData *ualData;
	if(ctx->getRangemode() == SLICE_OP)
	    ualData = getData(ctx, idx, fieldname, true);  //We are going to write in currentAos
	else
	    ualData = getData(ctx, idx, fieldname, false);  //We are going in AoS in main IDS

	ualData->writeData(datatype, dim, size, (unsigned char *)data, timebase);
    }

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
    int MemoryBackend::getFromArraystruct(ArraystructContext *ctx,
				  std::string fieldname,
				  std::string timebase,
				  int idx,
				  void** data,
				  int* datatype,
				  int* dim,
				  int* size) 
    {

//std::cout << "GET FROM ARRAYSTRUCT " << ctx->getPath() << "   " << fieldname << "   IDX:" << idx << std::endl;
        int status;
//If we are starting a readArrayStruct for a SLICE_OP or the AoS is NOT mapped, take NO actio, just pass it to the target backend
	if(!isMappedAoS(ctx))
	{
//	    targetB->getFromArraystruct(ctx, fieldname, idx, data, datatype, dim, size);
	    targetB->readData((Context *)ctx, fieldname, timebase, data, datatype, dim, size);
	    return 1;
	}
//Otherwise data are mapped
	if(ctx->getRangemode() == SLICE_OP)
	{
	    if(timebase.size() > 0)  //For timed AoS slices, the dimension must be increased putting 1 as 0D
	    {
	    	status = getSliceFromAoS(ctx, fieldname, idx, data, datatype, dim, size);
		size[*dim] = 1;
		(*dim)++;
	    }
	    else
	    	status = getSliceFromAoS(ctx, fieldname, idx, data, datatype, dim, size);
	    return status;
	}

	else
	    return getFromAoS(ctx, fieldname, idx, data, datatype, dim, size);
	return 1;
    }

    void MemoryBackend::endAction(Context *inCtx)
    {
    
        if(inCtx->getType() == CTX_ARRAYSTRUCT_TYPE) //Only in this case actions are required 
        {
	    ArraystructContext *ctx = (ArraystructContext *)inCtx;
//If we are startng a writeArrayStruct for a SLICE_OP and the AoS is NOT mapped, take NO action
	    if(ctx->getRangemode() == SLICE_OP && !isMappedAoS(ctx))
	    {
	    	targetB->endAction(inCtx);
	        return;
	    }
	    if(ctx->getParent() == NULL)
	    {
	        if(ctx->getAccessmode() == ualconst::write_op)
	        {
		    UalAoS *aos = getAoS(ctx, false);   //AoS stored in main IDS UalStruct 
		    UalAoS *currAos = getAoS(ctx, true);  //Current AoS being written
		    if(ctx->getRangemode() == SLICE_OP)
		    {
			if(aos->timebase != ctx->getTimebasePath())
			    aos->timebase = ctx->getTimebasePath();
			aos->addSlice(*currAos, ctx);
//			aos->addSlice(*(currAos->clone()), ctx);
		    }
	        }
	    }
	    return;
        }
//Everything outside AoS is expected to be mapped, so no action is required in write
//Only in read mode target end action is called NOTE: in MDSplus backend this is neutral, for others we shopuld check whether 
    }

    void MemoryBackend::beginAction(OperationContext *ctx)  
    {
//If reading or writing slices for non mapped AoS, pass to target backend
	if((ctx->getType() != CTX_ARRAYSTRUCT_TYPE && ctx->getAccessmode() == ualconst::read_op) || (ctx->getType() == CTX_ARRAYSTRUCT_TYPE && !isMappedAoS((ArraystructContext *)ctx)))	
	    targetB->beginAction(ctx);
    }



    void MemoryBackend::flush(PulseContext *ctx, std::string dataobjectName)
    {
	OperationContext newCtx(*ctx, dataobjectName, GLOBAL_OP);
	targetB->beginAction(&newCtx);
	UalStruct *ids = getIds(&newCtx);
	for(auto &field: ids->dataFields)
	{
	    std::string fieldName = field.first;
	    UalData *fieldData = field.second;
	    void *data;
	    int datatype, numDims;
	    int dims[MAX_DIM];
	    fieldData->readData(&data, &datatype, &numDims, dims);
	    targetB->writeData(&newCtx, fieldName, fieldData->getTimebase(), data, datatype, numDims, dims);
	}
	for(auto &aosField: ids->aosFields)
	{
	    std::string fieldName = aosField.first;
	    UalAoS *fieldAos = aosField.second;
	    flushAoS(&newCtx, fieldName, *fieldAos);
	}
	targetB->endAction(&newCtx);
    }





    void MemoryBackend::flushAoS(OperationContext *ctx, std::string fieldName, UalAoS &ualAos)
    {
	ArraystructContext arrayStructCtx(*ctx, fieldName, ualAos.timebase);
	recFlushAoS(ualAos, ctx, &arrayStructCtx);
    }

    void MemoryBackend::recFlushAoS(UalAoS &ualAos, OperationContext *opCtx, ArraystructContext *ctx)
    {
//    	targetB->beginWriteArraystruct(ctx, ualAos.aos.size());
	int size = ualAos.aos.size();
    	targetB->beginArraystructAction(ctx, &size);
	for(size_t idx = 0; idx < ualAos.aos.size(); idx++)
	{
	    for(auto &field: ualAos.aos[idx]->dataFields)
	    {
		std::string fieldName = field.first;
		UalData *fieldData = field.second;
		void *data;
		int datatype;
		int dim;
		int size[MAX_DIM];
    		fieldData->readData(&data, &datatype, &dim, size);
	
		//targetB->putInArraystruct(ctx, fieldName, fieldData->getTimebase(), idx, data, datatype, dim, size);
		targetB->writeData(ctx, fieldName, fieldData->getTimebase(), data, datatype, dim, size);
	    }
	    //Normal fields done, now handle AoS
	    for(auto &aosField: ualAos.aos[idx]->aosFields)
	    {
		std::string fieldName = aosField.first;
		UalAoS *currAos = aosField.second;

		ArraystructContext currCtx(*opCtx, fieldName, currAos->timebase, ctx, idx);
		recFlushAoS(*currAos, opCtx, &currCtx);
	    }
	}
	targetB->endAction(ctx);
    }


    int MemoryBackend::getFromAoS(ArraystructContext *ctx,
				  std::string fieldname,
				  int idx,
				  void** data,
				  int* datatype,
				  int* dim,
				  int* size) 
    {
	UalData *ualData = getData(ctx, idx, fieldname, false);  //We are going to read the AoS in main structure
	return ualData->readData(data, datatype, dim, size);
    }



    int MemoryBackend::getSliceFromAoS(ArraystructContext *ctx,
				  std::string fieldname,
				  int idx,
				  void** data,
				  int* datatype,
				  int* dim,
				  int* size) 
    {
	UalData *ualData = getData(ctx, idx, fieldname, true);  //We are going to read the AoS slice in the memory struture prepared by prepareSlice()
	return ualData->readData(data, datatype, dim, size);
    }



    void MemoryBackend::prepareSlice(ArraystructContext *ctx)
    {
	double time = ctx->getTime();
	UalStruct *ids = getIds(ctx);
	UalAoS *topAos = ids->getSubAoS(ctx->getPath());

	currentAos.deleteData();  //Prapere currentAoS that is going to contain the selected slice

	std::vector<StructPath> ctxV;
	StructPath topSp(ids, ctx->getDataobjectName());
	ctxV.push_back(topSp);
	StructPath sp(topAos->aos[ctx->getIndex()], ctx->getPath());
	ctxV.push_back(sp);
	if(topAos->timebase != "") //if the top AoS is timed
	{
	    int sliceIdx1, sliceIdx2;
	    currentAos.timebase = topAos->timebase;

	    std::string currTimebase;
//	    if(topAos->timebase.size() > ctx->getPath().size() && ctx->getPath() == topAos->timebase.substr(ctx->getPath().size()))
	    if(!(topAos->timebase[0] == '/'))
	    	currTimebase = topAos->timebase.substr(ctx->getPath().size() + 1);
	    else
	    	currTimebase = topAos->timebase;

 	    getSliceIdxs(currTimebase, time, ctxV, sliceIdx1, sliceIdx2, topAos);
 	    //getSliceIdxs(topAos->timebase, time, ctxV, sliceIdx1, sliceIdx2, topAos);
//For the moment only PREVIOUS SAMPLE is supported
	    currentAos.aos.push_back(topAos->aos[sliceIdx1]->clone());
	    return; //Done!!
	}
	for(size_t i = 0; i < topAos->aos.size(); i++)
	{
	    currentAos.aos.push_back(prepareSliceRec(ctx, *topAos->aos[i], *ids, time, ctxV, topAos));
	}
    }		
	
    UalData *MemoryBackend::getUalSlice(ArraystructContext *ctx, UalData &inData, double time)
    {
	UalData *retData = new UalData;
	void *data;
	int datatype;
	int numDims;
	int dims[16];
	double *timeData;
	int timeDatatype;
	int timeNumDims;
	int timeDims[16];
	OperationContext newCtx(*ctx, ctx->getDataobjectName(), READ_OP);
    	readData(&newCtx, inData.getTimebase(), inData.getTimebase(), (void **)&timeData, &timeDatatype, &timeNumDims, timeDims);
	    //Check timebase consistency
	inData.readTimeSlice((double *)timeData, timeDims[0],  time,  &data, &datatype, &numDims, dims, ualconst::previous_interp);
    	retData->writeData(datatype, numDims, dims, (unsigned char *)data, "");
	free((char *)data);
	free((char *)timeData);
	return retData;
    }
    UalData *MemoryBackend::getUalSlice(ArraystructContext *ctx, UalData &inData, double time, std::vector<double> timebaseV)
    {
	UalData *retData = new UalData;
	void *data;
	int datatype;
	int numDims;
	int dims[16];
	
	inData.readTimeSlice(timebaseV.data(), timebaseV.size(),  time,  &data, &datatype, &numDims, dims, ualconst::previous_interp);
    	retData->writeData(datatype, numDims, dims, (unsigned char *)data, "");
	free((char *)data);
	return retData;
    }

    UalStruct *MemoryBackend::prepareSliceRec(ArraystructContext *ctx, UalStruct &ualStruct, UalStruct &ids, double time, std::vector<StructPath> &ctxV, UalAoS *aos)
    {
	UalStruct *retUalStruct = new UalStruct;;
	for(auto &field:ualStruct.dataFields)

	{
	    if(field.second->getTimebase() == "")
		retUalStruct->dataFields[field.first] = field.second->clone();
	    else
	    {
		std::vector<double> timebaseV = getTimebaseVect(field.second->getTimebase(), ctxV);
		UalData *retData = getUalSlice(ctx, *field.second, time, timebaseV);
////NOTE: FOR COMPATIBILITY WITH CURRENT MDSplus backend, may be removed later
		retData->shrinkDimension();
////////////////////////////////////////////////////////////////////////////////

	    	retUalStruct->dataFields[field.first] = retData;
	    }
	}
	for(auto &aosField:ualStruct.aosFields)
	{
	    UalAoS *currAos = aosField.second;
	    UalAoS *newAos;
	    if(currAos->timebase != "")
	    {
	    	int sliceIdx1, sliceIdx2;

		std::vector<StructPath> newCtxV = ctxV;
		StructPath sp(currAos->aos[0], aosField.first);
		newCtxV.push_back(sp);

	        getSliceIdxs(currAos->timebase, time, newCtxV, sliceIdx1, sliceIdx2, currAos);
//For the moment only PREVIOUS SAMPLE is supported
		newAos = new UalAoS;
		newAos->aos.push_back(currAos->aos[sliceIdx1]->clone());
	    }
	    else
	    {
		newAos = new UalAoS;
		for(size_t i = 0; i < currAos->aos.size(); i++)
	  	{	
		    std::vector<StructPath> newCtxV = ctxV;
		    StructPath sp(currAos->aos[i], aosField.first);
		    newCtxV.push_back(sp);
	    	    newAos->aos.push_back(prepareSliceRec(ctx, *currAos->aos[i], ids, time, newCtxV, currAos));
		}
	    }
 	    retUalStruct->aosFields[aosField.first] = newAos;
	}
	return retUalStruct;
    }


    std::vector<double> MemoryBackend::getTimebaseVect(std::string path, std::vector<StructPath> &ctxV, UalAoS *aos)
    {
//First step: go up of as many including AoS as occurrences of ../
	std::string currPath = path;
        std::size_t currPos = currPath.find("../");
	int currIdx = ctxV.size() - 1;
	size_t aosPos;
	if(currPos != std::string::npos)
	{
	    std::string currAosPath = ctxV[currIdx].path;

	    while(currPos != std::string::npos)
	    {
	    	currPath = currPath.substr(currPos+3);
		currPos = currPath.find("../");
		aosPos = currAosPath.find_last_of("/");
		if(aosPos == std::string::npos)
		{
		    if(currIdx == 0) break;   //Finished climbing up AoS struct
		    currIdx--;
		    currAosPath = ctxV[currIdx].path;		
		}
		else
		   currAosPath = currAosPath.substr(0,aosPos);
	    } 
	    if(currPos == std::string::npos) //If time definition still within the AoS
	    {
		if(aosPos != std::string::npos)
		{
		    UalStruct *ualStruct = ctxV[currIdx-1].ualStruct;
		    std::string actPath = currAosPath+"/"+currPath;
		    return ualStruct->getData(actPath)->getDoubleVect();
		}
		else
		{
		    UalStruct *ualStruct = ctxV[currIdx].ualStruct;
	    	    return ualStruct->getData(currPath)->getDoubleVect();
		}
	    }
	    else //Time definition inside the IDS but outside the AoS
	    {
		std::string aosPath = ctxV[0].path;
	    	while(currPos != std::string::npos)
	    	{
	    	    currPath = currPath.substr(currPos+3);
		    currPos = currPath.find("../");
		    std::size_t aosPos = aosPath.find_last_of("/");
		    if(aosPos == std::string::npos)
			aosPath = "";
		    else
			aosPath = aosPath.substr(0, aosPos);
	    	}
		if(aosPath == "")
		    return ctxV[0].ualStruct->getData(currPath)->getDoubleVect();
		else
		    return ctxV[0].ualStruct->getData(aosPath+"/"+currPath)->getDoubleVect();
	    }
	}
	else if(path[0] == '/')
	    return ctxV[0].ualStruct->getData(currPath.substr(1))->getDoubleVect();   //OCIO CHE QUI SCIOPA TUTO
	else //it is the reference to internal field time
	{
	    if(!aos) //We are dealing with time dependent firlds of static AoS
	    	return ctxV[ctxV.size()-1].ualStruct->getData(currPath)->getDoubleVect();
	    else
	    {
	    	std::vector<double> retTimeV;
	    	for(size_t i = 0; i < aos->aos.size(); i++)
	    	{
		    retTimeV.push_back(aos->aos[i]->getData(path)->getDouble());
	    	}
	    	return retTimeV; 
	    }
	}
    }

/*	
    std::vector<double> MemoryBackend::getTimebaseVect(std::string path, UalAoS &aos, std::vector<UalStruct *> &aosParentV, ArraystructContext *ctx)
    {
//First step: go up of as many including AoS as occurrences of ../
	std::string currPath = path;
        std::size_t currPos = currPath.find("../");
	size_t upIdx = 0;
	if(currPos != std::string::npos)  //If the path begins with ../
	{
	    while(currPos != std::string::npos && upIdx < aosParentV.size())
	    {
		upIdx++;
	    	currPath = currPath.substr(currPos+3);
		currPos = currPath.find("../");
	    }
	    if(currPos == std::string::npos) //If time definition still within the AoS
	    	return aosParentV[aosParentV.size() - upIdx - 1]->getData(currPath)->getDoubleVect();
	    else //Time definition inside the IDS but outside the AoS
	    {
		std::string aosPath = ctx->getPath();
	    	while(currPos != std::string::npos)
	    	{
	    	    currPath = currPath.substr(currPos+3);
		    currPos = currPath.find("../");
		    std::size_t aosPos = aosPath.find_last_of("/");
		    if(aosPos == std::string::npos)
			aosPath = "";
		    else
			aosPath = aosPath.substr(0, aosPos);
	    	}
		if(aosPath == "")
		    return aosParentV[0]->getData(currPath)->getDoubleVect();
		else
		    return aosParentV[0]->getData(aosPath+"/"+currPath)->getDoubleVect();
	    }
	}
	else if(path[0] == '/')
	    return aosParentV[0]->getData(currPath.substr(1))->getDoubleVect();   //OCIO CHE QUI SCIOPA TUTO
	else //it is the reference to internal field time
	{
	    std::vector<double> retTimeV;
	    for(size_t i = 0; i < aos.aos.size(); i++)
	    {
		retTimeV.push_back(aos.aos[i]->getData(path)->getDouble());
	    }
	    return retTimeV;
	}
    }
*/ 
    void MemoryBackend::getSliceIdxs(std::string timebase, double time, std::vector<StructPath> &ctxV, int &sliceIdx1, int &sliceIdx2, UalAoS *aos)
    {
	std::vector<double> timesV = getTimebaseVect(timebase, ctxV, aos);
	if(time <= timesV[0])
	{
	    sliceIdx1 = sliceIdx2 = 0;
	}
	else if(time >= timesV[timesV.size() - 1])
	{
	    sliceIdx1 = sliceIdx2 = timesV.size() - 1;
	}
	else
	{
//	    for(int currSlice = 0; currSlice < timesV[timesV.size() - 1]; currSlice++)
	    for(size_t currSlice = 0; currSlice < timesV.size(); currSlice++)
	    {
		if(time >= timesV[currSlice] && time < timesV[currSlice+1])
		{
		    sliceIdx1 = currSlice;
		    sliceIdx2 = sliceIdx1 + 1;
		    break;
		}
	    }
	}
    }


    std::string MemoryBackend::getIdsPath(OperationContext *ctx)
    {
	return std::to_string(ctx->getShot())+"/"+std::to_string(ctx->getRun())+"/"+ctx->getDataobjectName();
    }

    //Get the IDS (in UalStruct) 
    UalStruct  *MemoryBackend::getIds(OperationContext *ctx)
    {

//std::cout << "GET IDS FOR " << ctx->getDataobjectName() << std::endl;

	UalStruct ids;
	std::string idsPath = getIdsPath(ctx);
	if(idsMap.find(idsPath) != idsMap.end())
	{
	    return idsMap.at(idsPath);
	} 
	else
	{
	    idsMap[idsPath] = new UalStruct;
	    return idsMap.at(idsPath);
	}
/*	try {
	   // return idsMap.at(ctx->getDataobjectName());
	    return idsMap.at(idsPath);
	} catch (const std::out_of_range& oor) 
	{
	    idsMap[idsPath] = new UalStruct;
	    return idsMap.at(idsPath);
	}
*/    }



    //Get the  AoS referred to the passed ArrayStructContext. If isCurrent, then the currentAoS is considered, otherwise the corresponding AoS in the main IDS UalStruct is condiered.
    UalAoS *MemoryBackend::getAoS(ArraystructContext *ctx, bool isCurrent)
    {
	std::vector<ArraystructContext *>currCtxV;
	ArraystructContext *currCtx = ctx;
	do {
	    currCtxV.push_back(currCtx);
	    currCtx = currCtx->getParent();
	} while(currCtx);
	UalAoS *topAos;
	if(isCurrent)
	    topAos = &currentAos;
	else
	{
	    UalStruct *ualStruct = getIds(currCtxV[currCtxV.size() - 1]);
	    topAos = ualStruct->getSubAoS(currCtxV[currCtxV.size() - 1]->getPath());
	}	

	for(int i = currCtxV.size() - 2; i >= 0; i--)
	{
	    if(topAos->aos.size() <= (size_t)(currCtxV[i]->getParent()->getIndex()))  
//	    if(topAos->aos.size() <= (size_t)(currCtxV[i]->getIndex()))
	    {
		int prevSize = topAos->aos.size();
//		topAos->aos.resize(currCtxV[i]->getIndex()+1);
		topAos->aos.resize(currCtxV[i]->getParent()->getIndex()+1);  
		for(int j = prevSize; j <= currCtxV[i]->getParent()->getIndex(); j++) 
//		for(int j = prevSize; j <= currCtxV[i]->getIndex(); j++)
		    topAos->aos[j] = new UalStruct;
	    }
	    topAos = topAos->aos[currCtxV[i]->getParent()->getIndex()]->getSubAoS(currCtxV[i]->getPath()); 
//	    topAos = topAos->aos[currCtxV[i]->getIndex()]->getSubAoS(currCtxV[i]->getPath());

	    if(topAos->timebase != currCtxV[i]->getTimebasePath())
		topAos->timebase = currCtxV[i]->getTimebasePath();

	} 
	return topAos;
    }

    UalData *MemoryBackend::getData(ArraystructContext *ctx, int idx, std::string path, bool isCurrent)
    {
	UalAoS *aos = getAoS(ctx, isCurrent);
	if(aos->timebase != ctx->getTimebasePath())
	    aos->timebase = ctx->getTimebasePath();
	if(aos->aos.size() <= (size_t)idx)
	{
	    int prevSize = aos->aos.size();
	    aos->aos.resize(idx+1);
	    for(int j = prevSize; j <= idx; j++)
		aos->aos[j] = new UalStruct;
	}
	return aos->aos[idx]->getData(path);
    }

//Check if the passed context refers to an AoS that is mapped in memory (i.e. for which deleteData or putData has been issued)
    bool MemoryBackend::isMappedAoS(ArraystructContext *ctx)
    {
	std::vector<ArraystructContext *>currCtxV;
	ArraystructContext *currCtx = ctx;
	do {
	    currCtxV.push_back(currCtx);
	    currCtx = currCtx->getParent();
	} while(currCtx);
	//currCtxV[currCtxV.size() - 1] is the context of the topmost AoS, that is also OperationContex
	std::string idsPath = getIdsPath(currCtxV[currCtxV.size() - 1]);
	//fullAosPath is the composition of the IDS path and the internal path within the IDS of the top AoS
	std::string fullAosPath = idsPath + "/";
	fullAosPath += currCtxV[currCtxV.size() - 1]->getPath();

	UalStruct *ids = getIds(currCtxV[currCtxV.size() - 1]);
	return ids->isAoSMapped(currCtxV[currCtxV.size() - 1]->getPath());
    }	



//////////////////////////////////////////////////////////////////////////

    UalData* UalData::clone()
    {
	UalData *newData = new UalData;
	*newData = *this;
	return newData;
    }


    int UalData::getItemSize(int inType)
    {
	switch(inType)
	{
	    case ualconst::char_data: return 1;
	    case ualconst::integer_data: return 4;
	    case ualconst::double_data: return 8;
	    case ualconst::complex_data: return 16;
	    default: return 0; //Never happens
	}
    }

    UalData::UalData()
    {
	timed = false;
	mapState = UNMAPPED;
    }
    std::vector<double> UalData::getDoubleVect()
    {
	if(type != ualconst::double_data)
	{
	    std::cout << "FATAL ERROR: time reference is not a double array" << std::endl;
	    throw UALBackendException("FATAL ERROR: time reference is not a double array",LOG); 
	}
	std::vector<double>res;
	for(size_t i = 0; i < bufV.size(); i++)
	    res.push_back(*(double *)(static_cast<unsigned char *>(bufV[i].get())));
	return res;
    }
    double UalData::getDouble()
    {
	if(type != ualconst::double_data)
	{
	    std::cout  << "FATAL ERROR: time reference is not double" << std::endl;
	    throw UALBackendException("FATAL ERROR: time reference is not double ",LOG); 
	}
	return *(double *)(static_cast<unsigned char *>(bufV[0].get()));
    }



    void UalData::setTimebase(std::string timebase)
    {
	timed = true;
	this->timebase = timebase;
    }
    void UalData::deleteData()
    {
	timebase = "";
	mapState = MAPPED;
	dimensionV.clear();
	bufV.clear();
    }

    void UalData::writeData(int type, int numDims, int *dims, unsigned char *buf, std::string timebase)
    {
	//Free previous data


	bufV.clear();
	mapState = MAPPED;
	this->type = type;
	this->timebase = timebase;
	timed = timebase != "";
	
	dimensionV.clear();
	int totSize = 1;
	for(int i = 0; i < numDims; i++)
	{
	    totSize *= dims[i];
	    dimensionV.push_back(dims[i]);
	}
	if(timed)
	{
//Handle  the case a simple scalar is passed (it should be a 1D array with one element)
	    int sliceSize = 1;
	    if(numDims == 0)
	    {
		sliceSize = 1;
	    	unsigned char *currBuf = new unsigned char[sliceSize*getItemSize(type)];
		memcpy(currBuf, buf, sliceSize * getItemSize(type));
		std::shared_ptr<unsigned char>sp(currBuf);
		bufV.push_back(sp);
		dimensionV.push_back(1);
	    }
	    else  //Normal case in which the slice is passed as array
	    {
		for(int i = 0; i < numDims - 1; i++)
		{
			sliceSize *= dims[i];
		} 
		for(int i = 0; i < dims[numDims-1]; i++)
		{
			unsigned char *currBuf = new unsigned char[sliceSize*getItemSize(type)];
			memcpy(currBuf, &buf[i * sliceSize * getItemSize(type)], sliceSize * getItemSize(type));
			std::shared_ptr<unsigned char>sp(currBuf);
			bufV.push_back(sp);
		}
	    }
	}
	else
	{
	    int currSize = totSize * getItemSize(type);
	    unsigned char *currBuf = new unsigned char[currSize];
	    memcpy(currBuf, buf, currSize);
	    bufV.push_back(std::shared_ptr<unsigned char>(currBuf));
	}
    }

//Called only when in state SLICE_MAPPED, that is, only when it contains only the most recent slices
    void UalData::prependData(int type, int numDims, int *dims, unsigned char *buf)
    {
	std::vector<std::shared_ptr<unsigned char>>newBufV;

	mapState = MAPPED;
	this->type = type;
	if(dimensionV.size() != (size_t)numDims)
	{
	    std::cout << "Internal error in Memory backend: wrong number of dimensions in prependData" << std::endl;
	    exit(0);
	}
	dimensionV[numDims - 1] += dims[numDims - 1];
	int sliceSize = 1;
	for(int i = 0; i < (int)dimensionV.size()-1; i++)
	    sliceSize *= dimensionV[i];
	for(int i = 0; i < dims[numDims - 1]; i++)
	{
	    int currSize = sliceSize * getItemSize(type);
	    unsigned char *currBuf = new unsigned char[currSize];
	    memcpy(currBuf, &buf[i * currSize], currSize);
	    newBufV.push_back(std::shared_ptr<unsigned char>(currBuf));
	}
	for(size_t i = 0; i < bufV.size(); i++)
	    newBufV.push_back(bufV[i]);
	bufV = newBufV;
    }
    void UalData::addSlice(int type, int numDims, int *dims, unsigned char *buf)
    {
	if(mapState == UNMAPPED)
	    mapState = SLICE_MAPPED;
	timed = true;
	if(bufV.size() == 0) //Yet an initialized data object
	{
	    dimensionV.clear();
	    this->type = type;
	    for(int i = 0; i < numDims-1; i++)  //The last passed dimension in slices is 1
	    {
	    	dimensionV.push_back(dims[i]);
	    }
	    dimensionV.push_back(0);
	}
	int sliceSize = 1;
	for(size_t i = 0; i < dimensionV.size() - 1; i++)
	    sliceSize *= dimensionV[i];

	unsigned char *currBuf = new unsigned char[sliceSize*getItemSize(type)];
	memcpy(currBuf, buf, sliceSize*getItemSize(type));
	dimensionV[dimensionV.size() - 1]++;
	bufV.push_back(std::shared_ptr<unsigned char>(currBuf));
    }
    void UalData::addSlice(UalData &slice)
    {
	if(mapState == UNMAPPED)
	    mapState = SLICE_MAPPED;
	timed = true;
	if(bufV.size() == 0) //Yet an initialized data object
	{
	    dimensionV.clear();
	    this->type = slice.type;
	    for(size_t i = 0; i < slice.dimensionV.size(); i++)
	    {
	    	dimensionV.push_back(slice.dimensionV[i]);
	    }
	}
	if(slice.dimensionV.size() == 0)
	{
	    if(dimensionV.size() == 0)
		dimensionV.push_back(1);
	    else
		dimensionV[0]++;
	    bufV.push_back(slice.bufV[0]);
	}
	else
	{
	    for(int i = 0; i < slice.dimensionV[slice.dimensionV.size() - 1]; i++)
	    {
	    	dimensionV[dimensionV.size()-1]++;
		bufV.push_back(slice.bufV[i]);
	    }
	}
    }

    int UalData::readData(void **retDataPtr, int *datatype, int *retNumDims, int *retDims)
    {
	if(mapState != MAPPED || bufV.size() == 0)
	    return 0;
//	    throw UALNoDataException("No data in memory" ,LOG);
	*datatype  = type;
	int totSize = 1;
	for(size_t i = 0; i < dimensionV.size(); i++)
	    totSize *= dimensionV[i];
	unsigned char *currBuf = (unsigned char *)malloc(totSize * getItemSize(type));
	if(timed)
	{
	    int sliceSize = totSize * getItemSize(type)/dimensionV[dimensionV.size()-1];
	    for(size_t i = 0; i < bufV.size(); i++)
	    {
		memcpy(&currBuf[i * sliceSize], bufV[i].get(), sliceSize);
	    }
	    *retDataPtr = currBuf;
	    for(size_t i = 0; i < dimensionV.size(); i++)
	        retDims[i] = dimensionV[i];
	    *retNumDims = dimensionV.size();
	}
	else
	{
	    memcpy(currBuf, bufV[0].get(), totSize * getItemSize(type));
	    *retDataPtr = currBuf;
	    *retNumDims = dimensionV.size();
	    for(size_t i = 0; i < dimensionV.size(); i++)
	    	retDims[i] = dimensionV[i];
	}
	return 1;
    }
    int UalData::readSlice(int sliceIdx, void **retDataPtr, int *datatype, int *retNumDims, int *retDims)
    {
	if(mapState != MAPPED || bufV.size() == 0)
	    return 0;
//	    throw UALNoDataException("No data in memory" ,LOG);

	if((size_t)sliceIdx >= bufV.size())
	{
	    std::cout << "Warning: slice idx outside limits converting mapped data to slice. Clipped." << std::endl;
	    sliceIdx = bufV.size() - 1;
	}
	int sliceSize = 1;
	for(int i = 0; i < (int)dimensionV.size()-1; i++)
	    sliceSize *= dimensionV[i];	
//	unsigned char *currBuf = new unsigned char[sliceSize * getItemSize(type)];
	unsigned char *currBuf = (unsigned char *)malloc(sliceSize * getItemSize(type));
	memcpy(currBuf, bufV[sliceIdx].get(), sliceSize * getItemSize(type));
	*retDataPtr = currBuf;
	*datatype = type;
	*retNumDims = dimensionV.size();
	for(int i = 0; i < (int)dimensionV.size()-1; i++)
	    retDims[i] = dimensionV[i];
	//The last dimension in returned slice is always 1
	if(dimensionV.size() > 0)
	    retDims[dimensionV.size()-1] = 1;
	return 1;
    }
    void UalData::readTimeSlice(double *times, int numTimes, double time, void **retDataPtr, int *datatype, int *retNumDims, int *retDims, int interpolation)
    {
	int sliceIdx1, sliceIdx2;
	if(time <= times[0])
	    sliceIdx1 = sliceIdx2 = 0;
	else if (time >= times[numTimes - 1])
	    sliceIdx1 = sliceIdx2 = numTimes - 1;
	else
	{
	    for(sliceIdx1 = 0; sliceIdx1 < numTimes - 1; sliceIdx1++)
	    {
		if(time >= times[sliceIdx1] && time < times[sliceIdx1+1])
		{
		    sliceIdx2 = sliceIdx1 + 1;
		    break;
		}
	    }
	}
	if(sliceIdx1 == sliceIdx2)
	{
	    readSlice(sliceIdx1, retDataPtr, datatype, retNumDims, retDims);
	}
	else
	{
	    switch(interpolation)
	    {
	        case ualconst::previous_interp:
		    readSlice(sliceIdx1, retDataPtr, datatype, retNumDims, retDims);
		    break;
	        case ualconst::closest_interp:
		    if((time - times[sliceIdx1]) < (times[sliceIdx2] - time))
		    	readSlice(sliceIdx1, retDataPtr, datatype, retNumDims, retDims);
		    else
		    	readSlice(sliceIdx2, retDataPtr, datatype, retNumDims, retDims);
		    break;
		case ualconst::linear_interp:
		{
		    void *data1, *data2;
		    readSlice(sliceIdx1, &data1, datatype, retNumDims, retDims);
		    readSlice(sliceIdx2, &data2, datatype, retNumDims, retDims);
		    int numSamples = 1;
		    for(int i = 0; i < *retNumDims; i++)
			numSamples *= retDims[i];
		    double delta = (time - times[sliceIdx1])/(times[sliceIdx2] - times[sliceIdx1]);

		    switch(*datatype)
		    {
			case ualconst::char_data: //Revert to previous sample
		    	    readSlice(sliceIdx1, retDataPtr, datatype, retNumDims, retDims);
			    break;
			case ualconst::integer_data:
			{
			    int *retData = (int *)malloc(sizeof(int) * numSamples);
			    for(int i = 0; i < numSamples; i++)
				retData[i] = ((int *)data1)[i] + delta * (((int *)data2)[i] - ((int *)data1)[i]),
			    *retDataPtr = retData;
			    break;
			}
			case ualconst::double_data:
			{
			    double *retData = (double *)malloc(sizeof(double) * numSamples);
			    for(int i = 0; i < numSamples; i++)
				retData[i] = ((double *)data1)[i] + delta * (((double *)data2)[i] - ((double *)data1)[i]),
			    *retDataPtr = retData;
			    break;
			}
			case ualconst::complex_data:
			{
			    double *retData = (double *)malloc(2*sizeof(double) * numSamples);
			    for(int i = 0; i < numSamples * 2; i++)
				retData[i] = ((double *)data1)[i] + delta * (((double *)data2)[i] - ((double *)data1)[i]),
			    *retDataPtr = retData;
			    break;
			}
			default: {}
		    }
		    free((char *)data1);
		    free((char *)data2);
		}
		default: {}
	    }
	}
    }

    UalAoS * UalAoS::clone()
    {
	UalAoS *newAos = new UalAoS;
	for(size_t i = 0; i < aos.size(); i++)
	    newAos->aos.push_back(aos[i]->clone());
	newAos->timebase = timebase;
	return newAos;
    }


    void UalAoS::deleteData()
    {
	timebase = "";
	for(size_t i = 0; i < aos.size(); i++)
	{
	    aos[i]->deleteData();
	    delete aos[i];
	}
	aos.clear();
    }
    UalData *UalStruct::getData(std::string path)
    {
	if(dataFields.find(path) != dataFields.end())  
	{  
	    return dataFields.at(path);
	} 
	else
	{
	    dataFields[path] = new UalData; 
	    return dataFields[path];
	}
/*	try  {  
	    return dataFields.at(path);
	} catch (const std::out_of_range& oor) 
	{
	    dataFields[path] = new UalData; 
	    return dataFields[path];
	} */
    }

    UalStruct *UalStruct::clone()
    {
	UalStruct *newStruct = new UalStruct;
	for(auto &field:dataFields)
	{
	    newStruct->dataFields[field.first] = field.second->clone();
	}
	for(auto &field:aosFields)
	{
	    newStruct->aosFields[field.first] = field.second->clone();
	}
	return newStruct;
    }



     void UalStruct::deleteData()
    {
	for(auto &field:dataFields)
	{
	    dataFields[field.first]->deleteData();
	    delete dataFields[field.first];
	}
	dataFields.clear();

	for(auto &field:aosFields)
	{
	    if(aosFields[field.first])
	    	aosFields[field.first]->deleteData();
	    delete aosFields[field.first];
	}
	aosFields.clear();
    }
    UalAoS *UalStruct::getSubAoS(std::string path)
    {
	if(aosFields.find(path) != aosFields.end())
	    return aosFields.at(path);
	else
	{
	    aosFields[path]=new UalAoS;
	    return aosFields[path];
	}
/*	try {
	    return aosFields.at(path);
	} catch (const std::out_of_range& oor) {aosFields[path]=new UalAoS;return aosFields[path];}
*/    }
    bool UalStruct::isAoSMapped(std::string path)
    {
	return (aosFields.find(path) != aosFields.end());
/*	try {
	    aosFields.at(path);
	    return true;
	} catch (const std::out_of_range& oor) {}
	return false;
*/    }


    void UalAoS::dump(int tabs)
    {
	for(int i = 0; i < tabs; i++)
	    std::cout << "  ";
	for (size_t i = 0; i < aos.size(); i++)
	{
	    std::cout << "[" << i << "]" << std::endl;
	    aos[i]->dump(tabs+1);
	}
    }




    UalAoS::~UalAoS()
    {
        for(size_t i = 0; i < aos.size(); i++)
	    delete aos[i];
    }

