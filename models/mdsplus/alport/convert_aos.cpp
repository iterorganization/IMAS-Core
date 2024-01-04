#include <mdsobjects.h>
#include <iostream>
#include <string.h>

static MDSplus::Data *convertApd(MDSplus::Data *inData)
{
    unsigned char clazz, dtype, nDims;
    int *dims;
    short length;
    MDSplus::Data **descs;
    inData->getInfo((char *)&clazz, (char *)&dtype, &length, (char *)&nDims, &dims, (void **)&descs);
    if(clazz != CLASS_APD)
    {   //Gabriele July 2018, convert strings and string arrays
        if(clazz == CLASS_A && dtype == DTYPE_T) // String Array
	{
//std::cout << std::endl<< "CONVERT STRING ARRAY " << inData << std::endl<< std::endl;
	    MDSplus::StringArray *sa = (MDSplus::StringArray *)inData;
	    int numStr = sa->getSize();
	    //GABRIELE Handle idiosincrasic case of null string array
	    int len;
	    char *buf;
	    if(numStr == 0)
	    {
		len = 1;
	    	buf = new char[1];
	    }
	    else
	    {
		MDSplus::String *s = (MDSplus::String *)sa->getElementAt(0);
		len = strlen(s->getString());
		buf = new char[len*numStr+1]; //To accomodate last null
		for(int i = 0; i < numStr; i++)
		{
			strcpy(&buf[i*len], sa->getElementAt(i)->getString());
		}
	    }
	    int dims[2];
	    dims[0] = len;
	    dims[1] = numStr;
	    return new MDSplus::Int8Array(buf, 2, dims);   //Memory leaks, who cares.....
	}
/*	else if (clazz == CLASS_S && dtype == DTYPE_T) // String 
	{
std::cout << std::endl<< "CONVERT STRING ARRAY " << inData << std::endl<< std::endl;
	    MDSplus::String *str = (MDSplus::String *)inData;
	    return new MDSplus::Int8Array(str->getString(), str->getSize());
	} */
	else
	{
	    return inData;
	}
    }
///////////////////////////////////////////////////////////////////
    inData->dtype = DTYPE_DSC;
    MDSplus::Data ** dscs = ((MDSplus::Apd *)inData)->getDscs();
    for(int i = 0; i < (int) ((MDSplus::Apd *)inData)->len(); i++)
	dscs[i] = convertApd(((MDSplus::Apd *)inData)->getDescAt(i));
    return inData;
}

static bool getTime(MDSplus::Apd *apd, double &time)
{
    if(apd->clazz != CLASS_APD)
    {
	std::cout << "Internal error, Expected Apd in getTime()"<<std::endl;
	return 0;
    }
    for(int i = 1; i < (int) apd->len(); i++)
    {
	if(!apd->getDescAt(i)) continue;
	if(apd->getDescAt(i)->clazz != CLASS_APD)
	{
	    std::cout << "Internal error, Expected Apd in getTime() 1"<<std::endl;
	    return 0;
    	}

	if (((MDSplus::Apd *)apd->getDescAt(i))->len() < 2)
   	{
	    std::cout << "Internal error, Expected at least 2 descriptors in getTime()" << std::endl;
	    return 0;
    	}
	try {
	    char *currName = ((MDSplus::Apd *)apd->getDescAt(i))->getDescAt(0)->getString();
	    if(!strcmp(currName, "time"))
	    {
		time = ((MDSplus::Apd *)apd->getDescAt(i))->getDescAt(1)->getDouble();
//std::cout << "TIME: " << time << std::endl;
		return true;
	    }
	}catch(MDSplus::MdsException &exc){}
    }
    return false;
}


int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        std::cout << "Usage: convert_aos <inPulse>, <inPath>, <outPulse>, <outPath>" << std::endl;
   	return 0;
    }
    MDSplus::Tree *inT, *outT;
    MDSplus::TreeNode *inN, *outN;
    try {
      inT = new MDSplus::Tree("ids", atoi(argv[1]));
      inN = inT->getNode(argv[2]);
      outT = new MDSplus::Tree("ids", atoi(argv[3]));
//      std::string outPathStr = std::string(argv[4])+":AOS0";
      std::string outPathStr = std::string(argv[4])+":AOS";
      outN = outT->getNode(outPathStr.c_str());
    }catch (MDSplus::MdsException &exc)
    {
	std::cout << "Error opening in/out tree: " << exc.what() << std::endl;
	return 0;
    }

    outN->deleteData();
    int numSegments = inN->getNumSegments();
    std::vector<double> times;

std::cout << "NumSegments: " << numSegments << std::endl;
    try {
        int prevSegDim = 0;
	for(int segIdx = 0; segIdx < numSegments; segIdx++)
	{
	MDSplus::Data *s = inN->getSegment(segIdx);
//Check for multiple serialized AoS
		MDSplus::Data *dimD = inN->getSegmentDim(segIdx);
		int numDims;
		try {
		    int *dim = dimD->getIntArray(&numDims);
		    delete [] dim;std::cout << numDims << std::endl;
		}catch(MDSplus::MdsException &exc)
		{ 
		    try {
			int actSegDim = dimD->getInt();
			numDims = actSegDim - prevSegDim + 1;
			prevSegDim = actSegDim + 1;
		    }catch(MDSplus::MdsException &exc) {numDims = 1;}
		}
//std::cout << "Working on segment " << segIdx << " of " << numSegments << "  with " << numDims << "  Aos"  << "(Segment dim: : " << dimD<< ")" << std::endl;
std::cout << "Working on segment " << segIdx << " of " << numSegments << "  with " << numDims << "  Aos"  << std::endl;
		MDSplus::deleteData(dimD);
		MDSplus::Data *serialized = s->data();
		MDSplus::deleteData(s);
		MDSplus::Data *segStartD, *segEndD;
		inN->getSegmentLimits(segIdx, &segStartD, &segEndD);
		int segStart = segStartD->getInt();
		MDSplus::deleteData(segStartD);
		MDSplus::deleteData(segEndD);
		if(numDims == 1)
		{
		    MDSplus::Data *deserialized;
		    try {
			deserialized = MDSplus::deserialize(serialized);
		    }
		    catch(MDSplus::MdsException &exc)
		    {
			int len;
			char *serBytes = serialized->getByteArray(&len);
			MDSplus::Data *newSerialized = new MDSplus::Uint8Array((unsigned char *)&serBytes[sizeof(int)], len - sizeof(int));
			deserialized = MDSplus::deserialize(newSerialized);
		    }
		    MDSplus::deleteData(serialized);
		    deserialized = convertApd(deserialized);
		    int serSize;
		    MDSplus::Apd *currApd = (MDSplus::Apd *)deserialized;
		    if(currApd->clazz != CLASS_APD)
		    {
		        std::cout << "Internal Error: expected APD 1" << std::endl;
		        return 0;
		    }
		    if (((MDSplus::Apd *)currApd)->len() < 2)
		    {
			std::cout << "Warning: Empty AoS:" <<deserialized << std::endl;
			return 0;
		    }
//std::cout << "1" << std::endl;
		    currApd = (MDSplus::Apd *)currApd->getDescAt(1);
		    if(currApd->clazz != CLASS_APD)
		    {
		        std::cout << "Internal Error: expected APD 2" << std::endl;
		        return 0;
		    }
//std::cout << "2" << std::endl;
		    if (((MDSplus::Apd *)currApd)->len() < 1)
		    {
			std::cout << "Warning: Empty AoS 1:" << deserialized << std::endl;
			return 0;
		    }
		    currApd = (MDSplus::Apd *)currApd->getDescAt(0);
		    if(currApd->clazz != CLASS_APD)
		    {
		        std::cout << "Internal Error: expected APD 3" << std::endl;
		        return 0;
		    }
//std::cout << "3" << std::endl;
		    double time;
		    if(getTime(currApd, time))
			times.push_back(time);
		    char *convSer = currApd->serialize(&serSize);
		    unsigned char *serAndLen = new unsigned char[sizeof(int)+serSize];
		    *(int *)serAndLen = serSize;
		    memcpy(&serAndLen[sizeof(int)], convSer, serSize);
		    MDSplus::Data *convSerialized = new MDSplus::Uint8Array(serAndLen, serSize+sizeof(int));
		    delete[] convSer;
		    delete [] serAndLen;
		    char segExpr[256];
//		    sprintf(segExpr, "data(build_path(\'%s:time0\'))[%d]",argv[4], segStart);
		    sprintf(segExpr, "data(build_path(\'%s:time\'))[%d]",argv[4], segStart);
		    MDSplus::Data *currSegStart = MDSplus::compile(segExpr, outT); 
//		    sprintf(segExpr, "data(build_path(\'%s:time0\'))[%d:%d]",argv[4], segStart, segStart);
		    sprintf(segExpr, "data(build_path(\'%s:time\'))[%d:%d]",argv[4], segStart, segStart);
		    MDSplus::Data *currSegDim = MDSplus::compile(segExpr, outT); 
//std::cout << currSegStart << "  " << currSegDim   << std::endl;
		    outN->makeSegment(currSegStart, currSegStart, currSegDim, (MDSplus::Array *)convSerialized);

//std::cout << "SEGMENT DONE" << std::endl;
		    MDSplus::deleteData(currSegStart);
		    MDSplus::deleteData(currSegDim);
		    MDSplus::deleteData(convSerialized);

		    double *timebase = new double[times.size()];
		    for(int i = 0; i < (int) times.size(); i++)
	    		timebase[i] = times[i];
		     MDSplus::Data *timebaseD = new MDSplus::Float64Array(timebase, times.size());
		    std::string timebasePath(argv[4]);
		     timebasePath += ":time";
		    MDSplus::TreeNode *timebaseNode = outT->getNode(timebasePath.c_str());
		    timebaseNode->putData(timebaseD);
		    MDSplus::deleteData(timebaseD);
		    delete [] timebase;
		}
		else
		{
		    int currPos = 0;
		    int currLen;
		    int serLen;
		    char *serBytes = serialized->getByteArray(&serLen);
		    for(int segIdx = 0; segIdx < numDims; segIdx++)
		    {
//std::cout << "SEG IDX " << segIdx << "  OF  "<< numDims << std::endl;
			currLen = *(int *)&serBytes[currPos];
			MDSplus::Data *currSerialized = new MDSplus::Uint8Array((const unsigned char *)&serBytes[currPos + sizeof(int)], currLen);
			currPos += sizeof(int)+currLen;
		        MDSplus::Data *deserialized = MDSplus::deserialize(currSerialized);
		    	MDSplus::deleteData(currSerialized);
		    	deserialized = convertApd(deserialized);
		    	int serSize;
		    	MDSplus::Apd *currApd = (MDSplus::Apd *)deserialized;
		    	if(currApd->clazz != CLASS_APD)
		    	{
		            std::cout << "Internal Error: expected APD 1" << std::endl;
		            return 0;
		    	}
		        currApd = (MDSplus::Apd *)currApd->getDescAt(1);
		        if(currApd->clazz != CLASS_APD)
		        {
		            std::cout << "Internal Error: expected APD 2" << std::endl;
		            return 0;
		        }
		        currApd = (MDSplus::Apd *)currApd->getDescAt(0);
		        if(currApd->clazz != CLASS_APD)
		        {
		            std::cout << "Internal Error: expected APD 3" << std::endl;
		            return 0;
		    	}
		    	double time;
		    	if(getTime(currApd, time))
			    times.push_back(time);
		    	char *convSer = currApd->serialize(&serSize);

//Gabriele Jan 2019. Now the slice len is ALWAYS put in front of serialized slice
		        unsigned char *serAndLen = new unsigned char[sizeof(int)+serSize];
		        *(int *)serAndLen = serSize;
		        memcpy(&serAndLen[sizeof(int)], convSer, serSize);
		        MDSplus::Data *convSerialized = new MDSplus::Uint8Array(serAndLen, serSize+sizeof(int));
		        delete [] serAndLen;
		    	delete[] convSer;
		    	char segExpr[256];
//		    	sprintf(segExpr, "data(build_path(\'%s:time0\')[%d])",argv[4], segStart+segIdx);
		    	sprintf(segExpr, "data(build_path(\'%s:time\'))[%d]",argv[4], segStart+segIdx);
//std::cout << "SEG IDX 6" << segIdx << "  OF  "<< numDims << std::endl;
		    	MDSplus::Data *currSegStart = MDSplus::compile(segExpr, outT); 
//		    	sprintf(segExpr, "data(build_path(\'%s:time0\')[%d:%d])",argv[4], segStart+segIdx, segStart+segIdx);
		    	sprintf(segExpr, "data(build_path(\'%s:time\'))[%d:%d]",argv[4], segStart+segIdx, segStart+segIdx);
		    	MDSplus::Data *currSegDim = MDSplus::compile(segExpr, outT); 
//std::cout << "SEG IDX 7" << segIdx << "  OF  "<< numDims << "   " << currSegStart << "  " << currSegDim   << "  " << time << std::endl;
		    	outN->makeSegment(currSegStart, currSegStart, currSegDim, (MDSplus::Array *)convSerialized);
//std::cout << "SEG IDX 8" << segIdx << "  OF  "<< numDims << std::endl;
		    	MDSplus::deleteData(currSegStart);
		    	MDSplus::deleteData(currSegDim);
		    	MDSplus::deleteData(convSerialized);
		    }
		    double *timebase = new double[times.size()];
		    for(int i = 0; i < (int) times.size(); i++)
	    		timebase[i] = times[i];
		     MDSplus::Data *timebaseD = new MDSplus::Float64Array(timebase, times.size());
		    std::string timebasePath(argv[4]);
		     timebasePath += ":time";
		    MDSplus::TreeNode *timebaseNode = outT->getNode(timebasePath.c_str());
		    timebaseNode->putData(timebaseD);
		    MDSplus::deleteData(timebaseD);
		    delete [] timebase;

		}
	}
    }catch (MDSplus::MdsException &exc)
    {
	std::cout << "Error serializing stuff: " << exc.what() << std::endl;
    }
    return 0;
}
	

      
