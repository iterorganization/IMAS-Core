#define NODENAME_MANGLING

#include <cstring>
#include <string.h>
#include <pthread.h>
#include <boost/filesystem.hpp>

#include "mdsplus_backend.h"

using namespace boost::filesystem;

//Version definition
#define MDSPLUS_BACKEND_MAJOR 1
#define MDSPLUS_BACKEND_MINOR 1



//#define MDSPLUS_SEGMENT_SIZE 4192
#define MDSPLUS_SEGMENT_SIZE 67072
//#define MDSPLUS_SEGMENT_SIZE 134144

#define ERROR_MESSAGE_LEN 10000
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define SEPARATORS "/.:[]"  /* separators for cpoPath */
// User for passing path to MDSplus which are limited to 12 char
// Keep the integrity of the first 8 characters
// Then sum the ASCII codes of the other characters and store them in the name
// SliceApd

#define DEF_TREENAME		"ids"
#define DEF_NORMALMODE		"NORMAL"
#define DEF_READONLYMODE	"READONLY"

//Original ids_path
static std::string originalIdsPath = "";


static void saveVersion(MDSplus::Tree *t)
{
    try {
	MDSplus::TreeNode *nMajor = t->getNode("VERSION:BACK_MAJOR");
	MDSplus::TreeNode *nMinor = t->getNode("VERSION:BACK_MINOR");
	MDSplus::Int32 *majorD = new MDSplus::Int32(MDSPLUS_BACKEND_MAJOR);
	MDSplus::Int32 *minorD = new MDSplus::Int32(MDSPLUS_BACKEND_MINOR);
	nMajor->putData(majorD);
	nMinor->putData(minorD);
	MDSplus::deleteData(majorD);
	MDSplus::deleteData(minorD);
	delete nMajor;
	delete nMinor;
    } catch(MDSplus::MdsException &exc)
    {
	throw UALBackendException("Cannot save Backend version in new pulse",LOG);
    }
}

#ifdef NODENAME_MANGLING

static size_t getMaxApdArrayDim(MDSplus::Apd *inApd)
{
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;
    size_t maxDim = 0;
    inApd->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_APD)
    {
	std::cout << "INTERNAL ERROR in getMaxApdArrayDim: not an APD" << std::endl;
	return 0;
    }
    bool isArray = true;
    if(inApd->getDescAt(0))
    {
    	inApd->getDescAt(0)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	if(clazz == CLASS_S && dtype == DTYPE_T)
	    isArray = false;
    }

    size_t notNull = 0;
    for(size_t i = 0; i < inApd->len(); i++)
    {
	if(inApd->getDescAt(i))
	{
	    notNull++;
   	    inApd->getDescAt(i)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    if(clazz == CLASS_APD)
    	    {
		size_t currDim = getMaxApdArrayDim((MDSplus::Apd *)inApd->getDescAt(i));
		if(currDim > maxDim)
		    maxDim = currDim;
	    }
	}
    }
    if(isArray && notNull > maxDim)
	maxDim = notNull;
    return maxDim;
}


static bool sameApdStructure(MDSplus::Apd *apd1, MDSplus::Apd *apd2)
{
    unsigned char clazz1, clazz2;
    unsigned char dtype1, dtype2;
    short length1, length2;
    char nDims1, nDims2;
    int *dims1, *dims2;
    void *ptr1, *ptr2;
    if(!apd1 && !apd2) 
        return true;
    if(!apd1 || ! apd2)
	return false;
    if(apd1->len() != apd2->len())
	return false;
    if(apd1->len() == 0)
	return true;
    apd1->getInfo((char *)&clazz1, (char *)&dtype1, &length1, &nDims1, &dims1, &ptr1);
    apd2->getInfo((char *)&clazz2, (char *)&dtype2, &length2, &nDims2, &dims2, &ptr2);
    if(clazz1 != clazz2 || dtype1 != dtype2)
	return false;
    if(clazz1 != CLASS_APD) 
    {
	std::cout << "INTERNAL ERROR in sameApdStructure: not an Apd" << std::endl;
	return false;
    }
    MDSplus::Data *data1 = apd1->getDescAt(0);
    MDSplus::Data *data2 = apd2->getDescAt(0);
    if(data1)
    {
	if(!data2) return false;
        data1->getInfo((char *)&clazz1, (char *)&dtype1, &length1, &nDims1, &dims1, &ptr1);
        data2->getInfo((char *)&clazz2, (char *)&dtype2, &length2, &nDims2, &dims2, &ptr2);
        if(clazz1 != clazz2 || dtype1 != dtype2)
	    return false;
        if(clazz1 == CLASS_S && dtype1 == DTYPE_T) //It is a Struct
        {
   	    char *field1 = data1->getString();
    	    char *field2 = data2->getString();
    	    int notSame = strcmp(field1, field2);
    	    delete[] field1;
    	    delete[] field2;
    	    if(notSame != 0)
		return false;
    	    for(size_t i = 1; i < apd1->len(); i++)
    	    {
		data1 = apd1->getDescAt(i);
		data2 = apd2->getDescAt(i);
		if(!data1 && !data2)
	    	    continue;
		if(!data1 || !data2)
	    	    return false;
    		data1->getInfo((char *)&clazz1, (char *)&dtype1, &length1, &nDims1, &dims1, &ptr1);
    		data2->getInfo((char *)&clazz2, (char *)&dtype2, &length2, &nDims2, &dims2, &ptr2);
		if(clazz1 != clazz2 || dtype1 != dtype2)
	            return false;
		if(clazz1 == CLASS_APD)
		{
	    	    if(!sameApdStructure((MDSplus::Apd *)data1, (MDSplus::Apd *)data2))
		    return false;
		}
		if(clazz1 == CLASS_A) //Arrays, must have the same shape
		{
	    	    int *shape1, *shape2;
	    	    int numDims1, numDims2;
	    	    shape1 = data1->getShape(&numDims1);
	    	    shape2 = data2->getShape(&numDims2);
	    	    if(numDims1 != numDims2)
	    	    {
			delete[] shape1;
			delete[] shape2;
			return false;
	    	    }
	    	    for(int j = 0; j < numDims1; j++)
	    	    {
			if(shape1[j] != shape2[j])
		 	{
	    	    	    delete[] shape1;
	    	    	    delete[] shape2;
  		    	    return false;
			}
	    	    }
	    	    delete[] shape1;
	    	    delete[] shape2;
		}
	    }
	    return true; //Struct case finished
	}
    }
//If we arrive here it is an Apd Array
    for(size_t i = 0; i < apd1->len(); i++)
    {
	if(!apd1->getDescAt(i) && !apd2->getDescAt(i))
	    continue;
	if(!apd1->getDescAt(i) || !apd2->getDescAt(i))
	    return false;
        apd1->getDescAt(i)->getInfo((char *)&clazz1, (char *)&dtype1, &length1, &nDims1, &dims1, &ptr1);
        apd2->getDescAt(i)->getInfo((char *)&clazz2, (char *)&dtype2, &length2, &nDims2, &dims2, &ptr2);
	if(clazz1 != CLASS_APD || clazz2 != CLASS_APD)
    	{
	    std::cout << "INTERNAL ERROR in sameApdStructyre: not an Apd" << std::endl;
	    return false; 
	}
	if(!sameApdStructure((MDSplus::Apd *)apd1->getDescAt(i), (MDSplus::Apd *)apd2->getDescAt(i)))
	    return false;
    }
    return true;
}

static bool sameApdStructure(MDSplus::Apd *apd)
{
    if(apd->len() < 2) return false;

    for(size_t i = 1; i < apd->len(); i++)
    {
	if(!apd->getDescAt(i)) return false;
	if(!sameApdStructure((MDSplus::Apd *)apd->getDescAt(0), (MDSplus::Apd *)apd->getDescAt(i)))
	    return false;
    }
    return true;
}


static MDSplus::Data *mergeApdFields(std::vector<MDSplus::Data *>dataVect)
{
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;

    MDSplus::Data *retArr;
    dataVect[0]->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_S && clazz != CLASS_A)
    {
	std::cout << "INTERNAL ERROR in mergeApdFields: inconsistent field types" << std::endl;
	return NULL; //Force core dump
    }
    if(clazz == CLASS_S)
    {
	char *buf = new char[length * dataVect.size()];
	int dataLen = length;
	for(size_t i = 0; i < dataVect.size(); i++)
	{
       	    dataVect[i]->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    memcpy(&buf[i * dataLen], ptr, dataLen);
	}
	switch(dtype) {
	    case DTYPE_L: 
		retArr = new MDSplus::Int32Array((int *)buf, dataVect.size());
		break;
	    case DTYPE_DOUBLE: 
		retArr = new MDSplus::Float64Array((double *)buf, dataVect.size());
		break;
	    case DTYPE_FTC: 
		retArr = new MDSplus::Complex64Array((double *)buf, dataVect.size());
		break;
	    default:
		std::cout << "INTERNAL ERROR in mergeApdFields: inconsistent field type" << std::endl;
		return NULL; //Force core dump
	}
    }
    else //CLASS_A 
    {
	int dataLen = length;
	int numElements = 1;
	for(int i = 0; i < nDims; i++)
	    numElements *= dims[i];
	//if(dtype != DTYPE_B && dtype != DTYPE_BU)
	{
	    char *buf = new char[dataLen * numElements * dataVect.size()];
	    for(size_t i = 0; i < dataVect.size(); i++)
	    {
    	    	dataVect[i]->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    	memcpy(&buf[i * dataLen * numElements], ptr, dataLen * numElements);
	    }
	    int *newDims = new int[nDims +1];
	    newDims[0] = dataVect.size();
	    for(int i = 0; i < nDims; i++)
	    	newDims[i+1]= dims[i];
	
            switch(dtype) {
	    	case DTYPE_B: 
	    	case DTYPE_BU: 
		    retArr = new MDSplus::Int8Array((char *)buf, nDims+1, newDims);
		    break;
	    	case DTYPE_L: 
		    retArr = new MDSplus::Int32Array((int *)buf, nDims+1, newDims);
		    break;
	    	case DTYPE_DOUBLE: 
		    retArr = new MDSplus::Float64Array((double *)buf, nDims+1, newDims);
		    break;
	    	case DTYPE_FTC: 
		    retArr = new MDSplus::Complex64Array((double *)buf, nDims+1, newDims);
		    break;
		default:
		    std::cout << " INTERNAL ERROR in mergeApdFields: unexpected type" << std::endl;
		    return NULL;
	    }
	    delete[] newDims;
	    delete [] buf;
	}
	/*else
    	{
	    if(nDims > 1)
	    {
		std::cout << "INTERNAL ERROR in mergeApdFields: STRING ARRAYS NOT SUPPORTED IN APD" << std::endl;
		return NULL;
	    }
	    int numElements = 1;
	    for(int i = 0; i < nDims; i++)
	    	numElements *= dims[i];



	    char **bufStr = new char *[dataVect.size()];
	    for(size_t i = 0; i < dataVect.size(); i++)
	    {
		int len;
    	    	char *currBuf = dataVect[i]->getByteArray(&len);
		bufStr[i] = new char[len+1];
		memcpy(bufStr[i], currBuf, len);
		bufStr[i][len] = 0;
		delete[]currBuf;
	    }
	    retArr = new MDSplus::StringArray(bufStr, dataVect.size());
	    for(size_t i = 0; i < dataVect.size(); i++)
	    {
    	    	delete [] bufStr[i];
	    }
	    delete [] bufStr;
	}
*/
    }
    return retArr;
}

static MDSplus::Apd *mergeApdArrays(std::vector<MDSplus::Data *>apdVect)
{
//It is assumed that the two input structures are already consistent, i.e. checked by SameApdStructure()
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;

    apdVect[0]->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_APD) 
    {
	std::cout << "INTERNAL ERROR in mergeApdArrays: Not an Apd" << std::endl;
	return  NULL; //Force core dump
    }
    MDSplus::Apd *retApd = new MDSplus::Apd();
    if(((MDSplus::Apd *)apdVect[0])->getDescAt(0)) //If not empty
    	((MDSplus::Apd *)apdVect[0])->getDescAt(0)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    else
	clazz = CLASS_APD; //Struct fields always begin with a non APD, so if it is empty we assume it is an Apd array
    if(clazz == CLASS_APD)  //Juts Apd array
    {    
	for(size_t i = 0; i < ((MDSplus::Apd *)apdVect[0])->len(); i++)
	{
	    if(!((MDSplus::Apd *)apdVect[0])->getDescAt(i)) //Empty element
		retApd->setDescAt(i, NULL);
	    else
	    {
	    	std::vector<MDSplus::Data *> currVect;
	    	for(size_t j = 0; j < apdVect.size(); j++)
	    	{
		    currVect.push_back(((MDSplus::Apd *)apdVect[j])->getDescAt(i));
	    	}
	    	retApd->setDescAt(i, mergeApdArrays(currVect));
	    }
	}
	return retApd;
    }
    if(clazz != CLASS_S || dtype != DTYPE_T)
    {
	std::cout << "INTERNAL ERROR in mergeApdArrays: inconsistent struct types" << std::endl;
	return NULL;
    }
    char *fieldName = ((MDSplus::Apd *)apdVect[0])->getDescAt(0)->getString();
    retApd->setDescAt(0, new MDSplus::String(fieldName));
//    delete [] fieldName;
    for(size_t i = 1; i < ((MDSplus::Apd *)apdVect[0])->len(); i++)
    {
 	MDSplus::Data *data = ((MDSplus::Apd *)apdVect[0])->getDescAt(i);
	if(!data)
	    retApd->setDescAt(i, NULL);
	else
	{	
   	    std::vector<MDSplus::Data *> currVect;
	    for(size_t j = 0; j < apdVect.size(); j++)
	    {
	        currVect.push_back(((MDSplus::Apd *)apdVect[j])->getDescAt(i));
	    }
	    data->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    if(clazz == CLASS_APD)
	    {
		retApd->setDescAt(i, (MDSplus::Data *)mergeApdArrays(currVect));
	    }
	    else //Just field
	    {
//std::cout << "MERGE APD FIELDS " << fieldName << "   " << currVect.size() << std::endl;
		retApd->setDescAt(i, mergeApdFields(currVect));
	    }
	}
    }
    delete [] fieldName;
    return retApd;
}

   
	
static void deflateApd(MDSplus::Apd *inApd)
{
    if(!inApd || inApd->len() == 0) return;
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;
    MDSplus::Data *currData = inApd->getDescAt(0);
    if(currData)
    	currData->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    else
	clazz = CLASS_APD; //It is in any case an array of Apd
    if(clazz == CLASS_APD) //Array 
    {
	size_t maxInnerDim = getMaxApdArrayDim(inApd);
	if(sameApdStructure(inApd) && inApd->len() >= maxInnerDim) //If all elements of this array have the same structure
	{
//std::cout << "COMPRESSING " << inApd->len() << "  elements" << std::endl;
//Coded into a single Structure qith one dimension more for leaves. The first descriptor is the number of elements (added dimension)
	    std::vector<MDSplus::Data *> apdVect;
	    for(size_t i = 0; i < inApd->len(); i++)
		apdVect.push_back(inApd->getDescAt(i));
	    MDSplus::Apd * mergedApd = mergeApdArrays(apdVect);
	    for(size_t i = 0; i < inApd->len(); i++)
	    {
	    	MDSplus::deleteData(inApd->getDescAt(i));
		inApd->getDscs()[i] = NULL;
	    }
	    inApd->getDscs()[0] = new MDSplus::Uint32(inApd->len());
	    inApd->getDscs()[1] = mergedApd;
	    return; //All done
	}
	for(size_t i = 0; i < inApd->len(); i++)
	{
	    deflateApd((MDSplus::Apd *)inApd->getDescAt(i));	
	}
    }
    else  //Must be a string for field name
    {
	if(clazz != CLASS_S || dtype != DTYPE_T)
	{
	    std::cout << "INTERNAL ERROR in deflateApd: not an String descriptor" << std::endl;
	    return;
	}
	for(size_t i = 1; i < inApd->len(); i++)
	{
    	    MDSplus::Data *data = inApd->getDescAt(i);
	    if(!data) continue;
    	    data->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    if(clazz == CLASS_APD)
	    {
		deflateApd((MDSplus::Apd *)data);
	    }
	}
    }
}


static std::vector<MDSplus::Data *> splitApdFields(MDSplus::Data *inData, int numInstances)
{
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;

    inData->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_A)
    {
	std::cout << "INTERNAL ERROR in splitApdFields: not an Array" << std::endl;

    }
    std::vector<MDSplus::Data *> retVect;
    if(nDims == 1)
    {
 	if(dims[0] != numInstances)
	{
	    std::cout << "INTERNAL ERROR in splitApdFields: invalid dimesion" << std::endl;

	}
     	switch(dtype)  {
	    case DTYPE_L:
	    { 
	    	int *buf = (int *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Int32(buf[i]));
	    	}
	        break;
	    }
	    case DTYPE_DOUBLE:
	    { 
	    	double *buf = (double *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Float64(buf[i]));
	    	}
	        break;
	    }
	    case DTYPE_FTC:
	    { 
	    	double *buf = (double *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Complex64(buf[2*i], buf[2*i+1]));
	    	}
	        break;
	    }
	    default:
		std::cout << "INTERNAL ERROR in mergeApdFields: inconsistent array field type" << std::endl;

	}
    }
    else //nDims > 1
    {
	int origSize = 1;
	for(int j = 1; j < nDims; j++)
	    origSize *= dims[j];
    	switch(dtype)  {
	    case DTYPE_B:
	    case DTYPE_BU:
	    { 
	    	char *buf = (char *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Int8Array(&buf[i*origSize], nDims - 1, &dims[1]));
	    	}
	        break;
	    }
	    case DTYPE_L:
	    { 
	    	int *buf = (int *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Int32Array(&buf[i*origSize], nDims - 1, &dims[1]));
	    	}
	        break;
	    }
	    case DTYPE_DOUBLE:
	    { 
	    	double *buf = (double *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Float64Array(&buf[i*origSize], nDims - 1, &dims[1]));
	    	}
	        break;
	    }
	    case DTYPE_FTC:
	    { 
	    	double *buf = (double *)ptr; 
	    	for(int i = 0; i < numInstances; i++)
	    	{	
		    retVect.push_back(new MDSplus::Complex64Array(&buf[2*i*origSize], nDims - 1, &dims[1]));
	    	}
	        break;
	    }
	    default:
		std::cout << "INTERNAL ERROR in mergeApdFields: inconsistent array field type" << std::endl;
	}
    }
    return retVect;
}

static std::vector<MDSplus::Apd *> splitApdArrays(MDSplus::Apd *inApd, int numInstances)
{
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;

    std::vector<MDSplus::Apd *> retVect;
    inApd->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_APD)
    {
	std::cout << "INTERNAL ERROR in splitApdArrays: not an Apd" << std::endl;
	return retVect;
    }
//Prepare retVect
    for(int i = 0; i < numInstances; i++)
	retVect.push_back(new MDSplus::Apd());

    //An Array may have empty components, in any case a struct starts with the name
    if(inApd->getDescAt(0))
    {
	inApd->getDescAt(0)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    	if(clazz == CLASS_S && dtype == DTYPE_T) //It is a Struct
    	{
	    char *fieldName = inApd->getDescAt(0)->getString();
	    for(int i = 0; i < numInstances; i++)
	    {
	    	retVect[i]->setDescAt(0, new MDSplus::String(fieldName));
	    }
	   //delete [] fieldName;
            for (size_t i = 1; i < inApd->len(); i++)
	    {
	    	MDSplus::Data *currField = inApd->getDescAt(i);
	    	if(!currField)
	    	{
		    for(int j = 0; j < numInstances; j++)
		    	retVect[j]->setDescAt(i, NULL);
	        }
	        else
	        {
    		    currField->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
		    if(clazz == CLASS_APD) 
		    {
		    	std::vector<MDSplus::Apd *>currVect = splitApdArrays((MDSplus::Apd *)currField, numInstances);
	    		if(currVect.size() != (size_t)numInstances)
   	    		{
			    std::cout << "INTERNAL ERROR in splitApdArrays: incorrect vector sizes" << std::endl;
			    return retVect;
    	    		}
//std::cout << "START SETDESC " << numInstances << std::endl;
		    	for(int j = 0; j < numInstances; j++)
		 	    retVect[j]->setDescAt(i, currVect[j]);
//std::cout << "END SETDESC " << numInstances <<  std::endl;
		    }
		    else //Field
		    {
		    	std::vector<MDSplus::Data *>currVect = splitApdFields(currField, numInstances);
//std::cout << "SPLIT APD FIELDS " << fieldName << "    " << currVect.size() << std::endl;
	    		if(currVect.size() != (size_t)numInstances)
   	    		{
			    std::cout << "INTERNAL ERROR in splitApdArrays: incorrect vector sizes" << std::endl;
			    return retVect;
    	    		}
//std::cout << "START SETDESC " << numInstances << std::endl;
		    	for(int j = 0; j < numInstances; j++)
		 	    retVect[j]->setDescAt(i, currVect[j]);
//std::cout << "END SETDESC " << numInstances <<  std::endl;
		    }
		}
	    }
	    delete [] fieldName;
	    return retVect;
	}
    }
   //If we arrive here, it is an Array of Apds
    for(size_t i = 0; i < inApd->len(); i++)
    {
	MDSplus::Apd *currApd = (MDSplus::Apd *)inApd->getDescAt(i);
	if(currApd)
	{
	    std::vector<MDSplus::Apd *>currVect = splitApdArrays(currApd, numInstances);
	    if(currVect.size() != (size_t)numInstances)
   	    {
		std::cout << "INTERNAL ERROR in splitApdArrays: incorrect vector sizes" << std::endl;
		return retVect;
    	    }

	    for(int j = 0; j < numInstances; j++)
		retVect[j]->setDescAt(i, currVect[j]);
	}
	else
	{
	    for(int j = 0; j < numInstances; j++)
		retVect[j]->setDescAt(i, NULL);
	}
    }
    return retVect;
}


static void inflateApd(MDSplus::Apd *inApd)
{
    if(!inApd) return;

    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;

    inApd->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    if(clazz != CLASS_APD) //Array 
    {
	std::cout << "INTERNAL ERROR in inflateApd: not an ADP descriptor" << std::endl;
	return;
    }
    if(inApd->len() == 0)
	return;
    if(inApd->len() > 0 && inApd->getDescAt(0))
    {
    	inApd->getDescAt(0)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	if(clazz == CLASS_S && dtype == DTYPE_LU) //Coded deflated
	{
	    int numInstances = inApd->getDescAt(0)->getInt();
	    if(inApd->len() < 2 || !inApd->getDescAt(1))
    	    {
		std::cout << "INTERNAL ERROR in inflateApd: missing compressed info" << std::endl;
		return;
    	    }
	    inApd->getDescAt(1)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    if(clazz != CLASS_APD)
   	    {
		std::cout << "INTERNAL ERROR in inflateApd: incorrect comressed info " << std::endl;
		return;
    	    }
	    std::vector<MDSplus::Apd *>apdVect = splitApdArrays((MDSplus::Apd *)inApd->getDescAt(1), numInstances);
	    MDSplus::deleteData(inApd->getDescAt(0));
	    MDSplus::deleteData(inApd->getDescAt(1));
	    inApd->getDscs()[0] = NULL;
	    inApd->getDscs()[1] = NULL;
//std::cout << "PARTE INFLATE SET DESC"  << numInstances << std::endl;
	    for(int i = 0; i < numInstances; i++)
	    {
		if(inApd->len() > (size_t)i)
		    inApd->getDscs()[i] = apdVect[i];
		else
		    inApd->setDescAt(i, apdVect[i]);
	    }
//std::cout << "FINISCE INFLATE SET DESC" << std::endl;
	    return; //All done
	}
    }
//Recursive propagation
    for(size_t i = 0; i < inApd->len(); i++)
    {
	if(inApd->getDescAt(i))
	{
	    inApd->getDescAt(i)->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
	    if(clazz == CLASS_APD)
		inflateApd((MDSplus::Apd *)inApd->getDescAt(i));
	}
    }
}

 


/*static char *path2MDS(char *path)
{
    static char MDSpath[13];
    char SUM[10];
    int i, sum;
    int len = strlen(path);

//    printf("Begin path2MDSpath = %s\n",path);
    strncpy(MDSpath,path,8);
    MDSpath[8] = '\0';
    sum = 0;
    for(i = 8; i < len; i++) {
	sum += tolower(path[i]);
    }
    sprintf(SUM,"%d",sum);
    strncat(MDSpath,SUM,4);
    for (i=0;i<12;i++) {
	MDSpath[i] = tolower(MDSpath[i]);
    }
//    printf("path = %s, MDSpath = %s\n",path, MDSpath);
    return  MDSpath;
}
*/ 
static char *path2MDS(char *path)  //Unlimited nodename length
{
    static char MDSpath[512];
    std::string inPath(path);
    if(inPath.length() > 12)
    {
   	std::string inPath(path);
    	std::string outPath = "";
    	while(inPath.length() > 12)
    	{
	    outPath += ".";
	    outPath += inPath.substr(0,12);
	    inPath = inPath.substr(12);
     	}
    	outPath += ".";
	outPath += inPath;
//std::cout << "OutPath: " << outPath << std::endl;
	strcpy(MDSpath, outPath.substr(1).c_str());
	return MDSpath;
    }
    strcpy(MDSpath, path);
    return MDSpath;
}

static std::string mdsconvertPath(const char *inputpath, bool isAos = false
)
{

    static char mdsCpoPath[2048];
    char  path[2048];
    char *ptr;
    int   i;

    strcpy(path,inputpath);
    ptr=strtok(path, SEPARATORS);
    if (ptr == NULL) return "";
    strcpy(mdsCpoPath,"");
    if (!isdigit(*ptr)) {
        if(isAos && (!strncmp((const char *)ptr, "timed_", 6) || !strncmp((const char *)ptr, "item_", 5) || !strcmp((const char *)ptr, "static") || !strcmp((const char *)ptr, "time") || !strcmp((const char *)ptr, "aos")))      
	    strcat(mdsCpoPath,ptr);
	else
      	    strcat(mdsCpoPath,path2MDS(ptr));
    }
    else {
      strcat(mdsCpoPath,ptr);
    }
    while ( (ptr=strtok(NULL, SEPARATORS)) != NULL) {
	strcat(mdsCpoPath,".");
	if (!isdigit(*ptr)) {
	    if(isAos && (!strncmp((const char *)ptr, "timed_", 6) || !strncmp((const char *)ptr, "item_", 5) || !strcmp((const char *)ptr, "static") || !strcmp((const char *)ptr, "time") || !strcmp((const char *)ptr, "aos")))
	        strcat(mdsCpoPath,ptr);
 	    else
                strcat(mdsCpoPath,path2MDS(ptr));
	}
	else {
	  strcat(mdsCpoPath,ptr);
	}
    }
    for(i = strlen(mdsCpoPath) - 1; i >=0; i--){
	if (mdsCpoPath[i] == '.'){
	  mdsCpoPath[i] = ':';
	  break;
	}
    }
    //printf("mds+.mdsconvertPath. modified path: %s\n",mdsCpoPath);
    std::string retStr(mdsCpoPath);
    return retStr;
}

static std::string checkFullPath(std::string &inStr, bool isAos = false)
{
    return mdsconvertPath(inStr.c_str(), isAos);
}
#else
static std::string mdsconvertPath(const char *path)
{
    std::string retStr(path);
   for(int i = 0; i < (int)retStr.length(); i++)
    {
	if(retStr[i] == '/')
	retStr[i] = '.';
    }
    for(int i = (int)retStr.length() - 1; i >= 0; i--)
    {
	if(retStr[i] == '.')
	{
	retStr[i] = ':';
	break;
	}
    }
    return retStr;	
}

static std::string checkFullPath(std::string &inStr)
{
    std::string outStr("");
    std::string currStr; 
    size_t currPos, prevPos = 0;
    while(true)
    {
	currPos = inStr.find_first_of(":.", prevPos);
	if (currPos == std::string::npos) //No more subtrees
	{
	    currStr = inStr.substr(prevPos);
	    if(currStr.length() > 12)
		currStr.resize(12);
	    outStr += currStr;
	    return outStr;
	}
	currStr = inStr.substr(prevPos, currPos - prevPos);
	if(currStr.length() > 12)
	    currStr.resize(12);
	outStr += (currStr + inStr[currPos]);
	prevPos = currPos + 1;
    }
}

#endif

static void skipTabs(int tabs)
{
    for(int i = 0; i < tabs; i++)
	std::cout<<"    ";
}

static void dumpStruct(MDSplus::Apd *apd, int tabs);
static void dumpArrayStruct(MDSplus::Apd *apd, int tabs)
{
    if(!apd) return;
    for(size_t i = 0; i < apd->len(); i++)
    {
        skipTabs(tabs);
        std::cout << "["<<i<<"]:\n";
	if(!apd->getDescAt(i))
	{
	  skipTabs(tabs+1);
	  std::cout << "None\n";
	}
	else  
	  dumpStruct((MDSplus::Apd *)apd->getDescAt(i), tabs+1);
    }
}
static void dumpStruct(MDSplus::Apd *apd, int tabs)
{
    if(!apd) return;
    skipTabs(tabs);
    std::cout << apd->getDescAt(0) << ":\n";
    for(size_t i = 1; i < apd->len(); i++)
    {
	
	if(!apd->getDescAt(i))
	{
	    skipTabs(tabs+1);
	    std::cout << "None\n";
	}
	else
	{
	    if(apd->getDescAt(i)->clazz == CLASS_APD)
	    {
	      //Can be either a structure or an array of structures
		MDSplus::Apd *currApd = (MDSplus::Apd *)apd->getDescAt(i);
		if(currApd->len() == 0)
		{
		    skipTabs(tabs+1);
		    std::cout << "None\n";
		}
		else if(currApd->getDescAt(0)->clazz == CLASS_S)
		  dumpStruct((MDSplus::Apd *)apd->getDescAt(i), tabs+1);
		else
		  dumpArrayStruct((MDSplus::Apd *)apd->getDescAt(i), tabs+1);
	    }
	    else
	    {
		skipTabs(tabs+1);
		std::cout << apd->getDescAt(i) << std::endl;
	    }
	}
    }
}


 
//Return the dimension in bytes of the first strring stored in seggmented data. If no segments return -1
static int getStringSizeInSegment(MDSplus::TreeNode *node)
{
    int numSegments = node->getNumSegments();
    if(numSegments == 0)
        return -1;
    char dtype, dimct;
    int dims[64], nextRow;
    node->getSegmentInfo(0, &dtype, &dimct, dims, &nextRow);
    return dims[0];
}


 
static std::string getSegmentData(std::string path, int segIdx)
{
    char segIdxBuf[16];
    sprintf(segIdxBuf, "%d", segIdx);
    return "(_=*;TreeShr->TreeGetSegment(val(getnci(build_path(\'"+path+"\'),\'NID_NUMBER\')),val("+segIdxBuf+"),xd(_),val(0));_;)";
}

    std::string MDSplusBackend::composePaths(std::string dataobjectPath, std::string path)
    {
    	std::string retStr = dataobjectPath;
	if(!path.empty())
	{
	    if(path[0] != '/')
	    	retStr += "/";
	    retStr += path;
	}
	for(int i = 0; i < (int)retStr.length(); i++)
	{
	    if(retStr[i] == '/')
	    	retStr[i] = '.';
	}
	for(int i = (int)retStr.length() - 1; i >= 0; i--)
	{
	    if(retStr[i] == '.')
	    {
	        retStr[i] = ':';
		break;
	    }
	}
	return retStr;	
    }


static char *getPathInfo(MDSplus::Data *data, MDSplus::TreeNode *refNode)
{
    unsigned char clazz;
    unsigned char dtype;
    short length;
    char nDims;
    int *dims;
    void *ptr;
//printf("CHIAMO GETINFO %p\n", data);
    if(!data) return (char *)"";
    data->getInfo((char *)&clazz, (char *)&dtype, &length, &nDims, &dims, &ptr);
    delete[] dims;
    switch (clazz) {
	case CLASS_S:
	    if(dtype == DTYPE_NID || dtype == DTYPE_PATH)
	    {
		MDSplus::TreeNode *currNode = (MDSplus::TreeNode *)data;
		static char *path = (char *)"";
		if(currNode->getNid() != refNode->getNid())
		    path = currNode->getPath();
		MDSplus::deleteData(currNode);
		return path;
	    }
	    return (char *)"";
	case CLASS_A:
	    return (char *)"";
	case CLASS_R:
	    MDSplus::Compound *compData = (MDSplus::Compound *)data;
	    for(int i = 0; i < compData->getNumDescs(); i++)
	    {
		MDSplus::Data *currDsc = compData->getDescAt(i);
		if(currDsc)
		{
		    char *retPath = getPathInfo(currDsc, refNode);
		    MDSplus::deleteData(currDsc); //Compound::getDescAt() increments ref counter!!!!!
		    if(*retPath) return retPath;
		}
	    }
    } 
    return (char *)"";
}


/////////////////////METHODS

    double *MDSplusBackend::getCachedTimebase(std::string timebasePath, int &nSamples)
    {
 	try {
	    std::vector<double> timebaseV = timebaseMap.at(timebasePath);
	    nSamples = timebaseV.size();
	    double *retData = new double[nSamples];
	    for(int i = 0; i < nSamples; i++)
		retData[i] = timebaseV[i];
	    return retData;
	}
 	catch (const std::out_of_range& oor) {}
	    return NULL;
    }

    void MDSplusBackend::updateCachedTimebase(std::string timebasePath, double *timebase, int nSamples)
    {
	std::vector<double> timebaseV;
	timebaseV.assign(timebase, timebase+nSamples);
	timebaseMap[timebasePath] = timebaseV;
    }


	

    int MDSplusBackend::getTimebaseIdx(std::string dataobjectPath, std::string timebase, double time)
    {




	try {
          double *times;
	  int datatype = 0;
          int numDims;
          int dims[64];

          int status = readData(tree, dataobjectPath, timebase, (void **)&times, &datatype, &numDims, dims);
	  if (datatype != ualconst::double_data)
	  {
	      printf("INTERNAL ERROR in getTimebaseIdx: unexpected timebase %s / %s data type: %d status: %d\n", dataobjectPath.c_str(), timebase.c_str(), datatype, status);
	      return 0;
	  }
	  if (numDims != 1 || dims[0] <= 0)
	  {
	      printf("INTERNAL ERROR in getTimebaseIdx: unexpected timebase dimension\n");
	      return 0;
	  }
	  int numTimes = dims[0];
	  if(time <= times[0])
	      return 0;
	  if(time >= times[numTimes - 1])
              return numTimes - 1;
	  int retIdx;
	  for(retIdx = 0; retIdx < numTimes - 1; retIdx++)
	      if(times[retIdx] <= time && times[retIdx + 1] >= time)
	          break;
          free(times);
	  return retIdx;
        }catch(MDSplus::MdsException& exc)
	{
	    printf("INTERNAL ERROR in getTimebaseIdx: %s\n", exc.what());
            return 0;
	}
    }

    int MDSplusBackend::getSegmentIdx(MDSplus::TreeNode *node, int timebaseIdx)
    {
        int actSlices = 0;
	int segIdx = 0;
	int numSegments = node->getNumSegments();
	char segNDims, segType;
	int  nextRow;
	int segDims[64];
	//printf("GET SEG IDX numSegments: %d\n", numSegments);
	for(segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    node->getSegmentInfo(segIdx, &segType, &segNDims, segDims, &nextRow);
	    if (segIdx < numSegments - 1)
		actSlices += segDims[0];
	    else
		actSlices += nextRow;
	    if(actSlices > timebaseIdx)
		break;
	}
	return segIdx;
    }

 
    double * MDSplusBackend::getSegmentIdxAndDim(MDSplus::TreeNode *node, std::string dataobjectPath, std::string timebase, double time, int &segIdx, int &nDim)
    {
          double *times;
	  int datatype = 0;
          int numDims;
          int dims[64];
	  int timebaseIdx;
	  char *timebasePath;
	  int numTimes;
	  bool timebaseCached = true;

	 //if timebase not passed (timed data of AoS, retrieve it from the dimension information of the first segment
	 if(timebase == "")
	 {

 	    try {
	        std::string timebasePathStr = timebasePathMap.at(node->getNid());
		timebasePath = new char[timebasePathStr.size() + 1];
		strcpy(timebasePath, timebasePathStr.c_str());
	    }
 	    catch (const std::out_of_range& oor) 
	    {
		try {
			MDSplus::Data *segDim = node->getSegmentDim(0); 
			timebasePath = getPathInfo(segDim, node);
			MDSplus::deleteData(segDim);
			timebasePathMap[node->getNid()] = std::string(timebasePath);
		}catch(MDSplus::MdsException &exc)
		{
			return NULL;
		}
	    }
         }
         else
	    timebasePath = NULL; 


	  if(timebasePath)
	      times = getCachedTimebase(timebasePath, numTimes);
	  else
	      times = getCachedTimebase(dataobjectPath+timebase, numTimes);

	  if(!times) 
	  {
	      timebaseCached = false;
	      int status = readData(tree, dataobjectPath, timebase, (void **)&times, &datatype, &numDims, dims, timebasePath);
	      if(!status) return 0;
	      if (datatype != ualconst::double_data)
	      {
	      	  printf("INTERNAL ERROR in getTimebaseIdx: unexpected timebase %s / %s data type: %d status: %d\n", dataobjectPath.c_str(), timebase.c_str(), datatype, status);
	          return 0;
	       }
	      if (numDims != 1 || dims[0] <= 0)
	      {
	          printf("INTERNAL ERROR in getTimebaseIdx: unexpected timebase dimension\n");
	          return 0;
	      }
	      numTimes = dims[0];
	      if(timebasePath)
	    	  updateCachedTimebase(timebasePath, times, numTimes);
	      else
	    	  updateCachedTimebase(dataobjectPath+timebase, times, numTimes);
	  }
	  if(timebasePath) delete[]timebasePath;

	  if(time <= times[0])
	      timebaseIdx = 0;
	  else if(time >= times[numTimes - 1])
              timebaseIdx = numTimes - 1;
	  else 
	  {
	      for(timebaseIdx = 0; timebaseIdx < numTimes - 1; timebaseIdx++)
	      	if(times[timebaseIdx] <= time && times[timebaseIdx + 1] > time)
	          break;
	  }
	  
        int actSlices = 0, prevSlices = 0;
	segIdx = 0;
	int numSegments = node->getNumSegments();
	char segNDims, segType;
	int  nextRow;
	int segDims[64];
	//printf("GET SEG IDX numSegments: %d\n", numSegments);
	for(segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    prevSlices = actSlices;
	    node->getSegmentInfo(segIdx, &segType, &segNDims, segDims, &nextRow);
	    if (segIdx < numSegments - 1)
		actSlices += nextRow;
//Gabriele June 2020		actSlices += segDims[segNDims - 1];
	    else
		actSlices += nextRow;
	    if(actSlices > timebaseIdx)
		break;
	}


//Gabriele June 2020	double *retTimes = new double[segDims[segNDims - 1]];
	double *retTimes = new double[nextRow];
	nDim = nextRow;
//	nDim = (segIdx < numSegments - 1)?segDims[segNDims - 1]:nextRow;
	memcpy(retTimes, &times[prevSlices], sizeof(double)* nDim);
	if(!timebaseCached) 
	    free(times);
	else
	    delete[] times;
	return retTimes;
    }







    MDSplus::Data *MDSplusBackend::assembleStringData(void *data, int numDims, int *inDims, int expectedLen)
    {
     //OLD UAL Compatibility: Invert dimensions
	int dims[16];
	for(int i = 0; i < numDims; i++)
	  dims[numDims - i -1] = inDims[i];
      //////////////////////////////////////////
	MDSplus::Data *retData;
	int *actDims = new int[numDims];
	memcpy(actDims, dims, numDims * sizeof(int));
	if(inDims[1] >= expectedLen)
	{
	    actDims[1] = expectedLen;
	    retData =  new MDSplus::Int8Array((char *)data,  numDims, actDims);
	}
	else //Need to fill shorter string
	{
	    char *extData = new char[expectedLen];
	    memcpy(extData, data, inDims[1]);
	    for(int i = inDims[1]; i < expectedLen; i++)
	        extData[i] = ' ';
	    actDims[1] = expectedLen;
	    retData = new MDSplus::Int8Array((char *)data,  numDims, actDims);
	    delete [] extData;
	}
	delete [] actDims;
	return retData;
    }
 
 
    MDSplus::Data *MDSplusBackend::assembleData(void *data, int datatype, int numDims, int *inDims)
    {
      //OLD UAL Compatibility: Invert dimensions
	int dims[16];
	for(int i = 0; i < numDims; i++)
	  dims[numDims - i -1] = inDims[i];
      //////////////////////////////////////////
    	switch(datatype) {
	  case ualconst::integer_data:
	    	if(numDims == 0) return new MDSplus::Int32(*(int *)data);
		return new MDSplus::Int32Array((int *)data, numDims, dims);
	    case ualconst::double_data:
	    	if(numDims == 0) return new MDSplus::Float64(*(double *)data);
		return new MDSplus::Float64Array((double *)data, numDims, dims);
	    case ualconst::char_data:
		return new MDSplus::Int8Array((char *)data,  numDims, dims);
    	    case ualconst::complex_data:
	        if(numDims == 0) return new MDSplus::Complex64(((double *)data)[0],
		    ((double *)data)[1]);
		return new MDSplus::Complex64Array((double *)data, numDims, dims);
	    default:
	      throw UALBackendException("Unexpected Data Type",LOG);
    
      }
    }
    
    void MDSplusBackend::disassembleData(MDSplus::Data *data, void **retDataPtr, int *datatype, int *retNumDims, int *retDims)
    {
    	char clazz, dtype, nDims;
	short length;
	void *dataPtr;
	int *dims;
   	data->getInfo(&clazz, &dtype, &length, &nDims, &dims, &dataPtr);
	switch(dtype)  {
	  case DTYPE_L: *datatype = ualconst::integer_data; break;
	    case DTYPE_DOUBLE: *datatype = ualconst::double_data; break;
	    case DTYPE_FTC: *datatype = ualconst::complex_data; break;
	    case DTYPE_T: 
	    case DTYPE_BU:
	    case DTYPE_B: *datatype = ualconst::char_data; break;
	default: throw UALBackendException("Unexpected Data type in disassembleData",LOG);
	}
	
	//Everything outside the backend has to be freed with free() 
	
	//Old UAL COmpatibility: invert dimensions
	//memcpy(retDims, dims, nDims * sizeof(int));
	for(int i = 0; i < nDims; i++)
	  retDims[nDims - i - 1] = dims[i];
	///////////////////////////////////////////////
	
	delete [] dims;
	
	*retNumDims = nDims;
	if(nDims == 0)
	{
	    *retDataPtr = malloc(length);
	    memcpy(*retDataPtr, dataPtr, length);
	    retDims[0] = 1;
	    // For string, store the length
	    retDims[1] = length;
	}
	else
	{
	    int nItems = 1;
	    for(int i = 0; i < nDims; i++)
		nItems *= (retDims)[i];
	    *retDataPtr = malloc(length * nItems);
	    memcpy(*retDataPtr, dataPtr, length * nItems);
	}
    }





    MDSplus::TreeNode * MDSplusBackend::getNode(const char *path, bool makeNew)
    {
	if(makeNew)
            return tree->getNode(path);
	std::string pathStr(path);
	MDSplus::TreeNode *node;
	auto search = treeNodeMap.find(pathStr);
	if(search != treeNodeMap.end())
	{
	    node = search->second;
	}
	else
	{
	    node = tree->getNode(path);
	    treeNodeMap[pathStr] = node;
	}

	return node;
    }


    void MDSplusBackend::freeNodes()
    {
		
	for ( auto it = treeNodeMap.begin(); it != treeNodeMap.end(); ++it )
	{
	    delete it->second;
	}
	treeNodeMap.clear();
    }

    int MDSplusBackend::getMdsShot(int shot, int run, bool translate, std::string strTree)
    {
	if(run > 99999)
	    throw  UALBackendException("Maximum run number allowed by MDSplus Backend is 99999",LOG);
        if(shot > 214748)
	    throw  UALBackendException("Maximum shot number allowed by MDSplus Backend is 214748",LOG);

	int runBunch = run/10000;

	if(translate)
	{
		char szPath[255] = { 0 };
		if (strTree.length() > 0)
		{
			sprintf(szPath, "%s_path", strTree.c_str());
		}
		else
		{
			sprintf(szPath, "%s_path", DEF_TREENAME);
		}
		
	    char baseName[64] = { 0 };
	    sprintf(baseName, "MDSPLUS_TREE_BASE_%d", runBunch);
	    char *translatedBase =  getenv(baseName);
	    if(translatedBase && *translatedBase)	// There is a translation for MDSPLUS_TREE_BASE_XX
	    {
		std::string translatedBaseStr(translatedBase);
		if(originalIdsPath == "")
		{
		    char *origPath = getenv(szPath);
		    if(origPath)
		        originalIdsPath = origPath; 
		}
		const char *modelDir = originalIdsPath.c_str();
		if(modelDir && *modelDir)  // ids_path is already set?
		{
		    translatedBaseStr += ';';
		    translatedBaseStr += modelDir;
		}
#ifdef WIN32
		char szEnv[256] = { 0 };
		sprintf(szEnv, "%s=%s", szPath, translatedBaseStr.c_str());
		putenv(szEnv);
#else // WIN32
		setenv(szPath, translatedBaseStr.c_str(), 1);
#endif // WIN32
	    }
	}
//	int retShot =  (shot * 10000) + (run%10000);
//	return retShot;
	return 1;  //Gabriele June 2022: now only shot number 1 is used since every different experiment resides on a different directory
    }
 


// Reset the IDS path to what it was before the call to getMdsShot
void MDSplusBackend::resetIdsPath(std::string strTree) {
    if (originalIdsPath != "") {
        char szPath[255] = { 0 };
        if (strTree.length() > 0)
        {
            sprintf(szPath, "%s_path", strTree.c_str());
        }
        else
        {
            sprintf(szPath, "%s_path", DEF_TREENAME);
        }

#ifdef WIN32
        char szEnv[256] = { 0 };
        sprintf(szEnv, "%s=%s", szPath, originalIdsPath.c_str());
        putenv(szEnv);
#else // WIN32
        setenv(szPath, originalIdsPath.c_str(), 1);
#endif // WIN32

        // Reset the global variable originalIdsPath to an empty string so
        // the environment gets set correctly on the next call to getMdsShot
        originalIdsPath = "";
    }
}

 #define PATH_MAX  2048
void MDSplusBackend::setDataEnv(DataEntryContext * ctx) 
{
  int i;

  std::string mdsplusBaseStr = ctx->getPath();

      // set every MDSPLUS_TREE_BASE_n env. variable
      for (i = 0; i < 10; i++) 
	{
	  std::string currMdsplusBaseDir = mdsplusBaseStr+"/";
	  //currMdsplusBaseDir += '0'+(char)i;
	  char env_name[32];
	  sprintf(env_name, "MDSPLUS_TREE_BASE_%d", i);
#ifdef WIN32
	  char szEnv[256] = { 0 };
	  sprintf(szEnv, "%s=%s", env_name, currMdsplusBaseDir.c_str());
	  putenv(szEnv);
#else // WIN32
	  setenv(env_name, currMdsplusBaseDir.c_str(), 1);
#endif // WIN32
    	}
    }  
  

    int MDSplusBackend::getSliceNumItems(int numDims, int *dims)
    {
    	int nItems = 1;
	for(int i = 0; i < numDims; i++)
	    nItems *= dims[i];
	return nItems;
    }

    int MDSplusBackend::getSliceSize(MDSplus::TreeNode *node, void *data, int datatype, int numDims, int *dims, bool isMultiple)
    {
//THE LAST DIMENSION REPRESENTS THE NUMBER OF SLICES!!!!!!!!!      
    	int nItems;
	if(isMultiple)
//	   nItems = getSliceNumItems(numDims-1, &dims[1]);
	   nItems = getSliceNumItems(numDims-1, &dims[0]);
	else
	   nItems = getSliceNumItems(numDims, dims);
	  
	switch(datatype) {
	//Make Sure that the number of slices in EVERY segment divides exactly the number of slices in the time segments
	//time is stored as double, so consider minimum item size as 8 bytes
	    case ualconst::integer_data: return sizeof(int) * nItems; 
	    case ualconst::complex_data: return 2 * sizeof(double) * nItems;
	    case ualconst::double_data: return sizeof(double) * nItems;
	    case ualconst::char_data:
	    {
	      int size = getStringSizeInSegment(node);
	      if(size == -1)
		size = dims[0]; //If strings already stored return their size, otherwise return the size of this one
	      return size;
	    }
	    default: return 0; //never reached
	}
    }

    void MDSplusBackend::writeData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, void *dataPtr, int datatype, int numDims,
	int *dims)
    {	
      if(!tree) throw UALBackendException("Pulse file not open",LOG);
    	try {
	    std::string fullPath = composePaths(dataobjectPath, path);
	    std::string checkedFullPath = checkFullPath(fullPath);
	    const char *checkedFullPathPtr = checkedFullPath.c_str();
	    MDSplus::TreeNode *node = getNode(checkedFullPathPtr);
	    if(node->getLength() > 0)
	        node->deleteData();
	    MDSplus::Data *data = assembleData(dataPtr, datatype, numDims, dims);
	    node->putData(data);
	    MDSplus::deleteData(data);
//	    delete node;
	}
	catch (MDSplus::MdsException &exc)
	{
	  throw UALBackendException(exc.what(),LOG);
	}
    }
	

	    
	    
	    
    void MDSplusBackend::writeTimedData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, std::string timebase, void *data, int datatype, int numDims,
	int *inDims, bool isAos, bool isRefAos, bool append)
    {
/** Steps:
1) Compute slice dimension
2) Compute number of slices per segment given the segment length 
3) For every segment: 
	3.1 Prepare start and end time 
	3.2 Prepare dimension
	3.3 Make segment. For the last segment: BeginSegment() and putSegment()
*/
      int dims[MAX_DIMS];
      for(int i = 0; i < numDims; i++)
//	dims[numDims-i-1] = inDims[i];
	dims[i] = inDims[i];

//The last dimension represents the time dimension, that is the number of slices	
//std::cout << "writeTimedData slices: "<< dims[0] << std::endl;      
      
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
    	try {
	    std::string fullPath = composePaths(dataobjectPath, path);
	    std::string mdsPath = checkFullPath(fullPath, isAos);
	    MDSplus::TreeNode *node = getNode(mdsPath.c_str());
//	    MDSplus::TreeNode *node = getNode(checkFullPath(fullPath).c_str());
	    
	    if(!append && (node->getLength() > 0 || node->getNumSegments() > 0))
	        node->deleteData();
		
	    int sliceSize = getSliceSize(node, data, datatype, numDims, dims, true);
	    int slicesPerSegment;
	    //Force the number of slice in time segments (double) to be the same of the  number of slices in data segments
	    if(datatype == ualconst::complex_data)
	      slicesPerSegment = 2 * MDSPLUS_SEGMENT_SIZE / sliceSize;
	    else if(datatype == ualconst::integer_data)
	      slicesPerSegment = (MDSPLUS_SEGMENT_SIZE/2) / sliceSize;
	    else
	      slicesPerSegment = MDSPLUS_SEGMENT_SIZE / sliceSize;
	    if(slicesPerSegment < 1) slicesPerSegment = 1;
	    int segmentSize = slicesPerSegment * sliceSize;

	    int prevSegments, prevSlices, freeSlices;
	    int leftSlices = dims[numDims -1];
	    if(append)
	    {
		prevSegments = node->getNumSegments();
		if(prevSegments == 0)  //if no slices present
		{
		    freeSlices = 0;
		    prevSlices = 0;
		}
		else
		{   
		    int segDims[64];
		    int  nextRow;
		    char segNDims, segType;
		    node->getSegmentInfo(prevSegments - 1, &segType, &segNDims, segDims, &nextRow);
		    freeSlices = slicesPerSegment - nextRow;
		    prevSlices = prevSegments * slicesPerSegment - freeSlices;
		}
	    }
	    else
	    {
		prevSegments = 0;
		freeSlices = 0;
		prevSlices = 0;
	    }
//First fill last nonfull segment   
	    if(freeSlices > leftSlices)
		freeSlices = leftSlices;
	    if(freeSlices > 0)
	    {
		
		int currDims[MAX_DIMS];
		for(int i = 0; i < numDims; i++)
		    currDims[i] = dims[i];
		currDims[numDims - 1] = freeSlices;
		MDSplus::Data *slices =  assembleData(((char *)data), datatype, numDims, currDims);
		node->putSegment((MDSplus::Array *)slices, -1);
		MDSplus::deleteData(slices);
		
	    }
	    leftSlices -= freeSlices;
	    if(leftSlices == 0) //All required slices fit in the left space of the last segment
		return; //All done

//	    int numSegments = (dims[numDims - 1] - freeSlices)/slicesPerSegment;
	    int numSegments = leftSlices / slicesPerSegment;
	    bool lastSegmentFilled = true;
//	    int lastSegmentSlices = (dims[numDims -1] - freeSlices) % slicesPerSegment;
	    int lastSegmentSlices = leftSlices % slicesPerSegment;

	    if(lastSegmentSlices != 0)
	    {
	        numSegments++;
		lastSegmentFilled = false;
	    }

	    std::string timePath = composePaths(dataobjectPath, timebase);

	    int *currDims = new int[numDims];
	    memcpy(currDims, dims, numDims * sizeof(int));
	    for(int segIdx = 0; segIdx < numSegments; segIdx++)
	    {	
		size_t sliceIdx = segIdx * slicesPerSegment;
		int timeSlicesPerSegment = MDSPLUS_SEGMENT_SIZE/sizeof(double);
		int timeStartSegmentIdx = (prevSlices + freeSlices + sliceIdx) / timeSlicesPerSegment;
		int timeStartSegmentOffset = (prevSlices + freeSlices + sliceIdx) - timeStartSegmentIdx * timeSlicesPerSegment;
		int timeEndSegmentIdx = (prevSlices + freeSlices + sliceIdx + slicesPerSegment - 1) / timeSlicesPerSegment;
		int timeEndSegmentOffset = (prevSlices + freeSlices + sliceIdx + slicesPerSegment - 1) - timeEndSegmentIdx * timeSlicesPerSegment;
		std::string segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
		char *nameBuf = new char[64+segData.size()]; 
		sprintf(nameBuf, "data(%s)[%d]", segData.c_str(), timeStartSegmentOffset);
		
		MDSplus::Data *startTime = tree->tdiCompile(nameBuf);
		delete [] nameBuf;


		segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeEndSegmentIdx);
		std::string segPath = mdsconvertPath(timePath.c_str(), isRefAos);
		nameBuf = new char[512+mdsPath.size()+2*segData.size()+segPath.size()];
		sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(%s)[%d - GetSegmentInfo(build_path(\'%s\'), %d) - 1]; else _a = data(%s)[%d]; _a;",
//		    mdsPath.c_str(), segIdx + 1, 
		    mdsPath.c_str(), prevSegments + segIdx + 1, 
		    segData.c_str(), timeSlicesPerSegment, segPath.c_str(), timeEndSegmentIdx,
		    segData.c_str(), timeEndSegmentOffset);

		MDSplus::Data *endTime = tree->tdiCompile(nameBuf);
		delete [] nameBuf;

		if(timeStartSegmentIdx == timeEndSegmentIdx) //Start and end times are in the same segment for times
		{
		    segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
		    segPath = mdsconvertPath(timePath.c_str(), isRefAos);
		    nameBuf = new char[512+mdsPath.size()+2*segData.size()+segPath.size()]; 

		    sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(%s)[%d:(%d - GetSegmentInfo(build_path(\'%s\'), %d)-1)]; else _a = data(%s)[%d:%d]; _a;",
//			mdsPath.c_str(), segIdx + 1, 
			mdsPath.c_str(), prevSegments + segIdx + 1, 
			segData.c_str(), timeStartSegmentOffset, timeSlicesPerSegment, segPath.c_str(), timeStartSegmentIdx,
			segData.c_str(), timeStartSegmentOffset, timeEndSegmentOffset); 
		} 
		else //Times are splitted in two different segments
		{
		  segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
		  std::string segData1 = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeEndSegmentIdx);
		  segPath = mdsconvertPath(timePath.c_str(), isRefAos);
		  nameBuf = new char[512 + mdsPath.size()+segData.size()+segData1.size()+3*segPath.size()];

//		  sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(set_range(",mdsPath.c_str(), segIdx + 1);
		  sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(set_range(",mdsPath.c_str(), prevSegments + segIdx + 1);
		  sprintf(&nameBuf[strlen(nameBuf)], "%d, [data(%s)[%d:%d], data(%s)[0:(%d - GetSegmentInfo(build_path(\'%s\'), %d)-1)]])); else _a = data(set_range(", 
			timeSlicesPerSegment - timeStartSegmentOffset + timeEndSegmentOffset + 1, segData.c_str(),
			timeStartSegmentOffset, timeSlicesPerSegment - 1, segData1.c_str(), timeSlicesPerSegment,
		 	segPath.c_str(), timeEndSegmentIdx);

		  sprintf(&nameBuf[strlen(nameBuf)], "%d, [data(getSegmentData(build_path('%s'), %d))[%d:%d], data(getSegmentData(build_path('%s'), %d))[0:%d]])); _a;", 
			timeSlicesPerSegment - timeStartSegmentOffset + timeEndSegmentOffset + 1, segPath.c_str(), timeStartSegmentIdx,
			timeStartSegmentOffset, timeSlicesPerSegment - 1, segPath.c_str(), timeEndSegmentIdx, timeEndSegmentOffset);
		}
		MDSplus::Data *dimension = tree->tdiCompile(nameBuf);
		delete []nameBuf;

		MDSplus::Data *slices;
		if(segIdx < numSegments - 1 || lastSegmentFilled)
		{
		    currDims[numDims - 1] = slicesPerSegment;
		    slices =  assembleData(((char *)data)+freeSlices * sliceSize + segIdx * segmentSize, datatype, numDims, currDims);
		    node->makeSegment(startTime, endTime, dimension, (MDSplus::Array *)slices);
		}
		else
		{
		    currDims[numDims - 1] = lastSegmentSlices;
		    slices =  assembleData(((char *)data)+freeSlices * sliceSize + segIdx * segmentSize, datatype, numDims, currDims);
		    char *initBytes = new char[sliceSize * slicesPerSegment];
		    memset(initBytes, 0, sliceSize * slicesPerSegment);

//Gabriele November 2017: fix dimensions of the segment
		    currDims[numDims - 1] = slicesPerSegment;
//////////////////////////////////////////////////////////
		    MDSplus::Data *initData = assembleData(initBytes, datatype, numDims, currDims);
		    node->beginSegment(startTime, endTime, dimension, (MDSplus::Array *)initData);
		    MDSplus::deleteData(initData);
		    delete[] initBytes;
		    node->putSegment((MDSplus::Array *)slices, -1);
		}
		MDSplus::deleteData(slices);
		MDSplus::deleteData(startTime);
		MDSplus::deleteData(endTime);
		MDSplus::deleteData(dimension);
	    }
//	    delete node;
	    delete []currDims;
	}catch(MDSplus::MdsException &exc)
	{
	  throw UALBackendException(exc.what(),LOG);
	}
    }
		
    int MDSplusBackend::readTimedData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, void **dataPtr, int *datatype,
	int *numDims, int *outDims, char *mdsPath)
    {
      int status = 0;
      if(!tree)  
	throw UALBackendException("Pulse file not open",LOG);
      MDSplus::TreeNode *node;
      try {
	if(mdsPath)
	{
	    node = getNode(mdsPath);
	}
	else
	{
	    std::string fullPath = composePaths(dataobjectPath, path);
	    node = getNode(checkFullPath(fullPath).c_str());
	}
      }catch(MDSplus::MdsException &exc)
      {
	//throw UALBackendException(exc.what(),LOG);
	return 0;
      }
      status = readTimedData(node, dataPtr, datatype, numDims, outDims);
   //   delete node;
      return status;
    }




    int MDSplusBackend::readTimedData(MDSplus::TreeNode *node, void **dataPtr, int *datatype,
	int *numDims, int *outDims)
    {
      int dims[MAX_DIMS];
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
      try {
	std::vector<MDSplus::Array *> segmentV;
	int numSegments = node->getNumSegments();
	if(numSegments == 0) 
	  {
//	    delete node;
	    return 0;
	  }
	MDSplus::Array *firstSegment = node->getSegment(0);
	int *segDims;
	char clazz, dtype, nDims;
	short length;
	void *ptr;
	int segInfoDims[64];
	char segNDims, segType;
	firstSegment->getInfo(&clazz, &dtype, &length, &nDims, &segDims, &ptr);
	//Only free() is used outside)
	int numSlices = segDims[0];
	int  nextRow;
	int actSlices; //keep into account the fact that the last segment may be not yet filled
//	node->getSegmentInfo(numSegments - 1, &segType, &segNDims, segInfoDims, &nextRow);
	node->getSegmentInfo(0, &segType, &segNDims, segInfoDims, &nextRow);
	actSlices = nextRow;
	
	memcpy(dims, segDims, nDims * sizeof(int));
	*numDims = nDims;
	segmentV.push_back(firstSegment);
	for(int i = 1; i < numSegments; i++)
	{
	    MDSplus::Array *currSegment = node->getSegment(i);
	    segmentV.push_back(currSegment);
	    int currNDims;
	    int *currSegDims = currSegment->getShape(&currNDims);
	    numSlices += currSegDims[0];
	    node->getSegmentInfo(i, &segType, &segNDims, segInfoDims, &nextRow);
	    actSlices += nextRow;
	    delete [] currSegDims;
	}

	int sliceSize = 1;
	for(int i = 1; i < nDims; i++)
//The last dimension represents the time dimension BUT the indexes have been inverted before 
//putting the slice in the segment 
//	for(int i = 0; i < nDims-1; i++)  //The last dimension represents the time dimension
	  sliceSize *= segDims[i];  
	sliceSize *= length;   //slice size in bytes
	char *currDataPtr = (char *)malloc((long)numSlices * (long)sliceSize);
	*dataPtr = currDataPtr;
	delete[] segDims;
	for(int i = 0; i < numSegments; i++)
	{
	  segmentV.at(i)->getInfo(&clazz, &dtype, &length, &nDims, &segDims, &ptr);
	  memcpy(currDataPtr, ptr, segDims[0] * sliceSize);
	  currDataPtr += segDims[0] * sliceSize;
	  delete [] segDims;
	  MDSplus::deleteData(segmentV.at(i));
	}
	switch(dtype) {
	  case DTYPE_L : *datatype = ualconst::integer_data; break;
	  case DTYPE_DOUBLE : *datatype = ualconst::double_data; break;
	  case DTYPE_FTC: *datatype = ualconst::complex_data; break;
	  case DTYPE_B: *datatype = ualconst::char_data; break;
	  case DTYPE_BU: throw UALBackendException("Time dependent string not supported. Use Arrays of structures instead",LOG);
	default: throw UALBackendException("Unsupported data type in timed data items",LOG);
	}
	
//	delete node;
	dims[0] = actSlices;
      }catch(MDSplus::MdsException &exc)
      {
       throw UALBackendException(exc.what(),LOG);
      }
      for(int i = 0; i < *numDims; i++)
	outDims[*numDims - 1 - i] = dims[i];
    
      //delete node;
      return 1;
    }
		
    void MDSplusBackend::writeSlice(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, std::string timebase, void *data, int datatype, int numDims, int *inDims, bool isAos, bool isRefAos)
    {
/** Steps:
1) Compute slice dimension
2) Compute number of slices per segment given the segment length 
3) Write segment data. If successful update dimension start time and end time.
    if Unsuccessful(segment filled) create a new segment and prepare new dimension start and end time

*/
//std::cout << "WRITE SLICE START" << std::endl;
      int dims[MAX_DIMS];
      for(int i = 0; i < numDims; i++)
	  dims[i] = inDims[i];


//Gabriele July 2017: one additional dimension (=1) is added by the high level, not to be considered here
      numDims--;
///////////////////////////////////////////////////////////////////////////////////////////////////////
	
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
    	try {
	    std::string fullPath = composePaths(dataobjectPath, path);
	    std::string mdsPath = checkFullPath(fullPath, isAos);
	    MDSplus::TreeNode *node = getNode(mdsPath.c_str());
	   // MDSplus::TreeNode *node = getNode(checkFullPath(fullPath).c_str());

	    int sliceSize = getSliceSize(node, data, datatype, numDims, dims, false);
	    int slicesPerSegment;
	    //Force the number of slice in time segments (double) to be the same of the  number of slices in data segments
	    if(datatype == ualconst::complex_data)
	      slicesPerSegment = 2 * MDSPLUS_SEGMENT_SIZE / sliceSize;
	    else if(datatype == ualconst::integer_data)
	      slicesPerSegment = (MDSPLUS_SEGMENT_SIZE/2) / sliceSize;
	    else if(datatype == ualconst::char_data)
	      slicesPerSegment = (MDSPLUS_SEGMENT_SIZE/8) / sliceSize;
	    else
	      slicesPerSegment = MDSPLUS_SEGMENT_SIZE / sliceSize;
	    if(slicesPerSegment < 1) slicesPerSegment = 1;

	    //std::string timeStr("time");
	    std::string timePath = composePaths(dataobjectPath, timebase);
	    
	    int *currDims = new int[numDims+1];


	    
 	    memcpy(&currDims[0], dims, numDims * sizeof(int));
	    currDims[numDims] = 1;
	    int numSegments = node->getNumSegments();
	    MDSplus::Data *slice;
	    if(datatype == ualconst::char_data)
	        slice = assembleStringData(data, numDims+1, currDims, sliceSize);
	    else
		slice = assembleData(data, datatype, numDims+1, currDims);

//////////Gabriele Febraury 2020: avoid generating exceptions for full segments 
	    char dtype, dimct;
	    int nextRow;
	    int segDims[64];
	    node->getSegmentInfo(node->getNumSegments() - 1, &dtype, &dimct, segDims, &nextRow);
	    if(nextRow < segDims[dimct - 1])  //Still room for another slice
		node->putSegment((MDSplus::Array *)slice, -1);
	    else	
/*	    try {
		node->putSegment((MDSplus::Array *)slice, -1);
	    }catch (MDSplus::MdsException &exc)   */

	    {
//std::cout << "GENERATED EXCEPTION FOR NEW SEGMENT" << std::endl;
		char *initBytes = new char[sliceSize * slicesPerSegment];
		memset(initBytes, 0, sliceSize * slicesPerSegment);
//Gabriele August 2015 Fortran order is assumed	    
		currDims[numDims] = slicesPerSegment;
//		currDims[0] = slicesPerSegment;
///////////////////////////////////////////////////
		MDSplus::Data *initData = assembleData(initBytes, datatype, numDims+1, currDims);
	    	char *nameBuf; //TODO: Check size -- DONE
		
		/*Compute the right segment idx and offset for time. Recall that time is a sequence of double values
		  and therefore the indexes and offsets may be diferent from the idx and offset for this segmented item
		  for dimension do not refer to a segment (start and end times must be self-consistent to be used)
		
		*/
		
		int sliceIdx = numSegments * slicesPerSegment;
		int timeSlicesPerSegment = MDSPLUS_SEGMENT_SIZE/sizeof(double);
		int timeStartSegmentIdx = sliceIdx / timeSlicesPerSegment;
		int timeStartSegmentOffset = sliceIdx - timeStartSegmentIdx * timeSlicesPerSegment;
		int timeEndSegmentIdx = (sliceIdx + slicesPerSegment - 1) / timeSlicesPerSegment;
		int timeEndSegmentOffset = (sliceIdx + slicesPerSegment - 1) - timeEndSegmentIdx * timeSlicesPerSegment;

		if(timePath == fullPath) //If writing time
		{
		    nameBuf = new char[32];
		    sprintf(nameBuf, "%d", sliceIdx );
		}
		else
		{
		    std::string segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
		    nameBuf = new char[63+segData.size()];
		    sprintf(nameBuf, "data(%s)[%d]", segData.c_str(), timeStartSegmentOffset);
//		    sprintf(nameBuf, "data(%s)[%d]", getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx).c_str(), timeStartSegmentOffset);
		}
	    	MDSplus::Data *startTime = tree->tdiCompile(nameBuf);
//	    	MDSplus::Data *startTime = MDSplus::compile(nameBuf, tree);
		delete [] nameBuf;
		if(timePath == fullPath) //If writing time
		{
		    nameBuf = new char[32];
		    sprintf(nameBuf, "%d", sliceIdx + slicesPerSegment - 1);
		}
		else
		{
		    std::string segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeEndSegmentIdx);
		    std::string segPath = mdsconvertPath(timePath.c_str(), isRefAos);

		    nameBuf = new char[512+mdsPath.size()+2*segData.size()+segPath.size()];
		    sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(%s)[%d - GetSegmentInfo(build_path(\'%s\'), %d) - 1]; else _a = data(%s)[%d]; _a;",
			mdsPath.c_str(), numSegments + 1, 
//			mdsPath.c_str(), numSegments, 
			segData.c_str(), timeSlicesPerSegment, segPath.c_str(), timeEndSegmentIdx,
			segData.c_str(), timeEndSegmentOffset);
		}

		MDSplus::Data *endTime = tree->tdiCompile(nameBuf);
//		MDSplus::Data *endTime = MDSplus::compile(nameBuf, tree);
		delete [] nameBuf;
		//Consistency check: timeStartSegmentIdx MUST be the same as timeEndSegmentIdx since the number 
		//of elements in the time segments is ALWAYS a multiple of the number of elements in any data segment
		if(timeStartSegmentIdx != timeEndSegmentIdx)
		  throw UALBackendException("INTERNAL ERROR: inconsistent number of slices per segment!!!!!",LOG);
		
		//Handle the case in which the range of times refers to 
		if(timePath == fullPath) //If writing time
		{
		    nameBuf = new char[64];
		    sprintf(nameBuf, "[%d:%d]", sliceIdx, sliceIdx+ slicesPerSegment - 1);
		}
		else
		{
		    if(timeStartSegmentIdx == timeEndSegmentIdx) //Start and end times are in the same segment for times
		    {
			std::string segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
			std::string segPath = mdsconvertPath(timePath.c_str(), isRefAos);
			nameBuf = new char[512+mdsPath.size()+2*segData.size()+segPath.size()];

		    	sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(%s)[%d:(%d - GetSegmentInfo(build_path(\'%s\'), %d)-1)]; else _a = data(%s)[%d:%d]; _a;",
			mdsPath.c_str(), numSegments + 1, 
//			mdsPath.c_str(), numSegments, 
			segData.c_str(), timeStartSegmentOffset, timeSlicesPerSegment, segPath.c_str(), timeStartSegmentIdx,
			segData.c_str(), timeStartSegmentOffset, timeEndSegmentOffset);
		    }
		    else //Times are splitted in two different segments
		    {
			std::string segData = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeStartSegmentIdx);
			std::string segData1 = getSegmentData(mdsconvertPath(timePath.c_str(), isRefAos).c_str(), timeEndSegmentIdx);
			std::string segPath = mdsconvertPath(timePath.c_str(), isRefAos);
			nameBuf = new char[512+mdsPath.size()+2*segData.size()+2+segData1.size()+segPath.size()];

		  	sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(set_range(",mdsPath.c_str(), numSegments+1);
//		  	sprintf(nameBuf, "if(GetNumSegments(\'%s\') == %d) _a = data(set_range(",mdsPath.c_str(), numSegments);
		  	sprintf(&nameBuf[strlen(nameBuf)], "%d, [data(%s)[%d:%d], data(%s)[0:(%d - GetSegmentInfo(build_path(\'%s\'), %d)-1)]])); else _a = data(set_range(", 
			  timeSlicesPerSegment - timeStartSegmentOffset + timeEndSegmentOffset + 1, segData.c_str(),
			  timeStartSegmentOffset, timeSlicesPerSegment - 1, segData1.c_str(), timeSlicesPerSegment,
		 	  segPath.c_str(), timeEndSegmentIdx);

		  	sprintf(&nameBuf[strlen(nameBuf)], "%d, [data(%s)[%d:%d], data(%s)[0:%d]])); _a;", 
			  timeSlicesPerSegment - timeStartSegmentOffset + timeEndSegmentOffset + 1, segData.c_str(),
			  timeStartSegmentOffset, timeSlicesPerSegment - 1, segData1.c_str(), timeEndSegmentOffset);
		    }
		}

		MDSplus::Data *dimension = tree->tdiCompile(nameBuf);
//		MDSplus::Data *dimension = MDSplus::compile(nameBuf, tree);
		delete [] nameBuf;
		node->beginSegment(startTime, endTime, dimension, (MDSplus::Array *)initData);
		MDSplus::deleteData(initData);
		MDSplus::deleteData(startTime);
		MDSplus::deleteData(endTime);
		MDSplus::deleteData(dimension);
		delete[] initBytes;
		node->putSegment((MDSplus::Array *)slice, -1);
	    }
	    MDSplus::deleteData(slice);
//	    delete node;
	    delete [] currDims;

	}catch(MDSplus::MdsException &exc)
	{
	  throw UALBackendException(exc.what(),LOG);
 	}

//std::cout << "WRITE SLICE END" << std::endl << std::endl;



    }

    int MDSplusBackend::readData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path, void **dataPtr, int *datatype,
	int *numDims, int *dims, char *mdsPath)
    {
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
      try {
	MDSplus::TreeNode *node;
	if(mdsPath)
	{
	    node = getNode(mdsPath);
	}
	else
	{
	    std::string fullPath = composePaths(dataobjectPath, path);
	    node = getNode(checkFullPath(fullPath).c_str());
	}
	int numSegments = node->getNumSegments();
	if(numSegments > 0)
	  {
//	    delete node;
	    return readTimedData(tree, dataobjectPath, path, dataPtr, datatype, numDims, dims, mdsPath);
	  }
	else
	  {
	    if(node->getLength() == 0) 
	      {
//		delete node;
		return 0;
	      }
	    MDSplus::Data *data = node->getData();
	    MDSplus::Data *evaluatedData = data->data();
	    disassembleData(evaluatedData, dataPtr, datatype, numDims, dims);
	    MDSplus::deleteData(data);
	    MDSplus::deleteData(evaluatedData);
	  }
//	delete node;
      }catch(MDSplus::MdsException &exc)
	{
	  return 0;
//	  throw UALBackendException(exc.what(),LOG);
	}
      return 1;
    }


    void MDSplusBackend::deleteData(MDSplus::Tree *tree, std::string dataobjectPath, std::string path)
    {
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
//Workaround for the fact that default nid mau be left changed if TreeNode::getNode() generates an exception. Fixed in new MDSplus releases.
      MDSplus::TreeNode *topNode = getNode("\\TOP");
      segmentIdxMap.clear();
      try {
	std::string fullPath = composePaths(dataobjectPath, path);
	MDSplus::TreeNode *node = getNode(checkFullPath(fullPath).c_str());
	node->deleteData();
//Handle the possibility that the node refers to a AoS
        if(strcmp(node->getUsage(),"STRUCTURE") == 0)
	{
	    std::string currPath(":static");
	    MDSplus::TreeNode *currNode = node->getNode(checkFullPath(currPath, true).c_str());
	    currNode->deleteData();
	    delete currNode;

	    currPath = ".timed_aos";
	    currNode = node->getNode(checkFullPath(currPath, true).c_str());
	    int numGroups = currNode->getNumChildren();

	    for(int groupIdx = 1; groupIdx <= numGroups; groupIdx++)
	    {
		char buf[64]; //TODO: Check size -- DONE
		sprintf(buf, "group_%d", groupIdx);
		currPath = buf;
		MDSplus::TreeNode *currGroup = currNode->getNode(checkFullPath(currPath, true).c_str());
		int numChildren = currGroup->getNumChildren();
		int childIdx;
		for(childIdx = 1; childIdx <= numChildren; childIdx++)
		{
		    char buf[64]; //TODO: Check size -- DONE
		    sprintf(buf, "item_%d:aos", (groupIdx-1)*1000+childIdx);
		    currPath = buf;
		    MDSplus::TreeNode *currChild = currGroup->getNode(checkFullPath(currPath, true).c_str());
		    if(currChild->getLength() == 0) break;  //Avoid goin through no more used timed_n
		    currChild->deleteData();
		    delete currChild;
		    sprintf(buf, "item_%d:time", (groupIdx-1)*1000+childIdx);
		    currPath = buf;
		    currChild = currGroup->getNode(checkFullPath(currPath, true).c_str());
		    currChild->deleteData();
		    delete currChild;
		}
		if(childIdx <= numChildren) break; //Avoid goin through no more used timed_n
		delete currGroup;
	    }
	    delete currNode;

	    currPath = ".timed_data";
	    currNode = node->getNode(checkFullPath(currPath, true).c_str());
	    numGroups = currNode->getNumChildren();

	    for(int groupIdx = 1; groupIdx <= numGroups; groupIdx++)
	    {
		char buf[64]; //TODO: Check size -- DONE
		sprintf(buf, "group_%d", groupIdx);
		currPath = buf;
		MDSplus::TreeNode *currGroup = currNode->getNode(checkFullPath(currPath, true).c_str());
		int numChildren = currGroup->getNumChildren();
		int childIdx;
		for(childIdx = 1; childIdx <= numChildren; childIdx++)
		  {
		    char buf[64]; //TODO: Check size -- DONE
		    sprintf(buf, "item_%d", (groupIdx-1)*1000+childIdx);
		    currPath = buf;
		    MDSplus::TreeNode *currChild = currGroup->getNode(checkFullPath(currPath, true).c_str());
		    if(currChild->getLength() == 0) break;  //Avoid goin through no more used timed_n
		    currChild->deleteData();
		    delete currChild;
		}
		if(childIdx <= numChildren) break; //Avoid goin through no more used timed_n
		delete currGroup;
	    }
	    delete currNode;
	}
//	delete node;
      }catch(MDSplus::MdsException &exc)
      {
//	throw UALBackendException(exc.what(),LOG);
	tree->setDefault(topNode);
//	delete topNode;
	throw UALBackendException(exc.what(),LOG);
      }
//      delete topNode;
    }

	



    int  MDSplusBackend::readSlice(MDSplus::Tree *tree, bool isApd, std::string dataobjectPath, std::string path, std::string timebase, double time, int interpolation, void **data, int *datatype,
	int *numDims, int *outDims, bool manglePath)
    {
/** Steps:
1) Find right segment
2) Get time, data and info 
3) Write segment data. If successful update dimension start time and end time.
    if Unsuccessful(segment filled) create a new segment and prepare new dimension start and end time

*/
	int dims[MAX_DIMS];
	bool isComplex = false;
        double *times;
	int nTimes;

    	if(!tree)  throw UALBackendException("Pulse file not open",LOG);
	std::string fullPath;
	MDSplus::TreeNode *node;
    	try {
	    if(isApd)
		fullPath = dataobjectPath;
	    else
	    	fullPath = composePaths(dataobjectPath, path);
	    if(manglePath)
	    	node = getNode(checkFullPath(fullPath).c_str());
	    else
	    	node = getNode(fullPath.c_str());
	}catch(MDSplus::MdsException &exc) 
	{	
	    return 0;
	    //throw UALBackendException(exc.what(),LOG);
	}


	try {
// Gabriele Dec 2019 --- OPTIMIZATION: avoid evaluating expressions for segment limits for non AoS fields
	    int numSegments = node->getNumSegments();
            int segmentIdx;
	    if(numSegments == 0) 
	      return 0;	      

	    //Since the expression contains node reference, it is necessary to set the active tree
	    //setActiveTree(tree);
//	    if(path == "") //In case readSlice is called inside AoS
	    if(isApd) //In case readSlice is called inside AoS
	    {
		//setActiveTree(tree);

		times = getSegmentIdxAndDim(node, dataobjectPath, timebase, time, segmentIdx, nTimes);
/*

		for(segmentIdx = 0; segmentIdx < numSegments; segmentIdx++)
		{
	  	    MDSplus::Data *startTimeData, *endTimeData;
		    node->getSegmentLimits(segmentIdx, &startTimeData, &endTimeData);
		
//		      std::cout << "START TIME: " << startTimeData << std::endl;
//		      std::cout << "END TIME: " << endTimeData << std::endl;
		
		    double startTime = startTimeData->getDouble();
	//Handle the case end time refers to a longer list that that contained in time node (homogenerous)
		    double endTime;
		//try {
		    endTime = endTimeData->getDouble();
			//}catch(MDSplus::MdsException &exc) {endTime = startTime;}
		    MDSplus::deleteData(startTimeData);
		    MDSplus::deleteData(endTimeData);
		//   if((startTime <= time && time <= endTime)||(time <= startTime && segmentIdx == 0) || (time >= endTime && segmentIdx == numSegments - 1))
		    if((startTime <= time && time <= endTime)||(time <= startTime && segmentIdx == 0) || 
				(time >= endTime && segmentIdx == numSegments - 1) || 
				(time < startTime && segmentIdx > 0)) //consider the case in which the time is between two segments
		  	break;
		}
		//Build times from startTimes and endTimes
    		MDSplus::Data *segDim = node->getSegmentDim(segmentIdx);


//times = new double[5]{1,2,3,4,5}; nTimes = 5;
	    	times = segDim->getDoubleArray(&nTimes);
		MDSplus::deleteData(segDim);
*/	    }
	    else
	    {
//printf("PATH: %s TIME: %s\n", dataobjectPath.c_str(), timebase.c_str());
		times = getSegmentIdxAndDim(node, dataobjectPath, timebase, time, segmentIdx, nTimes);
		    //int timebaseIdx = getTimebaseIdx(dataobjectPath, timebase, time);
		//printf("TIMEBASE IDX: %d\n", timebaseIdx);
		    //segmentIdx = getSegmentIdx(node, timebaseIdx);
		//printf("SEGMENT IDX: %d\n", segmentIdx);
		    //numSegments = node->getNumSegments();
	    }	
///////////////////////////////////////OPTIMIZATION

	    MDSplus::Array *segDataRead = node->getSegment(segmentIdx);
//Workaround for MDSplus bug in complex management	    
	    char clazz, ddtype, nDims;
	    short length;
	    void *dataPtr;
	    int *ddims;
	    segDataRead->getInfo(&clazz, &ddtype, &length, &nDims, &ddims, &dataPtr);
	    isComplex = (ddtype == DTYPE_FTC);
/////////////////////////////	    
//Gabriele Jan 2020: avoid useless array copy

//	    MDSplus::Data *segData = segDataRead->data();
//	    MDSplus::deleteData(segDataRead);
            MDSplus::Data *segData = segDataRead;
//////////////////////////////////////////////////////////
//	    MDSplus::Data *segDim = node->getSegmentDim(segmentIdx);
//            std::cout << segDim->decompile() << std::endl;
	    char dtype, dimct;
	    int nextRow;
	    int segDims[64];
	    node->getSegmentInfo(segmentIdx, &dtype, &dimct, segDims, &nextRow);
//	    int nTimes;
//	    double *times = segDim->getDoubleArray(&nTimes);
	    //In case the segment is not completely filled consider the right amount of times
	    //if(nTimes > nextRow) Gabriele February 2018: consider nextRow ONLY for the last segment because getSegmentInfo() works only for the last segment
	    if(nTimes > nextRow && segmentIdx == numSegments - 1)
	      nTimes = nextRow;
	    
	    //Consider the special case in which the time is between two segments. 
	    //In this case the previous segment is read and appended in front of the current one, updating segData and times
	    //if(time <= times[0] && segmentIdx > 0)
	    if(time < times[0] && segmentIdx > 0) 
	    {
	      MDSplus::Array *prevSegDataRead = node->getSegment(segmentIdx - 1);
	      MDSplus::Data *prevSegData = prevSegDataRead->data();
	      MDSplus::deleteData(prevSegDataRead);
	      char exprBuf[1024];
	      sprintf(exprBuf, "data(set_range(");
	      for(int i = 0; i < nDims - 1; i++)
		sprintf(&exprBuf[strlen(exprBuf)], "%d,", ddims[nDims - i - 1]); //ddims are returned in FORTRAN order!!!!!
	      //sprintf(&exprBuf[strlen(exprBuf)], "size($1)+size($2),[$1,$2]))", 2, prevSegData, segData);
	      sprintf(&exprBuf[strlen(exprBuf)], "size($1,%d)+size($2,%d),[$1,$2]))", nDims-1, nDims-1);
	      MDSplus::Data *mergedSegData = MDSplus::executeWithArgs(exprBuf, 2, prevSegData, segData);
	      MDSplus::deleteData(prevSegData);
	      MDSplus::deleteData(segData);
	      segData = mergedSegData;
	      segData->getInfo(&clazz, &ddtype, &length, &nDims, &ddims, &dataPtr); //Gabriele February 2020

	      int nPrevTimes;
	      MDSplus::Data *prevSegDim = node->getSegmentDim(segmentIdx - 1);
	      double *prevTimes = prevSegDim->getDoubleArray(&nPrevTimes);
	      double *mergedTimes = new double[nPrevTimes+nTimes];
	      memcpy(mergedTimes, prevTimes, nPrevTimes * sizeof(double));
	      memcpy(&mergedTimes[nPrevTimes], times, nTimes * sizeof(double));
	      nTimes += nPrevTimes;
	      delete[] prevTimes;
	      delete [] times;
	      MDSplus::deleteData(prevSegDim);
	      times = mergedTimes;
	    }

	    int idx;
	    MDSplus::Data *sliceData;
	    //Build appropriate expression based on interpolation
	    //If time outside segment limits, just return the upper or lower limit
	    char expr[512];
            int idxInSegment;
	    if(time <= times[0])
	    {
	      sprintf(expr, "$1[");
	      for(int i = 0; i < dimct - 1; i++)
		sprintf(&expr[strlen(expr)], "*,");
	      sprintf(&expr[strlen(expr)], "0]");
              idxInSegment = 0;
	    }
	    else if(time >= times[nTimes - 1])
	    {
	      sprintf(expr, "$1[");
	      for(int i = 0; i < dimct - 1; i++)
		sprintf(&expr[strlen(expr)], "*,");
	      sprintf(&expr[strlen(expr)], "%d]", nTimes - 1);
              idxInSegment = nTimes - 1;
	    }
	    else
	    {
	    //Find the index for just above time
	      for(idx = 1; idx < nTimes && times[idx] <= time; idx++);
	    //idx-1 and idx are the delimiting indexes
	      int actIdx;
	      switch(interpolation) {
		case ualconst::closest_interp:
		  if(time - times[idx-1] < times[idx] - time)
		    actIdx = idx - 1;
		  else
		    actIdx = idx;
		  sprintf(expr, "$1[");
		  for(int i = 0; i < dimct - 1; i++)
		    sprintf(&expr[strlen(expr)], "*,");
		  sprintf(&expr[strlen(expr)], "%d]", actIdx);
                  idxInSegment = actIdx;
		  break;
		case ualconst::previous_interp:
		  sprintf(expr, "$1[");
		  for(int i = 0; i < dimct - 1; i++)
		    sprintf(&expr[strlen(expr)], "*,");
		  sprintf(&expr[strlen(expr)], "%d]", idx - 1);
                  idxInSegment = idx - 1;
		  break;
		case ualconst::linear_interp:
		  if(dtype == DTYPE_L)
		    sprintf(expr, "INT($1[");
		  else
		    sprintf(expr, "($1[");
		  for(int i = 0; i < dimct - 1; i++)
		    sprintf(&expr[strlen(expr)], "*,");
		  sprintf(&expr[strlen(expr)], "%d]+($1[", idx - 1);
		  for(int i = 0; i < dimct - 1; i++)
		    sprintf(&expr[strlen(expr)], "*,");
		  sprintf(&expr[strlen(expr)], "%d]-$1[", idx);
		  for(int i = 0; i < dimct - 1; i++)
		    sprintf(&expr[strlen(expr)], "*,");
		  if(isComplex)
		    sprintf(&expr[strlen(expr)], "%d])*cmplx(%e,0.))", idx-1, (time - times[idx-1])/(times[idx]-times[idx-1]));
		  else
		    sprintf(&expr[strlen(expr)], "%d])*%e)", idx-1, (time - times[idx-1])/(times[idx]-times[idx-1]));
                  idxInSegment = -1;
		  break;
		default:
		  MDSplus::deleteData(segData);
		 // MDSplus::deleteData(segDim);
		  delete[] times;
		  throw UALBackendException("Unsupported interpolation",LOG);
	      }
	    }
	    delete [] times;
	    //Expression for slice retrieval is now ready
	    int nSamples;
	    char *currData;
//Gabriele Jan 2020 If not interpolation required, just copy the corresponding data chunk
	    if(idxInSegment >= 0) 
	    {
		int rowSize = length;
//		for(int i = 0; i < nDims - 1; i++)
		for(int i = 1; i < nDims; i++)
		    rowSize *= ddims[i];
		*data = malloc(rowSize);
		memcpy(*data, ((char *)dataPtr)+(idxInSegment * rowSize), rowSize);

		switch(dtype)  {
		    case DTYPE_B:
		    case DTYPE_BU:
			*datatype = ualconst::char_data;
		        break;
		    case DTYPE_L:
		    case DTYPE_LU:
			*datatype = ualconst::integer_data;
			break;
	      	    case DTYPE_DOUBLE:
	      	    case DTYPE_D: //For compatibility with old UAL
		    	*datatype = ualconst::double_data;
			break;
	      	    case DTYPE_FTC:
		    	*datatype = ualconst::complex_data;
		}
            }
	    else  //Interpolation required
            { 
	    	sliceData = MDSplus::executeWithArgs(expr, 1, segData);
	    	switch(dtype) {
	      	    case DTYPE_B:
	            case DTYPE_BU:  
		    currData = sliceData->getByteArray(&nSamples);
		    *data = malloc(nSamples);
		    memcpy(*data, currData, nSamples);
		    delete [] currData;
		    *datatype = ualconst::char_data;
		    break;
	        case DTYPE_L:
		    if(dimct == 1)//Scalar slice
		    {
		  	nSamples = 1;
		  	*data = malloc(sizeof(int));;
		  	*((int *)*data) = sliceData->getInt();
		    }
		    else
		    {
		        currData = (char *)sliceData->getIntArray(&nSamples);
		  	*data = malloc(nSamples * sizeof(int));
		  	memcpy(*data, currData, nSamples * sizeof(int));
		  	delete [] currData;
		    }
		    *datatype = ualconst::integer_data;
		    break;
	      	case DTYPE_DOUBLE:
	      	case DTYPE_D: //For compatibility with old UAL
		    if(dimct == 1)//Scalar slice
		    {
		  	nSamples = 1;
		  	*data = malloc(sizeof(double));
		  	*((double *)*data) = sliceData->getDouble();
		    }
		    else
		    {
		  	currData = (char *)sliceData->getDoubleArray(&nSamples);
		  	*data = malloc(nSamples * sizeof(double));
		  	memcpy(*data, currData, nSamples * sizeof(double));
		  	delete [] currData;
		    }
		    *datatype = ualconst::double_data;
		    break;
	      	case DTYPE_FTC:
		    if(dimct == 1)//Scalar slice
		    {
		  	nSamples = 1;
		  	std::complex<double> complexData = sliceData->getComplex();
		  	*data = malloc(2*sizeof(double));
		  	((double *)(*data))[0] = complexData.real();
		  	((double *)(*data))[1] = complexData.imag();
		    }
		    else
		    {
		  	std::complex<double> *complexData = sliceData->getComplexArray(&nSamples);
		  	*data = malloc(2 * nSamples*sizeof(double));
		  	for(int i = 0; i < nSamples; i++)
		  	{
		    	    ((double *)(*data))[2*i] = complexData[i].real();
		    	    ((double *)(*data))[2*i+1] = complexData[i].imag();
		  	}
		  	delete [] complexData;
		    }
		    *datatype = ualconst::complex_data;
	    	}
	    	MDSplus::deleteData(sliceData);
	    }
//////////////////////////////////////////////////////////////
	    delete [] ddims;

	    MDSplus::deleteData(segData);
	  //  MDSplus::deleteData(segDim);
	    *numDims = dimct - 1;
	    //*dims = new int[dimct - 1];
	    for(int i = 0; i < dimct-1; i++)
	      (dims)[i] = segDims[i];
	    
	    for(int i = 0; i < *numDims; i++)
	      outDims[i] = dims[i];
	    outDims[*numDims] = 1;
	    (*numDims)++;
///////////////////////////////////////////	

            return 1;

	}catch(MDSplus::MdsException &exc)
	{
	  throw UALBackendException(exc.what(),LOG);
	}
    }




/////////////////////////////////////////////////////////////////////////////////////
/** Array of Structure support methods */
////////////////////////////////////////////////////////////////////////////////////
    MDSplus::Apd *MDSplusBackend::getApdFromContext(ArraystructContext *arrStructCtx)
    {
	pthread_mutex_lock(&mutex);
	MDSplus::Apd *arrStructData = NULL;
	auto search = arrayStructCtxDataMap.find(arrStructCtx);
	if (search!=arrayStructCtxDataMap.end())
	  arrStructData = search->second;

	pthread_mutex_unlock(&mutex);
	return arrStructData;

	// for(size_t i = 0; i < arrayStructContextV.size(); i++)
	// {
	//     if(arrayStructContextV[i] == arrStructCtx)
	//     {
	// 	pthread_mutex_unlock(&mutex);
	// 	return arrayStructDataV[i];
	//     }
	// }
	// pthread_mutex_unlock(&mutex);
	// return NULL;
    }
    
    void MDSplusBackend::addContextAndApd(ArraystructContext *arrStructCtx, MDSplus::Apd *arrStructData)
    {
	pthread_mutex_lock(&mutex);
	auto search = arrayStructCtxDataMap.find(arrStructCtx);
	if (search!=arrayStructCtxDataMap.end())
	  {
	    search->second = arrStructData;
	  }
	else
	  {
	    arrayStructCtxDataMap.insert({arrStructCtx,arrStructData});
	  }

	// size_t i;
	// for(i = 0; i < arrayStructContextV.size(); i++)
	// {
	//     if(arrayStructContextV[i] == arrStructCtx)
	//     {
	//        // std::cout << "INTERNAL ERROR IN addContextAndApd\n"; Gabriele March 2019: seems that context my be reused.....
	// 	arrayStructDataV[i] = arrStructData;
	// 	break;
	//     }
	// }
	// if(i == arrayStructContextV.size()) //New context
	// {
	//   arrayStructContextV.push_back(arrStructCtx);
	//   arrayStructDataV.push_back(arrStructData);
	// }
	pthread_mutex_unlock(&mutex);
    }
    
    
    void MDSplusBackend::removeContextAndApd(ArraystructContext *arrStructCtx, MDSplus::Apd *arrStructData)
    {
	pthread_mutex_lock(&mutex);
	arrayStructCtxDataMap.erase(arrStructCtx);

	// for(size_t i = 0; i < arrayStructContextV.size(); i++)
	// {
	//     if(arrayStructContextV[i] == arrStructCtx)
	//     {
	// 	arrayStructContextV.erase(arrayStructContextV.begin()+i);
	// 	arrayStructDataV.erase(arrayStructDataV.begin()+i);
	//     }
	// }
	pthread_mutex_unlock(&mutex);
    }
//////////Array of structures stuff	
    void MDSplusBackend::writeStaticApd(MDSplus::Apd *apd, std::string dataobjectPath, std::string path)
    {

//      std::cout << "writeApd " << path << "\n";
//      if(!path.compare("antenna_ec"))
//       dumpArrayStruct(apd, 0);
      
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
    	try {
	    std::string extPath = path;
	    extPath += ":static";
	    std::string fullPath = composePaths(dataobjectPath, extPath);
	    MDSplus::TreeNode *node = getNode(checkFullPath(fullPath, true).c_str());
	    node->putData(apd);
//	    delete node;
	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }


//GAB February 2017. Write a dynamic AoS
    void MDSplusBackend::writeDynamicApd(MDSplus::Apd *apd, std::string aosPath, std::string timebasePath, bool append)
//aosPath refers to the path in the tree of the target AoS root (with children "TIME" and "AOS")
//if timedPath is empty, timebase will be derived by AoS "time" field. Otherwise it represet the full path in the tree of the timebase.

    {


/*/Gabriele June 2020: Test 
//dumpArrayStruct(apd, 0);
std::cout<<"PARTE DEFLATE" << std::endl;
    deflateApd(apd);
std::cout<<"FINISCE DEFLATE" << std::endl;
//dumpArrayStruct(apd, 0);
std::cout<<"PARTE INFLATE" << std::endl;
    inflateApd(apd);
std::cout<<"FINSCE INFLATE" << std::endl;
//dumpArrayStruct(apd, 0);
/////////////////////////*/



      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
      try {
	  std::string timePath;
     	  if(timebasePath.empty())
	  { 
	    //first step: extract timebase from "time" field of the passed AOS and store it in the timePath node (segmented)
	      int numElements = (int)apd->len();
	      double *times = new double[numElements];
	      std::string time("time");
	      for(int i = 0; i < numElements; i++)
	      {
		  MDSplus::Data *currTimeD = getFromApd(apd, i, time);
		  if(!currTimeD)
		  {
		      if(apd->getDescAt(i) == NULL)
			  times[i] = 0;
		      else
		      	  throw  UALBackendException("Cannot get Time information",LOG); //Gabriele  Oct 2019
	   	  }
		  else
		      times[i] = currTimeD->getDouble();
	      }
	      std::string newTimebasePath = composePaths(aosPath, "time");
	      int dims[] = {numElements};
	      writeTimedData(tree, aosPath, time, time, times, ualconst::double_data, 1, dims, true, true, append);
	      timePath = newTimebasePath;
	      delete[] times;
	  }
	  else
	      timePath = timebasePath;  //Timebase is already defined somewhere else


	std::string aos = composePaths(aosPath, "aos");
	MDSplus::TreeNode *node = getNode(checkFullPath(aos, true).c_str());

//GABRIELE DA QUA ******************************************
//devo trovare quante slices sono state salvate fino ad ora come pure lo spazio rimasto nel segmento
	int numSegments, startSlice = 0;
	numSegments = node->getNumSegments();
	for(int segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    MDSplus::Data *serializedData = node->getSegment(segIdx);
	    char clazz, dtype, nDims;
	    short length;
	    int *dims;
	    char *serialized;
		((MDSplus::Array *) serializedData)->getInfo(&clazz, &dtype, &length, &nDims, &dims, (void **) &serialized);
	    if(dtype != DTYPE_B && dtype != DTYPE_BU)
	    {
	  	throw  UALBackendException("INTERNAL ERROR: unexpected dtype in serialized AoS: ", LOG); 
	    }
	    if(nDims != 1)
	    {
	  	throw  UALBackendException("INTERNAL ERROR: unexpected dimensions  in serialized AoS: ", LOG); 
	    }
	    int idx = 0;
	    while(idx < dims[0])
	    {		
		if(idx > dims[0])    
	  	    throw  UALBackendException("INTERNAL ERROR: unexpected length in serialized AoS: ", LOG); 
		int sliceLen;
		memcpy(&sliceLen, &serialized[idx], sizeof(int));
		idx += sizeof(int);
		idx += sliceLen;
		startSlice++;
	    }
	    MDSplus::deleteData(serializedData);
	}

//Regardless free space in the last segment (if put slice called before) a new segment is initiaied

	char *nameBuf; //TODO: Check size
	size_t sliceIdx = 0;
	while(sliceIdx < apd->len())
	{
	    int leftSpaceInSegment = MDSPLUS_SEGMENT_SIZE;
	    std::vector<unsigned char *>serializedV;
	    std::vector<int> serializedLenV;
	    int baseSliceIdx = sliceIdx;
	    while(leftSpaceInSegment > 0 && sliceIdx < apd->len())
	    {
		MDSplus::Data *currStruct = apd->getDescAt(sliceIdx);
		int currLen;
		unsigned char *currSerialized;
		if(currStruct)
		{
		//Gabriele June 2020
//((MDSplus::Apd *)currStruct, 0);
		    deflateApd((MDSplus::Apd *)currStruct);
		    currSerialized = (unsigned char *)currStruct->serialize(&currLen);
		    //currSerialized = (unsigned char *)currStruct->serialize(&currLen);
		}
		else //hole
		{
		    currSerialized = 0;
	  	    currLen = 0;
		}
		serializedV.push_back(currSerialized);
		serializedLenV.push_back(currLen);
		leftSpaceInSegment -= currLen;
		sliceIdx++;
	    }
	    {
		int segSize = 0;
		for(size_t idx = 0; idx < serializedLenV.size(); idx++)
		    segSize += serializedLenV[idx] + sizeof(int);
		unsigned char *segment = new unsigned char[segSize];

		int numSlicesInSegment = serializedLenV.size();
		    //GAB Jan 2015: segSize must be set to 0!!!
		segSize = 0;
		for(int idx = 0; idx < numSlicesInSegment; idx++)
		{
		    int currLen = serializedLenV[idx];
		    memcpy(&segment[segSize], &currLen, sizeof(int));
		    segSize += sizeof(int);
		    if(currLen > 0)
		    {
			memcpy(&segment[segSize], serializedV[idx], currLen);
			delete [] serializedV[idx];
			segSize += currLen;
		    }
		}
//Gabriele Dec 2017: turaround MDSplus problem of numbers in path
		std::string currPath = mdsconvertPath(timePath.c_str());
		nameBuf = new char[128 + currPath.size()];
		sprintf(nameBuf, "data(build_path(\'%s\'))[%d]",currPath.c_str(), startSlice + baseSliceIdx);
//		sprintf(nameBuf, "data(build_path(\'%s\'))[%d]",mdsconvertPath(timePath.c_str()).c_str(), baseSliceIdx);
//		MDSplus::Data *start = MDSplus::compile(nameBuf, tree);
		MDSplus::Data *start = tree->tdiCompile(nameBuf);
		sprintf(nameBuf, "data(build_path(\'%s\'))[%d]", currPath.c_str(), startSlice + (int)sliceIdx - 1);
//		sprintf(nameBuf, "data(build_path(\'%s\'))[%d]", mdsconvertPath(timePath.c_str()).c_str(), (int)sliceIdx - 1);
//		MDSplus::Data *end = MDSplus::compile(nameBuf, tree);
		MDSplus::Data *end = tree->tdiCompile(nameBuf);
		sprintf(nameBuf, "data(build_path(\'%s\'))[%d:%d]", currPath.c_str(), startSlice + baseSliceIdx,  startSlice + (int)sliceIdx - 1);
//		sprintf(nameBuf, "data(build_path(\'%s\'))[%d:%d]", mdsconvertPath(timePath.c_str()).c_str(), baseSliceIdx, (int)sliceIdx - 1);
//		MDSplus::Data *dim = MDSplus::compile(nameBuf, tree);
		MDSplus::Data *dim = tree->tdiCompile(nameBuf);
		delete[] nameBuf;
		MDSplus::Data *segmentData = new MDSplus::Uint8Array(segment, segSize);
		node->makeSegment(start, end, dim, (MDSplus::Array *)segmentData);
		MDSplus::deleteData(start);
		MDSplus::deleteData(end);
		MDSplus::deleteData(dim);
		MDSplus::deleteData(segmentData);
		delete [] segment;
	    }
	    serializedV.clear();
	    serializedLenV.clear();
	}
//	delete node;
      }catch(MDSplus::MdsException &exc)	
      {
	  throw  UALBackendException(exc.what(),LOG); 
      }
    }

//Gabriele 2017
    void MDSplusBackend::writeApdSlice(MDSplus::Apd *inApd, std::string aosPath, std::string timebasePath, double time)
    {


//oasPath refers to the path in the tree of the AoS root
//if timedPath is empty, timebase will be derived by AoS "time" field. Otherwise it represet the full path in the tree of the timebase.
 	MDSplus::Apd *apd;
	
	//The slice MUST be an array of structures with one single element
// 	//The slice savedCANNOT BE an array of putinapdstructures so IT MUST have as first item a String object

	//Gabriele November 2017. If the slice is empty it is being called from put_non_times and do nothing. Slices will be added afterwards via putSlice 
	if(inApd->len() == 0)
	    return;
/////////////
		
	if(inApd->len() != 1 || inApd->getDescAt(0)->clazz !=  CLASS_APD)
	{
	    std::cout << "INTERNAL ERROR: UNEXPECTED APD IN writeApdSlice\n";
	    return;
	}
	apd = (MDSplus::Apd *)inApd->getDescAt(0);
	
      
        if(!tree)  throw UALBackendException("Pulse file not open",LOG);
    	try {
 	    std::string timePath;
     	    if(timebasePath.empty())
	    { 
	    //first step: extract time from "time" field of the passed AOS and store it in the timePath node as slice
	    	std::string time("time");
		MDSplus::Data *currTimeD = getFromApd(inApd, 0, time);  //Always the first slice (only 1 single time in the slice)
		if(!currTimeD)
		   throw  UALBackendException("Cannot get time information",LOG); 
		double sliceTime = currTimeD->getDouble();
	        int dims[] = {1};
		//Gabriele July 2017
 		//writeSlice(tree, aosPath, "time", "time",  &sliceTime, ualconst::double_data, 0, dims);
 		writeSlice(tree, aosPath, "time", "time",  &sliceTime, ualconst::double_data, 1, dims, true, true);
	        timePath = composePaths(aosPath, time);
	    }
	    else
	        timePath = timebasePath;  //Timebase is already defined somewhere else

	     std::string aos = composePaths(aosPath, "aos");
	    MDSplus::TreeNode *node = getNode(checkFullPath(aos, true).c_str());

	    int apdLen;
//	    unsigned char *apdSerialized = (unsigned char *)inApd->serialize(&apdLen);
//Gabriele June 2020
//dumpStruct((MDSplus::Apd *)apd, 0);
	    deflateApd(apd);
	    unsigned char *apdSerialized = (unsigned char *)apd->serialize(&apdLen);
//	    unsigned char *apdSerialized = (unsigned char *)apd->serialize(&apdLen);
	    int numSegments = node->getNumSegments();
	    if(numSegments > 0)
	    {
		char dtype, nDims;
		int dims[64];
		int leftRows;
		node->getSegmentInfo(numSegments - 1, &dtype, &nDims, dims, &leftRows);
		size_t leftSpace = dims[nDims - 1] - leftRows;
		if(leftSpace > sizeof(int) + apdLen) //Append APD in this segmentData
		{
		    unsigned char *segBuf = new unsigned char[sizeof(int) + apdLen];
		    memcpy(segBuf, &apdLen, sizeof(int));
		    memcpy(&segBuf[sizeof(int)], apdSerialized, apdLen);
		    MDSplus::Data *segData = new MDSplus::Uint8Array(segBuf, sizeof(int) + apdLen);
		    delete []segBuf;
		    MDSplus::Data *startData, *endData;
		    node->getSegmentLimits(numSegments - 1, &startData, &endData);
		    int startIdx, endIdx, dummyIdx;
		    getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
		    getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
		    //int endIdx = endData->getInt();
		    //int startIdx = startData->getInt();
		    MDSplus::deleteData(endData);
		    std::string currTimePath = mdsconvertPath(timePath.c_str());
		    char *nameBuf = new char[128+currTimePath.size()]; //TODO: Check size -- DONE
		    sprintf(nameBuf, "data(build_path(\'%s\'))[%d]", currTimePath.c_str(), endIdx+1);
//		    sprintf(nameBuf, "data(build_path(\'%s\'))[%d]", mdsconvertPath(timePath.c_str()).c_str(), endIdx+1);
//		    endData = MDSplus::compile(nameBuf, tree);
		    endData = tree->tdiCompile(nameBuf);
//		    sprintf(nameBuf, "data(build_path(\'%s\'))[%d : %d]", mdsconvertPath(timePath.c_str()).c_str(), startIdx, endIdx+1);
		    sprintf(nameBuf, "data(build_path(\'%s\'))[%d : %d]", currTimePath.c_str(), startIdx, endIdx+1);
//		    MDSplus::Data *dim = MDSplus::compile(nameBuf, tree);
		    MDSplus::Data *dim = tree->tdiCompile(nameBuf);
		    delete[] nameBuf;
		    node->putSegment((MDSplus::Array *)segData, -1);
		    node->updateSegment(numSegments - 1, startData, endData, dim);
		    MDSplus::deleteData(segData);
		    MDSplus::deleteData(startData);
		    MDSplus::deleteData(endData);
		    MDSplus::deleteData(dim);
		    delete []apdSerialized;
//		    delete node;
		    return;
		}
	    }
	    //From here, a new segment must be built
	    int sliceIdx;
	    if(numSegments == 0)
		sliceIdx = 0;
	    else
	    {
		MDSplus::Data *startData, *endData;
		node->getSegmentLimits(numSegments-1, &startData, &endData);
		int endIdx, dummyIdx;
		getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
		sliceIdx = endIdx + 1;
		//sliceIdx = endData->getInt() + 1;
		MDSplus::deleteData(startData);
		MDSplus::deleteData(endData);
	    }
	    char *nameBuf = new char[timePath.length() + 64];
	    sprintf(nameBuf, "data(build_path(\'%s\'))[%d]", mdsconvertPath(timePath.c_str()).c_str(), sliceIdx);
//	    MDSplus::Data *startData = MDSplus::compile(nameBuf, tree);
	    MDSplus::Data *startData = tree->tdiCompile(nameBuf);
	    delete [] nameBuf;
	    //MDSplus::Data *startData = new MDSplus::Int32(sliceIdx);

	    if(2 * (apdLen + sizeof(int)) < MDSPLUS_SEGMENT_SIZE) 
//If it is likely that more than one slice will be hosted in the segment
	    {
		unsigned char *initBuf = new unsigned char[MDSPLUS_SEGMENT_SIZE];
		memset(initBuf, 0, MDSPLUS_SEGMENT_SIZE);
		MDSplus::Data *initData = new MDSplus::Uint8Array(initBuf, MDSPLUS_SEGMENT_SIZE);
		delete []initBuf;
		node->beginSegment(startData, startData, startData, (MDSplus::Array *)initData);
		MDSplus::deleteData(initData);
		unsigned char *segBuf = new unsigned char[sizeof(int) + apdLen];
		memcpy(segBuf, &apdLen, sizeof(int));
		memcpy(&segBuf[sizeof(int)], apdSerialized, apdLen);
		MDSplus::Data *segData = new MDSplus::Uint8Array(segBuf, sizeof(int) + apdLen);
		delete []segBuf;
		node->putSegment((MDSplus::Array *)segData, -1);
		MDSplus::deleteData(segData);
	    }
	    else //put the whole slice in the whole segmentData
	    {
		unsigned char *segBuf = new unsigned char[apdLen + sizeof(int)];
		memset(segBuf, 0, apdLen + sizeof(int));
		memcpy(segBuf, &apdLen, sizeof(int));
		memcpy(&segBuf[sizeof(int)], apdSerialized, apdLen);
		MDSplus::Data *segData = new MDSplus::Uint8Array(segBuf, apdLen + sizeof(int));
		delete []segBuf;
		node->makeSegment(startData, startData, startData, (MDSplus::Array *)segData);
		MDSplus::deleteData(segData);
	    }

	    MDSplus::deleteData(startData);
	    delete []apdSerialized;

//	    delete node;
 	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }
    
    MDSplus::Apd *MDSplusBackend::readApd(MDSplus::Tree *tree, std::string dataobjectPath, std::string path)
    {
      if(!tree)  throw UALBackendException("Pulse file not open",LOG);
	try {
	    std::string fullPath = composePaths(dataobjectPath, path);
	    MDSplus::TreeNode *node;
	    try {
	    	node = getNode(checkFullPath(fullPath, true).c_str());
	    } catch(MDSplus::MdsException &exc) { return NULL;}
	    if (node->getLength() == 0)  return NULL;
	    MDSplus::Data *retData = node->getData();
	    if(retData->clazz != CLASS_APD)
		throw  UALException("Internal error: array of structure is not an APD data");

	   // std::cout << "readApd" << std::endl;
	   //dumpArrayStruct((MDSplus::Apd *)retData, 0);
//	    delete node;
	    return (MDSplus::Apd *)retData;
	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }

//Gabriele 2021: lazy AoS
   MDSplus::Apd *MDSplusBackend::readDynamicLazyApd(MDSplus::TreeNode *node)
   {
/*	MDSplus::TreeNode *timeNode;
        if(timebasePath == "")
	    timeNode = node->getParent()->getNode("TIME");
	else
	    timeNode = tree->getNode(mdsconvertPath(timebasePath.c_str()).c_str());

        int nTimes;
	MDSplus::Data *timesD = timeNode->data();
	double  *times = timesD->getDoubleArray(&nTimes);
	MDSplus::deleteData(timesD);
	delete [] times;
*/
//Gabriele June 2019: handle the case of empty AoS
        if(strcmp(node->getDType(), "DTYPE_MISSING") == 0)
        {
 	    MDSplus::Apd *retApd = new MDSplus::Apd();
//	    retApd->appendDesc(NULL); October 2019
	    return retApd;
        }
///////////////////////////////////
        try {
	    int numSegments = node->getNumSegments();
	    MDSplus::Data *startData, *endData;
	    //std::cout << "numSegments = " << numSegments << "\n";
	    node->getSegmentLimits(numSegments - 1, &startData, &endData);
	    int endIdx, dummyIdx, nSlices;
	    getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    nSlices = endIdx + 1;
	    MDSplus::deleteData(endData);
	    MDSplus::deleteData(startData);

            MDSplus::Apd *apd = new MDSplus::Apd();
            for(int i = 0; i < nSlices; i++)
	        apd->appendDesc(NULL);
	    apd->appendDesc(new MDSplus::TreeNode(node->getNid(), node->getTree())); //The returned APD has no data, shall 
	    return apd;

	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }
	
    void MDSplusBackend::fillApdSlicesAroundIdx(MDSplus::Apd *apd, int sliceIdx)
    {
        MDSplus::TreeNode *node = (MDSplus::TreeNode *)apd->getDescAt(apd->len() -1);
	if(!node)
	    throw  UALBackendException("Internal error: TreeNode not found in fillApdSlicesArountIdx",LOG);
	//int numSegments = node->getNumSegments();
	//MDSplus::Data *startData, *endData;
	int segIdx, startIdx, endIdx;
	getSegmentIdxFromSliceIdx(node, sliceIdx, segIdx, startIdx, endIdx);

/*	for(segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    node->getSegmentLimits(segIdx, &startData, &endData);
	    int startIdx, endIdx, dummyIdx;
	    getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
	    MDSplus::deleteData(startData);
	    getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    MDSplus::deleteData(endData);
	    if(sliceIdx >= startIdx && sliceIdx <= endIdx)
	    {
*/		MDSplus::Data *segData = node->getSegment(segIdx);
		int serializedLen;
		char *serialized = (char *)segData->getByteUnsignedArray(&serializedLen);
		MDSplus::deleteData(segData);
		int bufIdx = 0;
		int sliceLen;
		for(int idx = 0; idx <= endIdx - startIdx; idx++)
		{
		    memcpy(&sliceLen, &serialized[bufIdx], sizeof(int));
		    bufIdx += sizeof(int);
		    MDSplus::Data *sliceData = MDSplus::deserialize(&serialized[bufIdx]);
		    if(sliceData->clazz != CLASS_APD)
		  	throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
//Gabriele June 2021 -- must inflate
		    inflateApd((MDSplus::Apd *)sliceData);
/////////////////////////
	      	    MDSplus::Data **dscs = apd->getDscArray();
		    if(dscs[startIdx + idx])
		  	throw  UALBackendException("Internal error: unexpected  array of structure found in fillApdSlicesArountIdx",LOG);
	      	    dscs[startIdx + idx] = sliceData;	
		    bufIdx += sliceLen;
		}
		delete[]serialized;

//		break;
//	    }
//	}
    }

////////////Lazy AoS 2021










//Gabriele 2017 
   MDSplus::Apd *MDSplusBackend::readDynamicApd(MDSplus::TreeNode *node) 
    {
        if(!tree)  throw UALBackendException("Pulse file not open",LOG);
//Gabriele June 2019: handle the case of empty AoS
        if(strcmp(node->getDType(), "DTYPE_MISSING") == 0)
        {
 	    MDSplus::Apd *retApd = new MDSplus::Apd();
//	    retApd->appendDesc(NULL); October 2019
	    return retApd;
        }
///////////////////////////////////
        try {
	    MDSplus::Apd *retApd = new MDSplus::Apd();
	    int numSegments = node->getNumSegments();
	    //std::cout << "numSegments = " << numSegments << "\n";
	    for(int segIdx = 0; segIdx < numSegments; segIdx++)
	    {
		MDSplus::Data *startData, *endData;
		node->getSegmentLimits(segIdx, &startData, &endData);
	    	int startIdx, endIdx, dummyIdx;
	    	getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
	    	MDSplus::deleteData(startData);
	    	getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    	MDSplus::deleteData(endData);
		MDSplus::Data *serializedData = node->getSegment(segIdx);
//Gabriele Dec 2019
		char clazz, dtype, nDims;
		short length;
		int *dims;
		char *serialized;
		((MDSplus::Array *) serializedData)->getInfo(&clazz, &dtype, &length, &nDims, &dims, (void **) &serialized);
		if(dtype != DTYPE_B && dtype != DTYPE_BU)
		{
	  	    throw  UALBackendException("INTERNAL ERROR: unexpected dtype in serialized AoS: ", LOG); 
		}
		if(nDims != 1)
		{
	  	    throw  UALBackendException("INTERNAL ERROR: unexpected dimensions  in serialized AoS: ", LOG); 
		}
		int idx = 0;
		for(int sliceIdx = 0; sliceIdx < endIdx - startIdx + 1; sliceIdx++)
		{
		    int sliceLen;
		    memcpy(&sliceLen, &serialized[idx], sizeof(int));
		    idx += sizeof(int);
//Gabriele June 2020
		    MDSplus::Data *sliceData = MDSplus::deserialize(&serialized[idx]);
		    inflateApd((MDSplus::Apd *)sliceData);
//		    MDSplus::Data *sliceData = MDSplus::deserialize(&serialized[idx]);
//dumpStruct((MDSplus::Apd *)sliceData, 0);
		    idx += sliceLen;
		    retApd->appendDesc(sliceData);
		}
		MDSplus::deleteData(serializedData);
/////////////////////
	    }
	    return retApd;
	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }

    void MDSplusBackend::getSegmentIdxFromSliceIdx(MDSplus::TreeNode *node, int sliceIdx, int &retSegmentIdx, int &retStartIdx, int &retEndIdx)
    {
      //int startIdx, endIdx, dummyIdx;
	int numSegments = node->getNumSegments();
	int nid = node->getNid();
	bool found = false;
	std::vector<SegmentDescriptor> segDescV;
	try {
	    segDescV = this->segmentIdxMap.at(nid);
	    found = true;
	}
 	catch (const std::out_of_range& oor) 
	{
	    std::vector<SegmentDescriptor> segIdxDescV;
	    MDSplus::Data *startData, *endData;
	    for(int segIdx = 0; segIdx < numSegments; segIdx++)
	    {
	    	node->getSegmentLimits(segIdx, &startData, &endData);
	    	int startIdx, endIdx, dummyIdx;
	    	getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
	    	MDSplus::deleteData(startData);
	    	getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    	MDSplus::deleteData(endData);
		segIdxDescV.push_back(SegmentDescriptor(segIdx, startIdx, endIdx));
	    }
	    this->segmentIdxMap[nid] = segIdxDescV;
	}
	if(!found)
	{
	    try {
	    	segDescV = segmentIdxMap[nid];
	    }
 	    catch (const std::out_of_range& oor) 
	    {
		throw UALBackendException("Internal error in getSliceAt: expected slice element is missing (check consistency with timebase XXXX)",LOG);
	    }
	}
	for(SegmentDescriptor segDesc : segDescV)
	{
	    if(sliceIdx >= segDesc.startIdx && sliceIdx <= segDesc.endIdx)
	    {
		retSegmentIdx = segDesc.segmentIdx;
		retStartIdx = segDesc.startIdx;
		retEndIdx =segDesc.endIdx;
		return;
	    }
	}
	throw UALBackendException("Internal error in getSliceAt: expected slice element is missing (check consistency with timebase)",LOG);
  }	

/*	int numSegments = node->getNumSegments();
	MDSplus::Data *startData, *endData;
	for(int segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    node->getSegmentLimits(segIdx, &startData, &endData);
	    int startIdx, endIdx, dummyIdx;
	    getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
	    MDSplus::deleteData(startData);
	    getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    MDSplus::deleteData(endData);
	    if(sliceIdx >= startIdx && sliceIdx <= endIdx)
	    {
		retSegmentIdx = segIdx;
		retStartIdx = startIdx;
		retEndIdx = endIdx;
		return;
	    }
	}
	throw UALBackendException("Internal error in getSliceAt: expected slice element is missing (check consistency with timebase)",LOG);
    }
*/
	
    MDSplus::Apd *MDSplusBackend::getApdSliceAt(MDSplus::TreeNode *node, int sliceIdx)
    {
	int numSegments = node->getNumSegments();
	if(numSegments == 0)
	{
	   MDSplus::Apd *apd = new MDSplus::Apd();
	   apd->appendDesc(NULL);
	   return apd; ///XXXXXXXXXXXXXXXXXXXXXXXXXX
	}
	//MDSplus::Data *startData, *endData;
	int segIdx, startIdx, endIdx;
	getSegmentIdxFromSliceIdx(node, sliceIdx, segIdx, startIdx, endIdx);
	/* for(segIdx = 0; segIdx < numSegments; segIdx++)
	{
	    node->getSegmentLimits(segIdx, &startData, &endData);
	    int startIdx, endIdx, dummyIdx;
	    getIndexesInTimebaseExpr(startData, startIdx, dummyIdx);
	    MDSplus::deleteData(startData);
	    getIndexesInTimebaseExpr(endData, endIdx, dummyIdx);
	    MDSplus::deleteData(endData);
	    if(sliceIdx >= startIdx && sliceIdx <= endIdx)
	    {
		//From here slices are stored with their size 
*/
		MDSplus::Data *segData = node->getSegment(segIdx);
		int serializedLen;
		char *serialized = (char *)segData->getByteUnsignedArray(&serializedLen);
		MDSplus::deleteData(segData);
		int bufIdx = 0;
		int sliceLen;
		for(int idx = 0; idx < sliceIdx - startIdx; idx++)
		{
		    memcpy(&sliceLen, &serialized[bufIdx], sizeof(int));
		    bufIdx += sizeof(int) + sliceLen;
		}
		memcpy(&sliceLen, &serialized[bufIdx], sizeof(int));
		bufIdx += sizeof(int);
//Gabriele June 2020
		MDSplus::Data *sliceData = MDSplus::deserialize(&serialized[bufIdx]);
		inflateApd((MDSplus::Apd *)sliceData);

//dumpStruct((MDSplus::Apd *)sliceData, 0);
		delete[]serialized;
		if(sliceData->clazz != CLASS_APD)
		  throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
//Check for the countless variations of slice storage in the past. An array of structures must be returned:
//if it is already (new version) return it as it is. Otherwise build a new APD and 
		MDSplus::Apd *retApd = (MDSplus::Apd *)sliceData;
		if(retApd->len() > 0 && retApd->getDescAt(0) != 0 && retApd->getDescAt(0)->clazz == CLASS_APD)
		    return retApd; //Already an array of structures
		else //Make it an array of descriptors
		{
		    retApd = new MDSplus::Apd();
		    retApd->setDescAt(0, sliceData);
		    return retApd; 
		}
		
//	    }
//	}
//	throw UALBackendException("Internal error in getSliceAt: expected slice element is missing (check consistency with timebase)",LOG);
    }
	

//Gabriele 2017
    MDSplus::Apd *MDSplusBackend::readSliceApd(MDSplus::TreeNode *inNode, std::string timebasePath, double time, int interpolation, std::string currPath)
//aosPath is the complete path from pulsefile root downto the TIMED_*/ITEM_n npde holding the time dependent serialized APD
    {
     	if(!tree)  throw UALBackendException("Pulse file not open",LOG);
    	try {
	    double *timebase;
	    int timebaseLen;
	    MDSplus::TreeNode *timebaseNode;
	    if(timebasePath.empty())
	    {	  
	    	MDSplus::TreeNode *parentNode = inNode->getParent();
	    	timebaseNode = parentNode->getNode(":time");
//	    	timebaseNode = parentNode->getNode(":TIME0");  //Node name is mangled!!
		delete parentNode;
	    }
	    else
	    {
		timebaseNode = getNode(mdsconvertPath(timebasePath.c_str()).c_str());
	    }
	    int nDims, datatype, dims[64];
	    if(!readTimedData(timebaseNode, (void **)&timebase, &datatype, &nDims, dims)) return NULL;
//	    delete timebaseNode;
	    if(datatype != ualconst::double_data || nDims != 1 || dims[0] < 1)
	    	throw UALBackendException("Unexpected time type or dimension in beginAosSliceAction",LOG);

	    switch(interpolation) 
	    {
		case ualconst::closest_interp:
		{
	    	    int sliceIdx;
	    	    timebaseLen = dims[0];
	    	    if(time <= timebase[0] || timebaseLen == 1)
			sliceIdx = 0;
	    	    else if (time >= timebase[timebaseLen - 1])
			sliceIdx = timebaseLen - 1;
	    	    else
	    	    {
			for(sliceIdx = 1; sliceIdx < timebaseLen; sliceIdx++)
			{
		    	    if(timebase[sliceIdx-1] < time && timebase[sliceIdx] >= time)
			    {
				if((time - timebase[sliceIdx-1]) < (timebase[sliceIdx] - time))
				    sliceIdx--;
				break;
			    }
			}
	    	    }
	    	    free((char *)timebase);
	    	    MDSplus::Apd *retApd = getApdSliceAt(inNode, sliceIdx);
 	    	    return retApd;
		}
		case ualconst::previous_interp:
		{
	    	    int sliceIdx;
	    	    timebaseLen = dims[0];
	    	    if(time <= timebase[0] || timebaseLen == 1)
			sliceIdx = 0;
	    	    else if (time >= timebase[timebaseLen - 1])
			sliceIdx = timebaseLen - 1;
	    	    else
	    	    {
			for(sliceIdx = 0; sliceIdx < timebaseLen-1; sliceIdx++)
			{
		    	    if(timebase[sliceIdx] <= time && timebase[sliceIdx+1] > time)
				break;
			}
	    	    }
	    	    free((char *)timebase);
	    	    MDSplus::Apd *retApd = getApdSliceAt(inNode, sliceIdx);
 	    	    return retApd;
		}
		case ualconst::linear_interp:
		{
	    	    int sliceIdx, sliceIdx1 = -1;
	    	    timebaseLen = dims[0];
	    	    if(time <= timebase[0] || timebaseLen == 1)
			sliceIdx = sliceIdx1 = 0;
	    	    else if (time >= timebase[timebaseLen - 1])
			sliceIdx = sliceIdx1 = timebaseLen - 1;
	    	    else
	    	    {
			for(sliceIdx = 0; sliceIdx < timebaseLen-1; sliceIdx++)
			{
		    	    if(timebase[sliceIdx] <= time && timebase[sliceIdx+1] > time)
			    {
				sliceIdx1 = sliceIdx+1;
				break;
			    }
			}
			if(sliceIdx == timebaseLen - 1)
			    sliceIdx1 = sliceIdx;
	    	    }
		    if(sliceIdx == sliceIdx1)
		    {
	    	    	free((char *)timebase);
	    	    	MDSplus::Apd *retApd = getApdSliceAt(inNode, sliceIdx);
 	    	    	return retApd;
		    }
		    else
		    {
	    	    	MDSplus::Apd *apd = getApdSliceAt(inNode, sliceIdx);
	    	    	MDSplus::Apd *apd1 = getApdSliceAt(inNode, sliceIdx1);

			if(!apd || ! apd1) return NULL;
			//if(checkStruct(apd, apd1)) //Gabriele June 2022: let check be performed during interpolation itself
			{
   			    MDSplus::Apd *retApd = MDSplusBackend::interpolateStruct(apd, apd1, time, timebase[sliceIdx], timebase[sliceIdx1], currPath);
	    	    	    free((char *)timebase);
			    MDSplus::deleteData(apd);
			    MDSplus::deleteData(apd1);

		    	    return retApd; //Already an array of structures
			}
		/*	else  //AoS are not compatible (should ever happen)
			{
			    MDSplus::deleteData(apd1);
			    return apd;
			} */
		    }
		}
		default:
		   throw  UALBackendException("INTERNAL ERROR: Invalid Interpolation", LOG);
	    }
	}catch(MDSplus::MdsException &exc)	
	{
	  throw  UALBackendException(exc.what(),LOG); 
	}
    }
	    
    MDSplus::Data *MDSplusBackend::getFromApd(MDSplus::Apd *apd, int idx, std::string path, ArraystructContext *ctx)
    {

//std::cout<<"GET FROM APD path:" << path << "  idx: " << idx << std::endl;
//dumpArrayStruct(apd, 0);

	if(idx >= (int)apd->len())
	  throw UALBackendException("Invalid index in array of structures",LOG);

//Gabriele Feb 20201 Lazy Aos. Check if the Apd is partially filled. In this case the last descriptor is the originating TreeNode
	MDSplus::Data *lastDesc = apd->getDescAt(apd->len() - 1);
	if(lastDesc && lastDesc->clazz == CLASS_S && lastDesc->dtype == DTYPE_NID)
	{
	    if(!apd->getDescAt(idx))
	    {
	    	fillApdSlicesAroundIdx(apd, idx);
	    }
	}
////////////////////////////////////////////////////		



	MDSplus::Apd *currApd = (MDSplus::Apd*)apd->getDescAt(idx);
	if(!currApd)  //Jan 2015 allow holes in struct arrays
          return NULL;
	if(currApd->clazz != CLASS_APD)
	  throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
	size_t prevPos = 0; 
	size_t currPos = 0;
	bool isLast = false;
	std::string currName;

	if(path == "")  //Gabriele November 2017: Only if resolving Dynamic sub AoS
	{
	    if(currApd->len() != 2)
	  	throw  UALBackendException("Internal error: incorrect array of structure for dynamic sub AoS",LOG);
	    MDSplus::Data *retData = currApd->getDescAt(1);
	    return retData;
	}
	while(true)
	{
	    currPos = path.find_first_of("/", currPos);
	    if(currPos == std::string::npos)
	    {
		currName = path.substr(prevPos, path.size() - prevPos);
		isLast = true;
	    }
	    else
	      currName = path.substr(prevPos, currPos - prevPos);

	    
	    int apdIdx;
	    int len = currApd->len();
//	    for(apdIdx = 1; apdIdx < len; apdIdx++)
	    for(apdIdx = 0; apdIdx < len; apdIdx++)
	    {
		MDSplus::Apd *newApd = (MDSplus::Apd *)currApd->getDescAt(apdIdx);
		if(!newApd)
		    continue;
//Gabriele September 2015. Old UAL Compatibility patch : the first descriptor may be a XD descriptor		
		if(newApd->clazz != CLASS_APD)
		{
		    if(apdIdx > 0)
		    	throw UALBackendException("Missing component in array of structues");
		       // throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
		//May not be an internal error in case a Dynamic internal AoS is defined Just below the root of the static AoS
		//in this case the APD has the name followed by NID
		    else
			continue;
		}
		if(newApd->len() == 0)
		    return NULL;

		MDSplus::Data *currNameData = newApd->getDescAt(0);
		char *currNameChar = currNameData->getString();
		if(currName.compare((const char *)currNameChar) == 0)
		{
		    if(isLast)
		    {
		        MDSplus::Data *retData = newApd->getDescAt(1);
			if(ctx && retData && (retData->clazz == CLASS_S && retData->dtype == DTYPE_NID))
			{
			    resolveApdField(newApd, ctx);
			    retData = newApd->getDescAt(1);
			}
		        delete [] currNameChar;
		        return retData;
		    }
		    else
		    {
			currApd = newApd;
			delete[] currNameChar;
		    }
		    break;
		}
		delete [] currNameChar;
	    }
	    if(apdIdx == len)
	    {
//OLD UAL COMPATIBILITY PATCH: Check if currName is the first element of the APD and in case return the following descriptr	      
		MDSplus::Data *currNameData = currApd->getDescAt(0);
		char *currNameChar = currNameData->getString();
		if(currName.compare((const char *)currNameChar) == 0)
		{
		  if(isLast)
		    {
		        MDSplus::Data *retData = currApd->getDescAt(1);
			if(ctx && (retData->clazz == CLASS_S && retData->dtype == DTYPE_NID))
			{
			    resolveApdField(currApd, ctx);
			    retData = currApd->getDescAt(1);
			}
		        delete [] currNameChar;
		        return retData;
		    }
		}
		
		delete [] currNameChar;
		return NULL;
//////////////////////////////////////////////////////////////////////////////////////////////////////////	      

	    } 
	    currPos++;
	    prevPos = currPos;
	}
	//never reached
	return NULL;
    }

//Check for compatibility between two AoS for carrying out interpolation
    bool MDSplusBackend::checkStruct(MDSplus::Apd *apd1, MDSplus::Apd *apd2)
    {
        int len1 = apd1->len();
	int len2 = apd2->len();
	if(len1 != len2)
	    return false;
	if(len1 < 1) return true;
	if(apd1->getDescAt(0) &&  (apd1->getDescAt(0)->clazz != CLASS_APD)) //If is part of the APD tree and not a recursive AoS
	    return checkStructRec(apd1, apd2);
	for(int idx = 0; idx < len1; idx++)
	{
            if(apd1->getDescAt(idx) == 0 && apd2->getDescAt(idx) == 0)
		continue;
	    else if (apd1->getDescAt(idx) == 0 || apd2->getDescAt(idx) == 0)
		return false;
	    if(!checkStructRec((MDSplus::Apd*)apd1->getDescAt(idx), (MDSplus::Apd*)apd2->getDescAt(idx)))
		return false;
	}
	return true;
    }
    
    bool MDSplusBackend::checkStructRec(MDSplus::Apd *apd1, MDSplus::Apd *apd2)
    {
	int len1 = apd1->len();
	int len2 = apd2->len();
	if(len1 != len2) return false;
	if(len1 < 1) return false;
	char *name1 = apd1->getDescAt(0)->getString();
	char *name2 = apd2->getDescAt(0)->getString();
	if(strcmp(name1, name2))
	{
	    delete[] name1;
	    delete[] name2;
	    return false;
	}
	delete[] name1;
	delete[] name2;
	for(int idx1 = 1; idx1 < len1; idx1++)
	{
	    if(!checkStructItem(apd1->getDescAt(idx1), apd2->getDescAt(idx1)))
		return false;
	}
	return true;
    }
    bool MDSplusBackend::checkStructItem(MDSplus::Data *item1, MDSplus::Data *item2)
    {
	 if((item1 && !item2)||(item2 && !item1)) return false;
	 if(!item1 && !item2) return true;
         if(item1->clazz != item2->clazz || item1->dtype != item2->dtype)
	     return false;
	 if(item1->clazz == CLASS_APD)
	     return checkStruct((MDSplus::Apd *)item1, (MDSplus::Apd *)item2);
	 if(item1->clazz == CLASS_A && ((MDSplus::Array *)item1)->getSize() != ((MDSplus::Array *)item2)->getSize())
	     return false;
	 return true;
    }
    
//AoS or Struct interpolation
    MDSplus::Apd *MDSplusBackend::interpolateStruct(MDSplus::Apd *apd1, MDSplus::Apd *apd2, double t, double t1, double t2, std::string currPath) //The two AoS are already checked
    {
          MDSplus::Apd *interpApd = new MDSplus::Apd();
          int len = apd1->len();
          int len1 = apd2->len();
	  if(len != len1)
	  {
	      std::cout << "WARNING: Linear interpolation not possible (different number of elements) for node "+currPath << std::endl;
	      return interpApd;
	  }
	  if(len == 0) 
	      return interpApd;

	  if(apd1->getDescAt(0) != NULL && apd1->getDescAt(0)->clazz != CLASS_APD)
	  {
	      if(!(apd2->getDescAt(0) != NULL && apd2->getDescAt(0)->clazz != CLASS_APD))
	      {
	          std::cout << "WARNING: Linear interpolation not possible for node "+currPath << std::endl;
		  return interpApd;
	      }
//At this point it is a Struct
	      return interpolateStructRec(apd1, apd2, t, t1, t2, currPath, false);
	  }
//At this point it is an AoS
	  for(int idx = 0; idx < len; idx++)
	  {
	      interpApd->appendDesc(interpolateStructRec((MDSplus::Apd*)apd1->getDescAt(idx), (MDSplus::Apd*)apd2->getDescAt(idx), t, t1,t2, currPath+"["+std::to_string(idx)+"]", true));
	  }
	  return interpApd;
    }
    //This will handle APDs where the first element is the item name, i.e. Struct
    MDSplus::Apd *MDSplusBackend::interpolateStructRec(MDSplus::Apd *apd1, MDSplus::Apd *apd2, double t, double t1, double t2, std::string currPath, bool firstRec)
    {
	if(!apd1 || !apd2)
	    return  NULL;
	int len1 = apd1->len(); //already checked
	int len2 = apd2->len(); //already checked
	if(apd1->len() < 2 || apd2->len() < 2)
	{
	    std::cout << "WARNING: Linear interpolation not possible for node "+currPath << std::endl;
	    return NULL;
	}
	MDSplus::Data *name1 = apd1->getDescAt(0);
	MDSplus::Data *name2 = apd1->getDescAt(0);
	if(name1->clazz != CLASS_S || name2->clazz != CLASS_S)
	{
	    std::cout << "INTERNAL ERROR: inconsistent AoS structure 1" << std::endl;
	    return NULL;
	}
	char *nameStr1 = name1->getString();
	char *nameStr2 = name2->getString();
	if(strcmp(nameStr1, nameStr2))
	{
	    std::cout << "WARNING: Linear interpolation not possible for node "+currPath << std::endl;
	    delete [] nameStr1;
	    delete[] nameStr2;
	    return NULL;
	}
//At this point the names match
        MDSplus::Apd *interpApd = new MDSplus::Apd();
	interpApd->appendDesc(new MDSplus::String(nameStr1));
	if(!firstRec) currPath += "."+std::string(nameStr1); //Just to avoid field name duplications
	delete [] nameStr1;
	delete[] nameStr2;
	if(apd1->getDescAt(1)->clazz != CLASS_APD) //It is not a directory but contains a datum
	{
	    if(apd2->getDescAt(1)->clazz == CLASS_APD) //The other one is a directory or an AoS, inconsistent
	    {
	    	std::cout << "WARNING: Linear interpolation not possible for node " +currPath<< std::endl;
		MDSplus::deleteData(interpApd);
		return NULL;
	    }
	    interpApd->appendDesc(interpolateStructItem(apd1->getDescAt(1), apd2->getDescAt(1), t, t1, t2, currPath));
	    return interpApd;
	}
	//At this point it may be either a subdirectory or an AoS
	MDSplus::Apd *currItem1 = (MDSplus::Apd* )apd1->getDescAt(1);
	if(currItem1->len() > 0 && currItem1->getDescAt(0)->clazz == CLASS_APD) //If the field is an AoS
	{
	    MDSplus::Apd *currItem2 = (MDSplus::Apd* )apd2->getDescAt(1);
	    if(!(currItem2->len() > 0 && currItem2->getDescAt(0)->clazz == CLASS_APD)) //If the field is NOT an AoS, inconsistent
	    {
	    	std::cout << "WARNING: Linear interpolation node possible for node "+currPath << std::endl;
		MDSplus::deleteData(interpApd);
		return NULL;
	    }
//At this point both are AoS
	    interpApd->appendDesc(interpolateStruct((MDSplus::Apd *)apd1->getDescAt(1), (MDSplus::Apd *)apd2->getDescAt(1), t, t1, t2, currPath));
	    return interpApd;
	}

//At this point we are handling a subdirectory
	for(int idx1 = 1; idx1 < len1; idx1++)
	{
	    if(!apd1->getDescAt(idx1))
	    	continue;
	    MDSplus::Apd *currApd1 = (MDSplus::Apd *)apd1->getDescAt(idx1);
	    if(currApd1->len() < 2 || !currApd1->getDescAt(1))
		continue;
	    if(currApd1->getDescAt(0)->clazz != CLASS_S)
	    {
	    	std::cout << "INTERNAL ERROR: inconsistent AoS structure 2.1" << std::endl;
		return NULL;
	    }
	    MDSplus::Data *currName1 = currApd1->getDescAt(0);
	    char *currNameStr1 = currName1->getString();
	    int idx2 ;
	    for(idx2 = 1 ; idx2 < len2; idx2++)
	    {
	    	if(!apd2->getDescAt(idx2))
	    	    continue;
		if(apd2->getDescAt(idx2)->clazz != CLASS_APD)
		{
			std::cout << "INTERNAL ERROR: inconsistent AoS structure 3" << std::endl;
			return NULL;
		}
		MDSplus::Apd *currApd2 = (MDSplus::Apd *)apd2->getDescAt(idx2);
		if(currApd2->len() < 2 || !currApd2->getDescAt(1))
		{
		    continue;
		}
	    	MDSplus::Data *currName2 = currApd2->getDescAt(0);
		if(currName2->clazz != CLASS_S)
		{
			std::cout << "WARING: Linear interpolation not possible for node "+currPath  << std::endl;
			return NULL;
		}
	    	char *currNameStr2 = currName2->getString();
		if(!strcmp(currNameStr1, currNameStr2))
		{
		    interpApd->appendDesc(interpolateStructRec(currApd1, currApd2, t, t1, t2, currPath, false));
		    delete [] currNameStr2;
		    break;
		}
		delete [] currNameStr2;
	    }
	    delete [] currNameStr1;
	}
	return interpApd; 
    }
/*    MDSplus::Apd *MDSplusBackend::interpolateStructRecOLD(MDSplus::Apd *apd1, MDSplus::Apd *apd2, double t, double t1, double t2)
    {
	if(!apd1 || !apd2)
	    return  NULL;
	int len1 = apd1->len(); //already checked
	int len2 = apd2->len(); //already checked
        MDSplus::Apd *interpApd = new MDSplus::Apd();
	if(len1 != len2)
	{
	    std::cout << "WARNING: interpolation requested for inconsistent AoS" << std::endl;
	    return interpApd;
	}
	MDSplus::Data *name = apd1->getDescAt(0);
	char *nameStr = name->getString();
	interpApd->appendDesc(new MDSplus::String(nameStr));
	delete[] nameStr;
 	for(int idx1 = 1; idx1 < len1; idx1++)
	{
	    interpApd->appendDesc(interpolateStructItem(apd1->getDescAt(idx1), apd2->getDescAt(idx1), t, t1, t2));
	}
	return interpApd; 
    }
*/	
    MDSplus::Data *MDSplusBackend::interpolateStructItem(MDSplus::Data *item1, MDSplus::Data *item2, double t, double t1, double t2, std::string currPath)
    {
//Note: items have already been checked for compatibility
         if(item1->clazz == CLASS_APD)
	 {
	     if(item2->clazz != CLASS_APD)
	     {
	         std::cout << "WARNING: Linear interpolation not possible for node "+currPath << std::endl;
		 return NULL;
	     }
	     return interpolateStruct((MDSplus::Apd *)item1, (MDSplus::Apd *)item2, t, t1, t2, currPath);
	 }
	 else if(item1->clazz == CLASS_S)
	 {
	     if(item2->clazz != CLASS_S)
	     {
	      	std::cout << "WARNING: Linear interpolation not possible for node "+currPath  << std::endl;
		return NULL;
	     }
	     if(item1->dtype != item2->dtype)
	     {	
	         std::cout << "WARNING: interpolation requested for inconsistent AoS at "+currPath  << std::endl;
		 return NULL;
	     }
	     switch(item1->dtype)  {
	       case DTYPE_L: 
	       {
		   int i1 = item1->getInt();
		   int i2 = item2->getInt();
		   int i = i1+(i2 - i1)*(t-t1)/(t2-t1);
		   return new MDSplus::Int32(i);
	       }
 	       case DTYPE_DOUBLE: 
	       {
		   double d1 = item1->getDouble();
		   double d2 = item2->getDouble();
		   double d = d1+(d2 - d1)*(t-t1)/(t2-t1);
		   return new MDSplus::Float64(d);
	       }
	       case DTYPE_FTC:
	       {
		   std::complex<double>c1 = item1->getComplex();
		   std::complex<double>c2 = item2->getComplex();
		   std::complex<double>c = c1+(c2 - c1)*(t-t1)/(t2-t1);
		   return new MDSplus::Complex64(c.real(), c.imag());
	       }
	       default: //MUST never happen
		 std::cout << "INTERNAL ERROR: Unexpected type in interpolation" << std::endl;
		 return NULL;
	     }
	 }
	 else if(item1->clazz == CLASS_A)
	 {
	     if(item2->clazz != CLASS_A)
	     {
	         std::cout << "WARNING: Linear interpolation not possible for node "+currPath  << std::endl;
		 return NULL;
	     }
	     if(item1->dtype != item2->dtype)
	     {
	         std::cout << "WARNING: Linear interpolation not possible for node "+currPath  << std::endl;
		 return NULL;
	     }
	     int len;
	     int nDims, *dims;
	     int nDims1, *dims1;
	     dims = ((MDSplus::Array *)item1)->getShape(&nDims);
	     dims1 = ((MDSplus::Array *)item2)->getShape(&nDims1);
	     if(nDims != nDims1)
	     {
	         std::cout << "WARNING: Linear interpolation not possible for node "+currPath  << std::endl;
	     	 delete [] dims;
	     	 delete [] dims1;
		 return NULL;
	     }
	     for(int i = 0; i < nDims; i++)
	     {
		if (dims[i] != dims1[i])
	     	{
	             std::cout << "WARNING: Linear interpolation not possible for node "+currPath  << std::endl;
	     	     delete [] dims;
	     	     delete [] dims1;
		     return NULL;
	     	}
	     }
	     MDSplus::Data *interpData;
	     switch(item1->dtype)  {
	       case DTYPE_L: 
	       {
		   int *i1 = item1->getIntArray(&len);
		   int *i2 = item2->getIntArray(&len);
		   int *i = new int[len];
		   for(int idx = 0; idx < len; idx++)
		   {
		       i[idx] = i1[idx]+(i2[idx] - i1[idx])*(t-t1)/(t2-t1);
		   }
		   interpData = new MDSplus::Int32Array(i, nDims, dims);
		   delete [] i;
		   break;
	       }
 	       case DTYPE_DOUBLE: 
	       {
		   double *d1 = item1->getDoubleArray(&len);
		   double *d2 = item2->getDoubleArray(&len);
		   double *d = new double[len];
		   for(int idx = 0; idx < len; idx++)
		   {
		       d[idx] = d1[idx]+(d2[idx] - d1[idx])*(t-t1)/(t2-t1);
		   }
		   interpData = new MDSplus::Float64Array(d, nDims, dims);
		   delete [] d;
		   break;
	       }
	       case DTYPE_FTC:
	       {
		   std::complex<double> *c1 = item1->getComplexArray(&len);
		   std::complex<double> *c2 = item2->getComplexArray(&len);
		   std::complex<double> *c = new std::complex<double>[len];
		   for(int idx = 0; idx < len; idx++)
		   {
		       c[idx] = c1[idx]+(c2[idx] - c1[idx])*(t-t1)/(t2-t1);
		   }
		   double *reim = new double[2*len];
		   for(int idx = 0; idx < len; idx++)
		   {
		       reim[2*idx] = c[idx].real();
		       reim[2*idx+1] = c[idx].imag();
		   }
// 		   interpData = new MDSplus::Complex64Array(reim, nDims, dims);
		   delete [] reim;
		   break;
	       }
	       case DTYPE_B:
	       case DTYPE_BU: //for strings take closest
	       {
		  char *bytes; 
		  if(t - t1 < t2 - t)
		  {
		      bytes = item1->getByteArray(&len);
		      interpData = new MDSplus::Int8Array(bytes, nDims, dims);
		  }
		  else
		  {
		      bytes = item2->getByteArray(&len);
		      interpData = new MDSplus::Int8Array(bytes, nDims, dims);
		  }
		  delete [] bytes;
		  break;
	       }
	       default: //MUST never happen
		 std::cout << "INTERNAL ERROR: Unexpected type in interpolation" << std::endl;
		 return NULL;
	     }
	     delete [] dims;
	     delete [] dims1;
	     return interpData;
	 }	
	 std::cout << "INTERNAL ERROR: Unexpected class in interpolation" << std::endl;
	 return NULL;
    }
	 
 //Gabriele 2017
    void MDSplusBackend::insertNewInApd(ArraystructContext *ctx, std::string rootName, MDSplus::Apd *apd, int idx, std::string path, std::string timebasePath, bool isSlice, MDSplus::Data *mdsData, void* data, int datatype, int dim, int* size)
//If not empty, path and timebase path contain already the full path to the nodes going to contain timed data andtime
    {
//std::cout << "INSERT NEW IN APD: " << rootName << "   " << path << "   "  << timebasePath << "     " << idx << std::endl;

	MDSplus::Apd *currApd;
	if(!apd->hasDescAt(idx))
	{
	    apd->setDescAt(idx, currApd = new MDSplus::Apd());
	    currApd->appendDesc(new MDSplus::String(rootName.c_str()));
	}
	if(path == "" && mdsData) //Inserting directly the item (for node reference to dynamic AoS
	{
	    currApd = (MDSplus::Apd *)apd->getDescAt(idx);   //Gabriele Dec 2017
	    currApd->appendDesc(mdsData);
	}
	else
	    insertInApd(ctx, apd, idx, path, timebasePath, isSlice, mdsData, data, datatype, dim, size);
//	    insertInApd(ctx, apd, idx, path, timebasePath, isSlice, NULL, data, datatype, dim, size);
    }
    void MDSplusBackend::insertInApd(ArraystructContext *ctx, MDSplus::Apd *apd, int idx, std::string path, std::string timebasePath, bool isSlice, MDSplus::Data *mdsData, void* data, int datatype, int dim, int* size)
    {
//std::cout << "INSERT IN APD: " << apd->decompile() << "   " << path << "   "  << timebasePath << "     " << idx << std::endl;
//If not empty, path and timebase path contain already the full path to the nodes going to contain timed data and time
	size_t apdIdx;
	MDSplus::Apd *currApd;
	//if(idx >= (int)apd->len() || apd->getDescAt(idx) == 0)
	if(!apd->hasDescAt(idx))
	  apd->setDescAt(idx, currApd = new MDSplus::Apd());
	currApd = (MDSplus::Apd *)apd->getDescAt(idx);
	if(currApd->clazz != CLASS_APD)
	  throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);

	std::string currName;
	size_t prevPos = 0; 
	size_t currPos = 0;
	while(true)
	{
	    currPos = path.find_first_of("/", currPos);
	    if(currPos == std::string::npos)
	    {
		currName = path.substr(prevPos, path.size() - prevPos);
		break;
	    }
	    currName = path.substr(prevPos, currPos - prevPos);
	    size_t len = currApd->len();
	    for(apdIdx = 1; apdIdx < len; apdIdx++) 
	    {
		MDSplus::Apd *runApd = (MDSplus::Apd *)currApd->getDescAt(apdIdx);
		if(runApd->clazz != CLASS_APD)
		    throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
		MDSplus::Data *currNameData = runApd->getDescAt(0);
		char *currNameChar = currNameData->getString();
		if(currName.compare((const char *)currNameChar) == 0)
		{
		    currApd = runApd;
		    delete [] currNameChar;
		    break;
		}
		//delete currNameData; PER ORA
		delete []currNameChar;
	    }
	    if(apdIdx >= len) //The name has to be added to current apd
	    {
		MDSplus::Apd *newApd = new MDSplus::Apd();
	        currApd->appendDesc(newApd);
		newApd->appendDesc(new MDSplus::String(currName.c_str()));
		currApd = newApd;
	    }
	    currPos++;
	    prevPos = currPos;
	}
	//Now it is necessary to append the last part of the name associated with the passed Data instance
	for(apdIdx = 1; apdIdx < currApd->len(); apdIdx++)  
	{
	    MDSplus::Apd *runApd = (MDSplus::Apd *)currApd->getDescAt(apdIdx);
	    if(!runApd) continue;
	    MDSplus::Data *currNameData = runApd->getDescAt(0);
	    char *currNameChar = currNameData->getString();
	    if(!currName.compare((const char *)currNameChar))
	    {
//This should not occur....
printf("Warning, struct field added more than once\n");

		if(mdsData)
		    currApd->setDescAt(0, mdsData);
		else
		{
		    MDSplus::Data *newData = assembleData(data, datatype, dim, size);
		    currApd->setDescAt(0, newData);
		    //MDSplus::deleteData(newData);
		}
		delete [] currNameChar;
		break;
	    }
	    delete []currNameChar;
	}

	if(apdIdx >= currApd->len()) //Still to add pair name, data
	{
	    MDSplus::Apd *newApd = new MDSplus::Apd();
	    currApd->appendDesc(newApd);
	    newApd->appendDesc(new MDSplus::String(currName.c_str()));
	    if(timebasePath.empty()) //if it is a non timed field
	    {
		if(mdsData)
		    newApd->appendDesc(mdsData);
		else
		{
		    MDSplus::Data *newData = assembleData(data, datatype, dim, size);
		    newApd->appendDesc(newData);
		}
	    }
	    else //Adding a timed field, stored in a separate node
	    {
		//First check if timebase path refers to a node external or internal the AoS. If it refers to an internal 
		//node of the AoS, it is translated to the corresponding TIME_n/AOS node
		std::string currTimebasePath = relativeToAbsolutePath(ctx, timebasePath, idx);
		bool isInternalTime = false;
		if(currTimebasePath.find_first_of('[') != std::string::npos) //If it is the reference of an internal AoS field
		{
		  currTimebasePath = getTimedNode(ctx, currTimebasePath, false);
		    isInternalTime = true;
		}
 		std::string timedPath = getTimedNode(ctx, path, idx, false);
		if(isSlice)
		{
//Gabriele July 2017. writeSlice expects an additional dimension (=slices to be written) from high level
		    int *newSize = new int[dim];
		    int rowSamples=1;
		    for(int i = 0; i < dim; i++)
		    	newSize[i] = size[i];
//Gabriele Febrarry 2021
		    for(int i = 0; i < dim - 1; i++)
			rowSamples *= size[i];
		    int numSlices = size[dim-1];
		    newSize[dim-1] = 1;
 		    int sampleSize = 0;
		    switch(datatype)  {
			case ualconst::char_data: sampleSize = 1; break;
			case ualconst::integer_data: sampleSize = 4; break;
			case ualconst::double_data: sampleSize = 8; break;
			case ualconst::complex_data: sampleSize = 16; break;
		    }
		    char *charData = (char *)data;
		    //size[dim++] = 1;
//////////////////////////////////////////////////////////
//Gabriele November 2017 In case this is being written by put_non_timed. Slices will be added afterwards
		   
///////////////////////////////////////////////////////////
 //  		    	writeSlice(tree, ctx->getDataobjectName(), timedPath, currTimebasePath, data, datatype, dim - 1, newSize, true, isInternalTime);
		    for(int i = 0; i < numSlices; i++)
		        writeSlice(tree, ctx->getOperationContext()->getDataobjectName(), timedPath, currTimebasePath, &charData[i*rowSamples*sampleSize], datatype, dim, newSize, true, isInternalTime);
		   // delete[] newSize;

		}
		else
		{
		    writeTimedData(tree, ctx->getOperationContext()->getDataobjectName(), timedPath, currTimebasePath, data, datatype, dim, size, true, isInternalTime);
		}
		try {
		  std::string currPath = composePaths(ctx->getOperationContext()->getDataobjectName(), timedPath);
		    MDSplus::TreeNode *node = getNode(checkFullPath(currPath, true).c_str(), true); //In this case the node will be freed, so it cannot be reused!!
		    newApd->appendDesc(node);

//std::cout << newApd->decompile() << std::endl;
//std::cout << "*********************" << std::endl;
//std::cout << apd->decompile() << std::endl;


		}catch(MDSplus::MdsException &exc)
		{
		    std::cout << "INTERNAL ERROR: NODE NOT FOUND IN insertApd for timed fields" << std::endl;
		}
	    }
	}
//	dumpArrayStruct(apd, 0);


    }
      
    void MDSplusBackend::getIndexesInTimebaseExpr(MDSplus::Data *timebase, int &idx1, int &idx2)
    {
	if(timebase->dtype != DTYPE_FUNCTION)
	  throw  UALBackendException("Internal error: incorrect timebase type of array of structure",LOG);
        MDSplus::Data *idxs = ((MDSplus::Function *)timebase)->getArgumentAt(1);
	if(idxs->dtype == DTYPE_RANGE) 
	{
	    MDSplus::Range *rangeD = (MDSplus::Range *)idxs;
	    MDSplus::Data *startD = rangeD->getBegin();
	    MDSplus::Data *endD = rangeD->getEnding();
	    idx1 = startD->getInt();
	    idx2 = endD->getInt();
	    MDSplus::deleteData(startD);
	    MDSplus::deleteData(endD);
	    MDSplus::deleteData(rangeD);
	}
	else //A single index
	{
	    idx1 = idxs->getInt();
	    idx2 = -1;
	    MDSplus::deleteData(idxs);
	}
    }

//Get the full path from tree top to the AoS root
    std::string MDSplusBackend::getTopAoSPath(ArraystructContext *ctx)
    {
	ArraystructContext *currCtx = ctx;
	while(currCtx->getParent())
	    currCtx = currCtx->getParent();
	return currCtx->getOperationContext()->getDataobjectName() + "/" + currCtx->getPath();
    }


//Get the name of the AoSroot
    std::string MDSplusBackend::getTopAoSName(ArraystructContext *ctx)
    {
	ArraystructContext *currCtx = ctx;
	while(currCtx->getParent())
	    currCtx = currCtx->getParent();
	return currCtx->getPath();
    }


//Get the internal path from the IDS root to the current AoS including indexes of traversed AoS (empty string for AoS root)
 /*   std::string MDSplusBackend::getAoSFullPath(ArraystructContext *ctx, int idx)
    {
	char buf[16];
	sprintf(buf, "[%d]/", idx);
	std::string retPath = ctx->getPath();
	retPath += buf;
	ArraystructContext *currCtx = ctx;
	while(currCtx->getParent())
	{
	    sprintf(buf, "[%d]", currCtx->getIndex());
	    currCtx = currCtx->getParent();
	    retPath = ((currCtx->getPath()+buf)+"/")+retPath;
	}
	return retPath;
    }
*/

    std::string MDSplusBackend::getAoSFullPath(ArraystructContext *ctx, int idx)
    {
	char buf[32]; //TODO: Check size -- DONE
	sprintf(buf, "[%d]/", idx);
	std::string retPath = ctx->getPath();
	retPath += buf;
	ArraystructContext *currCtx = ctx;
	while(currCtx->getParent())
	{
	    sprintf(buf, "[%d]", currCtx->getParent()->getIndex());   //Gabriele March 2018
//	    sprintf(buf, "[%d]", currCtx->getIndex());
	    currCtx = currCtx->getParent();
	    retPath = ((currCtx->getPath()+buf)+"/")+retPath;
	}
	return retPath;
    }


    std::string MDSplusBackend::toLower(std::string inS)
    {
	std::string s(inS);
	for(size_t i = 0; i < s.size(); i++)
	    s[i] = tolower(s[i]);
	return s;
    }

//Get the full path from tree top of the node selected to contain the time dependent item (timedependent field os serialed dynami AoS)
    std::string MDSplusBackend::getNodePathFor(std::string aosPath, std::string aosFullPath, std::string aosName, bool isAos)
    {
/*	//Check if already assigned
 	try {
	    return timedNodePathMap.at(toLower(aosFullPath));
	}
 	catch (const std::out_of_range& oor) {}
	int idx;
	try {
	    idx = timedNodeFreeIdxMap[toLower(aosPath)];
	}
 	catch (const std::out_of_range& oor) 
	{
	    idx = maxAosTimedId;
	    maxAosTimedId++;
	    timedNodeFreeIdxMap[toLower(aosPath)] = idx;
	}
*/

    std::unordered_map<std::string, int> * nodeFreeIdxMap;
	int *maxAosId;
	std::unordered_map<std::string, std::string> * nodePathMap;

	if (isAos) {
	  nodeFreeIdxMap = &timeaNodeFreeIdxMap;
	  maxAosId = &maxAosTimeaId;
	  nodePathMap = &timeaNodePathMap;
	} else {
	  nodeFreeIdxMap = &timedNodeFreeIdxMap;
	  maxAosId = &maxAosTimedId;
	  nodePathMap = &timedNodePathMap;
	}
	
    auto searchPath = nodePathMap->find(toLower(aosFullPath));
	if (searchPath != nodePathMap->end())
	    return searchPath->second;
	int idx;
    auto search = nodeFreeIdxMap->find(toLower(aosPath));
	if (search == nodeFreeIdxMap->end())
	{
	    idx = (*maxAosId);
	    (*maxAosId)++;
	    (*nodeFreeIdxMap)[toLower(aosPath)] = idx;
	}
	else
	{
	    idx = search->second;
	}

//////////////////////////////////////////////////////////////////////////////////////
	(*nodeFreeIdxMap)[toLower(aosPath)] = idx + 1;
	char *buf = new char[aosName.size()+48];
	if (isAos) {
	  sprintf(buf, "%s/timed_aos/group_%d/item_%d", aosName.c_str(), idx/1000+1, idx + 1);
        } else {
	  sprintf(buf, "%s/timed_data/group_%d/item_%d", aosName.c_str(), idx/1000+1, idx + 1);
        }
	std::string retPath(buf);
	delete []buf;
        (*nodePathMap)[toLower(aosFullPath)] = retPath;
	return retPath;
     }

//Used to get the path of TIMED_*/ITEM_n node from field and index (referred to innermost AoS)
std::string MDSplusBackend::getTimedNode(ArraystructContext *ctx, std::string field, int idx, bool isAos)
    {
	std::string fullAosPath = getAoSFullPath(ctx, idx)+field;
	std::string topAosPath = getTopAoSPath(ctx);
	std::string topAosName = getTopAoSName(ctx);
	return getNodePathFor(topAosPath, fullAosPath, topAosName, isAos);
    }

//Used to get the path of TIMED_*/ITEM_n node from the complete path referred to AoS root
std::string MDSplusBackend::getTimedNode(ArraystructContext *ctx, std::string fullAosPath, bool isAos)
    {
	std::string topAosPath = getTopAoSPath(ctx);
	std::string topAosName = getTopAoSName(ctx);
	return getNodePathFor(topAosPath, fullAosPath, topAosName, isAos);
    }

    std::string MDSplusBackend::relativeToAbsolutePath(ArraystructContext *ctx, std::string relPath, int idx)
    {
//Gabriele May 2017: if relPath starts with '/', it is already an absolute path from the IDS root
	if(relPath[0] == '/')
	    return relPath;
/////////////////////////////////////////////////////////////////////////////////
//	std::string absolutePath = getAoSFullPath(ctx, 0);
	std::string absolutePath = getAoSFullPath(ctx, idx);
	if(absolutePath[absolutePath.size()-1] == '/')
	    absolutePath = absolutePath.substr(0, absolutePath.size() - 1);
	std::string currPath = relPath;
//	std::size_t currRelPos = currPath.find_first_of("../");
	std::size_t currRelPos = currPath.find("../");
	while(currRelPos != std::string::npos)
	{
	    std::size_t currAbsPos = absolutePath.find_last_of("/");
	    if(currAbsPos == std::string::npos) //it is the root component
		absolutePath = "";
	    else
	    	absolutePath = absolutePath.substr(0, currAbsPos);
	    currPath = currPath.substr(currRelPos+3);
	    currRelPos = currPath.find("../");
	}
	if(absolutePath.empty())
	    return currPath;
	return (absolutePath+"/")+currPath;
    }

 	
//Reset maps used by getNodePathFor()
    void MDSplusBackend::resetNodePath()
    {
	timedNodeFreeIdxMap.clear();
	timedNodePathMap.clear();
	maxAosTimedId = 0;
	//timedNodePathMap.clear();
	timeaNodeFreeIdxMap.clear();
	timeaNodePathMap.clear();
	maxAosTimeaId = 0;
    }

//TEMPORARY FOR TESTS
//Reset maps used by getNodePathFor()
    void MDSplusBackend::fullResetNodePath()
    {
	timedNodeFreeIdxMap.clear();
	timedNodePathMap.clear();
	maxAosTimedId = 0;
	timeaNodeFreeIdxMap.clear();
	timeaNodePathMap.clear();
	maxAosTimeaId = 0;
    }




///////////////////////////////////////////////////
/////////////// Public part ///////////////////////
///////////////////////////////////////////////////
	/**
     Init a given back-end object.
     @param[in] id backend identifier
     @result pointer on backend object
  */
  Backend* MDSplusBackend::initBackend(int id)
  {  return new MDSplusBackend; }

  void MDSplusBackend::openPulse(DataEntryContext *ctx,
			 int mode)
    {
      std::string options = ctx != nullptr ? ctx->getOptions() : "";

 	  // Extract MDSplus options
	  char szOption[256] = { 0 };
	  char szTree[256] = { 0 };
	  
	  // By default use "NORMAL" mode
	  strcpy(szOption, DEF_NORMALMODE);
	  
	  // By default use "ids" tree name
	  strcpy(szTree, DEF_TREENAME);
	  
	  std::string strValue;
	  std::map<std::string, std::string> mapOptions;
	  if (extractOptions(options, mapOptions) > 0)
	  {
		  // Open tree in readonly mode requested? 
		  if (isOptionExist("readonly", mapOptions, strValue))
		  {
			  strcpy(szOption, DEF_READONLYMODE);
		  }
		  // Open a specific tree name?
		  if (isOptionExist(DEF_TREENAME, mapOptions, strValue) && strValue.length() > 0)
		  {
			  strcpy(szTree, strValue.c_str());
		  }
	  }
	  
	  std::string mdsplusBaseStr = ctx->getPath();
	  if(mode == CREATE_PULSE || mode == FORCE_CREATE_PULSE)
		create_directories(mdsplusBaseStr.c_str());
		
	  setDataEnv(ctx); 
    	  int shotNum = getMdsShot(ctx->getShot(), ctx->getRun(), true, szTree);
		  
	  switch(mode) {
	    case ualconst::open_pulse:
	    case ualconst::force_open_pulse:
	          try {
	              tree = new MDSplus::Tree(szTree, shotNum, szOption); break;
		  }catch(MDSplus::MdsException &exc)
		  {
                    resetIdsPath(szTree);
		    throw  UALBackendException(exc.what(),LOG); 
		  }
		  break;
	    case ualconst::create_pulse:
	    case ualconst::force_create_pulse:
	          try {
		      MDSplus::Tree *modelTree = new MDSplus::Tree(szTree, -1, DEF_READONLYMODE);
		      modelTree->createPulse(shotNum);
		      delete modelTree;
		      tree = new MDSplus::Tree(szTree, shotNum, szOption);
		      saveVersion(tree);
		  }catch(MDSplus::MdsException &exc)
		  {
                    resetIdsPath(szTree);
		    throw UALBackendException(exc.what(),LOG); 
		  }
		  break;
	    default:
              resetIdsPath(szTree);
	      throw  UALBackendException("Mode not yet supported",LOG);
	  
	  }
	  treeNodeMap.clear();
          resetIdsPath(szTree);
      }

      
  void MDSplusBackend::closePulse(DataEntryContext *ctx,
			  int mode)
  {
    if(tree)
    {
        freeNodes();
        delete tree;
        tree = 0;
    }
   }

  void MDSplusBackend::beginAction(OperationContext *ctx)  {}


  void MDSplusBackend::writeData(OperationContext *ctx,
			 std::string fieldname,
			 std::string timebase, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size)
  {
    
    timebaseMap.clear();
    segmentIdxMap.clear();
    switch(ctx->getRangemode()) {
      case ualconst::global_op:
	if(timebase.empty())
	  writeData(tree, ctx->getDataobjectName(), fieldname, data, datatype, dim, size);
	else
	  writeTimedData(tree, ctx->getDataobjectName(), fieldname, timebase, data, datatype, dim, size, false, false, false);
	break;
      case ualconst::slice_op:
        if(size[0] > 1)
	    writeTimedData(tree, ctx->getDataobjectName(), fieldname, timebase, data, datatype, dim, size, false, false, true);
	else
	    writeSlice(tree, ctx->getDataobjectName(), fieldname, timebase, data, datatype, dim, size);
	break;
    }
	
  }
	    
    
   
   int MDSplusBackend::readData(OperationContext *ctx,
			std::string fieldname,
			std::string timebase,
			void** data,
			int* datatype,
			int* dim,
			int* size)  
  {
    switch(ctx->getRangemode()) {
      case ualconst::global_op:
	if(!timebase.empty())
	  return readTimedData(tree, ctx->getDataobjectName(), fieldname, data, datatype, dim, size);
	else
	  return readData(tree, ctx->getDataobjectName(), fieldname, data, datatype, dim, size);
	break;
      case ualconst::slice_op:
	if(!timebase.empty())
	  return readSlice(tree, false, ctx->getDataobjectName(), fieldname, timebase, ctx->getTime(), ctx->getInterpmode(), data, datatype, dim, size);
	else
	  return readData(tree, ctx->getDataobjectName(), fieldname, data, datatype, dim, size);
	break;
    }
    return 1;
  }
  
  void MDSplusBackend::deleteData(OperationContext *ctx,
			std::string fieldname)  
  {
     deleteData(tree, ctx->getDataobjectName(), fieldname);
  }   

    
//Gabriele 2017
  void MDSplusBackend::beginWriteArraystruct(ArraystructContext *ctx,
				     int size)
  {
      segmentIdxMap.clear();//std::cout << "BEGIN WRITE ARRAY STRUCT " << ctx->getParent() << "   " << ctx->getPath() << "  " << ctx->getIndex() << std::endl;

//NOTE: size is not required as Apd uses std::vector to keep descriptors    
      std::string emptyStr("");
      MDSplus::Apd *newApd = new MDSplus::Apd();
      MDSplus::Apd *currApd;
      std::string path = ctx->getPath();
      int currPos = path.find_last_of("/");
      std::string rootName;
      if(currPos == (int)std::string::npos)
	rootName = path;
      else
	rootName = path.substr(currPos+1, path.size() - currPos);
      //Jan 2015: Add as many empty fields as the passed size -  Oct 2019
      for(int i = 0; i < size; i++) {
	newApd->setDescAt(i, currApd = new MDSplus::Apd());
	currApd->appendDesc(new MDSplus::String(rootName.c_str()));
      }
      if(ctx->getParent())
      {
 	  MDSplus::Apd *parentApd = getApdFromContext(ctx->getParent());
//	  if(!ctx->getTimed() || ctx->getParent()->getTimed()) //If either the STATIC struct array is going to be contained in another structure of array or the SYNCHRONOUS struct array is contained in a dynamic/synchronous struct array
	  if(ctx->getTimebasePath().empty() || !ctx->getParent()->getTimebasePath().empty()) //If either the STATIC struct array is going to be contained in another structure of array or within a timedAoS
	  {
	      insertNewInApd(ctx, ctx->getPath(), parentApd, ctx->getParent()->getIndex(), ctx->getPath(), emptyStr, false, newApd);	 //Gabriele March 2018 
      	  }
	  else //Beginning a dynamic AoS
	  {
	    std::string nodePath =  getTimedNode(ctx->getParent(), ctx->getPath(), ctx->getParent()->getIndex(), true);  //Gabriele March 2018
//	      std::string nodePath =  getTimedNode(ctx->getParent(), ctx->getPath(), ctx->getIndex(), true);
	      std::string currPath = composePaths(ctx->getOperationContext()->getDataobjectName(), nodePath+"/aos");
	      MDSplus::TreeNode *node = getNode(checkFullPath(currPath, true).c_str(), true);

 	      insertNewInApd(ctx, ctx->getPath(), parentApd, ctx->getParent()->getIndex(), ctx->getPath(),  emptyStr, false, node); //Gabriele March 2018
 	     // insertNewInApd(ctx, ctx->getPath(), parentApd, ctx->getIndex(), ctx->getPath(),  emptyStr, false, node); 
	   }
      }
      else //Reset starting index for timed SubAoS
      {
	  resetNodePath();
      }
      addContextAndApd(ctx, newApd);     
  }

  void MDSplusBackend::beginReadArraystruct(ArraystructContext *ctx,
				    int* size)
  {
    MDSplus::Apd *currApd;
    if(ctx->getParent()) //We are going to start reading a nested array of structures
    {
	MDSplus::Apd *parentApd = getApdFromContext(ctx->getParent());
	
	if (parentApd == NULL ||  (int)parentApd->len()==0)
	{
          if (parentApd != NULL)
	     delete parentApd;
	  *size = 0;
	  return;
	}

	if(ctx->getParent()->getIndex() >= (int)parentApd->len()) //If wrong index  Gabriele March 2018
	  throw UALBackendException("Invalid index in array of structures",LOG);
 	if(ctx->getTimebasePath().empty() || (!ctx->getTimebasePath().empty() && !ctx->getParent()->getTimebasePath().empty())) 

	{
	    MDSplus::Apd *currApd = (MDSplus::Apd *)getFromApd(parentApd, ctx->getParent()->getIndex(), ctx->getPath()); //Gabriele March 2018
	    if(!currApd) 
	    {
		*size = 0;
		return;
	    }
	    //MDSplus::Apd *currApd = (MDSplus::Apd *)getFromApd(parentApd, ctx->getIndex(), ctx->getPath());
	    if(currApd->clazz != CLASS_APD)
	        throw  UALBackendException("Internal error: array of structure is not an APD data",LOG);
	    addContextAndApd(ctx, currApd);  
	    *size = currApd->len();
	    //MDSplus::deleteData(currApd);
	}
	else //Dynamic AoS
	{
//skip the first part of the name for nested Dynamic AoS
	    std::string aosPath = ctx->getPath();
	    size_t currPos = aosPath.find_first_of("/");
	    aosPath = aosPath.substr(currPos + 1);
	    MDSplus::TreeNode *node = (MDSplus::TreeNode *)getFromApd(parentApd, ctx->getParent()->getIndex(), aosPath); //Gabriele March 2018
	    if(!node)
	    {
		*size = 0;
		return;
	    }
	    if(ctx->getOperationContext()->getRangemode() == GLOBAL_OP)
	    {
		std::string timebasePath = relativeToAbsolutePath(ctx, ctx->getTimebasePath());
		if(timebasePath.find_first_of('[') != std::string::npos) //If it is the reference of an internal AoS field
		  timebasePath = getTimedNode(ctx, timebasePath, true)+"/aos";

		if(!timebasePath.empty()) //If a timebase is defined, its path must be completed
		    timebasePath = ctx->getOperationContext()->getDataobjectName()+"/"+timebasePath;


//		if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If the timebase path refers to a field INTERNAL to the AoS (it mist be time!!)
		currApd = readDynamicLazyApd(node);
		if(currApd->len() == 0) //Empty AoS
		    *size = 0;
		else
	    	    *size = currApd->len() - 1; //The last descriptorcontains the node
	        //currApd = readDynamicApd(node);
	    }
	    else
	    {
		std::string timebasePath = relativeToAbsolutePath(ctx, ctx->getTimebasePath());

		if(timebasePath.find_first_of('[') != std::string::npos) //If it is the reference of an internal AoS field
		  timebasePath = getTimedNode(ctx, timebasePath, true)+"/aos";

		if(!timebasePath.empty()) //If a timebase is defined, its path must be completed
		    timebasePath = ctx->getOperationContext()->getDataobjectName()+"/"+timebasePath;


//		if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If the timebase path refers to a field INTERNAL to the AoS (it mist be time!!)
		if(ctx->getTimebasePath().find("../") == std::string::npos && ctx->getTimebasePath().at(0) != '/') //If the timebase path refers to a field INTERNAL to the AoS (it mist be time!!)
		  currApd = readSliceApd(node, "", ctx->getOperationContext()->getTime(), ctx->getOperationContext()->getInterpmode(), ctx->getOperationContext()->getDataobjectName()+"."+ctx->getPath());
		else
		  currApd = readSliceApd(node, timebasePath, ctx->getOperationContext()->getTime(), ctx->getOperationContext()->getInterpmode(),ctx->getOperationContext()->getDataobjectName()+"."+ctx->getPath());
		if(!currApd)
		{
		  //delete node;
		  *size = 0;
		  return;
		}
	    	*size = currApd->len();
	    }
	    //delete node;
//std::cout << "BEGIN READARRAYSTRUCT\n";
//dumpArrayStruct(currApd, 0);
	    addContextAndApd(ctx, currApd);   
	    //*size = currApd->len();
	}
    }
    else //The array of structures must be read from the pulse file
    {
	resetNodePath();  //Reset TIMED_*/GROUP_n/ITEM_n mapping
	// if(ctx->getTimed())
	if(!ctx->getTimebasePath().empty()) 
	{
	  std::string currPath = composePaths(ctx->getOperationContext()->getDataobjectName(), ctx->getPath()+"/timed_aos/group_1/item_1/aos");
	    MDSplus::TreeNode *node;
	    //NOTE: Instead of this try/catch block, one could test the ACCESS_LAYER version in the \TOP.REF_INFO.ACC_LAYER node.
	    try {
	        node = getNode(checkFullPath(currPath, true).c_str());
	    }
	    catch(MDSplus::MdsException &exc) {
	      if (!strcmp(exc.what(),"%TREE-W-NNF, Node Not Found")) {
		  // Backward compatibility for previous MDSplus backend versions
		currPath = composePaths(ctx->getOperationContext()->getDataobjectName(), ctx->getPath()+"/timed_1/aos");
		  node = getNode(checkFullPath(currPath, true).c_str());
	      }
	      else {throw;}
	    }
	    if(ctx->getOperationContext()->getRangemode() == GLOBAL_OP)
	    {
		std::string timebasePath = relativeToAbsolutePath(ctx, ctx->getTimebasePath());

		if(timebasePath.find_first_of('[') != std::string::npos) //If it is the reference of an internal AoS field
		  timebasePath = getTimedNode(ctx, timebasePath, true)+"/aos";

		if(!timebasePath.empty()) //If a timebase is defined, its path must be completed
		  timebasePath = ctx->getOperationContext()->getDataobjectName()+"/"+timebasePath;


//		if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If the timebase path refers to a field INTERNAL to the AoS (it mist be time!!)
		currApd = readDynamicLazyApd(node);
		//currApd = readDynamicApd(node);
		if(currApd->len() == 0) //Empty AoS
		    *size = 0;
		else
	    	    *size = currApd->len() - 1; //The last descriptorcontains the node
	    }
	    else
	    {
		if(!(ctx->getTimebasePath().substr(0,3) == "../") && ctx->getTimebasePath()[0] != '/') //If it refers to a field which is internal to the AoS (must be time)
		  currApd = readSliceApd(node, "", ctx->getOperationContext()->getTime(), ctx->getOperationContext()->getInterpmode(), ctx->getOperationContext()->getDataobjectName()+"."+ctx->getPath());
		else
		{
		    std::string timebase = relativeToAbsolutePath(ctx, ctx->getTimebasePath());
		    timebase = ctx->getOperationContext()->getDataobjectName()+"/"+timebase;
		    currApd = readSliceApd(node, timebase, ctx->getOperationContext()->getTime(), ctx->getOperationContext()->getInterpmode(),ctx->getOperationContext()->getDataobjectName()+"."+ctx->getPath());
		}
		if(!currApd)
		{
		    *size = 0;
		    return;
		}
	   	*size = currApd->len();
	    }
	    //delete node;
	}
	else
	{
	  currApd = readApd(tree, ctx->getOperationContext()->getDataobjectName(), ctx->getPath()+"/static");
	    if(!currApd)
	    {
	        *size = 0;
		return;
	    }	
	    *size = currApd->len();

/*****Lazy AoS	
	    MDSplus::Apd *resApd;
	    if(ctx->getRangemode() == GLOBAL_OP)
	    {
	        resApd = resolveApdTimedFields(currApd);
	    } 
	    else
	    {
	        resApd = resolveApdSliceFields(currApd, ctx->getTime(), ctx->getInterpmode(), ctx->getTimebasePath(), ctx->getDataobjectName());
	    }
	    MDSplus::deleteData(currApd);
	    currApd = resApd;
**********/	
	}

//std::cout << "RESOLVED" << std::endl;
//dumpArrayStruct(currApd, 0);
//	addContextAndApd(ctx, currApd);   
	if(!currApd)
	    *size = 0;
	else
	{
	    addContextAndApd(ctx, currApd);   
	   // *size = currApd->len();
	}
    }
}


  MDSplus::Apd *MDSplusBackend::resolveApdTimedFields(MDSplus::Apd *apd)
  {
      //MDSplus::setActiveTree(tree);
      size_t numDescs = apd->len();
      MDSplus::Apd *retApd = new MDSplus::Apd();
      for(size_t i = 0; i < numDescs; i++)
      {
	  MDSplus::Data *currDescr = apd->getDescAt(i);
	  if(!currDescr)
	  {
	      retApd->appendDesc(NULL);
	      continue;
	  }
	  if(currDescr->clazz == CLASS_APD)
	  {
	      MDSplus::Data *currData = resolveApdTimedFields((MDSplus::Apd *)currDescr);
	      retApd->appendDesc(currData);
	  }
	  else if(currDescr->clazz == CLASS_S && currDescr->dtype == DTYPE_NID) //If it is a nid reference (either nested dynamic AoS or time dependent data)
	  {
	      //MDSplus::Data *currData = currDescr->data();
	      const char *dtype = ((MDSplus::TreeNode *)currDescr)->getDType();
/*	      if(!strcmp(dtype, "DTYPE_MISSING")) //Gabriele June 2019: Handle Empty dynamic AoS
	      {
		  retApd->appendDesc(NULL);
		  retApd->appendDesc(emptyApd);
		  continue;
	      }
*/	      if(strcmp(dtype, "DTYPE_BU") && strcmp(dtype, "DTYPE_MISSING"))  //if it is not a serialized APD (nested dynamic AoS)
	      {
		  try {

//		      MDSplus::Data *currData = currDescr->data();
		      void *dataPtr;
		      int datatype;
		      int numDims;
		      int dims[64];
    		      readTimedData((MDSplus::TreeNode *)currDescr, &dataPtr, &datatype, &numDims, (int *)dims);
    		      MDSplus::Data *currData = assembleData(dataPtr, datatype, numDims, dims);
		      free(dataPtr);

		      retApd->appendDesc(currData);
		  }catch(MDSplus::MdsException &exc){std::cout << exc.what() << std::endl;}

	      }
	      else
	      {
//		MDSplus::deleteData(currData); //It is a nested dynamic AoS, not handled here
		  currDescr->incRefCount();
		  retApd->appendDesc(currDescr);
	      }
	  }
 	  else
	  {
	      currDescr->incRefCount();
	      retApd->appendDesc(currDescr);
	  }
      }
      return retApd;
  }
	
		  
  MDSplus::Apd *MDSplusBackend::resolveApdSliceFields(MDSplus::Apd *apd, double time, int interpolation, std::string timebasePath, std::string dataobjectPath)
  {
      size_t numDescs = apd->len();
      MDSplus::Apd *retApd = new MDSplus::Apd();
      for(size_t i = 0; i < numDescs; i++)
      {
	  MDSplus::Data *currDescr = apd->getDescAt(i);
	  if(!currDescr)
	    continue;
	  if(currDescr->clazz == CLASS_APD)
	  {
	      MDSplus::Data *currData = resolveApdSliceFields((MDSplus::Apd *)currDescr, time, interpolation, timebasePath, dataobjectPath);
	      currData->incRefCount();
	      retApd->appendDesc(currData);
	  }
	  else if(currDescr->clazz == CLASS_S && currDescr->dtype == DTYPE_NID) //If it is a nid reference (either nested dynamic AoS or time dependent data
	  {
	      void *data;
	      int datatype, numDims;
	      int dims[16];
	      const char *dtype = ((MDSplus::TreeNode *)currDescr)->getDType();
//Gabriele June 2019
	      if(!strcmp(dtype, "DTYPE_BU") || !strcmp(dtype, "DTYPE_MISSING"))  //It is a serialized APD
	      {
//		  MDSplus::Apd *resolvedApd = readSliceApd((MDSplus::TreeNode *)currDescr, timebasePath, time, interpolation);
//dumpArrayStruct(resolvedApd, 0);
//		  retApd->appendDesc(resolvedApd);
		  currDescr->incRefCount();
		  retApd->appendDesc(currDescr);
	      }
	      else //if it is not a serialized APD (nested dynamic AoS)
	      {
		  char *path = ((MDSplus::TreeNode *)currDescr)->getFullPath();
		  std::string pathStr(path);
		  delete [] path;
		  std::string emptyStr("");

		  if(!readSlice(tree, true, pathStr, dataobjectPath, timebasePath, time, interpolation, &data, &datatype, &numDims, dims, false))
		      throw UALBackendException("Internal error: expected valid slice in resolveApdSliceFields",LOG);

		  MDSplus::Data *currData = assembleData(data, datatype, numDims, dims);
		  free((char *)data);
		  currData->incRefCount();
		  retApd->appendDesc(currData);
	      }
	  }
	  else
	  {
	      currDescr->incRefCount();
	      retApd->appendDesc(currDescr);
	  }
      }
      return retApd;
  }
	
  void MDSplusBackend::resolveApdField(MDSplus::Apd *apd, ArraystructContext *ctx)
  {
      if(apd->len() != 2)
      {
	  std::cout << "INTERNAL ERROR in resolveApdField: wrong number (" << apd->len() << " !=2) of apd fields"<< std::endl;
          throw UALBackendException("Internal error:  wrong number (%d !=2) of apd fields in resolveApdField",LOG);
      }
      MDSplus::Data *currDescr = apd->getDescAt(1);
      const char *dtype = ((MDSplus::TreeNode *)currDescr)->getDType();
      if(!strcmp(dtype, "DTYPE_MISSING"))
      {
          std::cout << "WARNING: EMPTY APD FIELD in resolveApdField" << std::endl;
	  return;
      }
      if(!strcmp(dtype, "DTYPE_BU") && strcmp(dtype, "DTYPE_MISSING"))  //if it is not a serialized APD (nested dynamic AoS)
      {
	  std::cout << "INTERNAL ERROR in resolveApdField: unexpected serialized apd found" << std::endl;
          throw UALBackendException("Internal error:  nexpected serialized apd found in resolveApdField",LOG);
      }
      if(ctx->getOperationContext()->getRangemode() == GLOBAL_OP)
      {
	  try {
	      void *dataPtr;
	      int datatype;
	      int numDims;
	      int dims[64];
    	      readTimedData((MDSplus::TreeNode *)currDescr, &dataPtr, &datatype, &numDims, (int *)dims);
    	      MDSplus::Data *currData = assembleData(dataPtr, datatype, numDims, dims);
	      free(dataPtr);
	      MDSplus::Data **dscs = apd->getDscArray();
	      MDSplus::deleteData(dscs[1]);
	      dscs[1] = currData;	
 	  }catch(MDSplus::MdsException &exc){std::cout << exc.what() << std::endl;}
      }
      else //get slice
      {
	  void *data;
	  int datatype, numDims;
	  int dims[16];
          char *path = ((MDSplus::TreeNode *)currDescr)->getFullPath();
          std::string pathStr(path);
	  delete [] path;
	  std::string emptyStr("");
	  if(!readSlice(tree, true, pathStr, ctx->getOperationContext()->getDataobjectName(), ctx->getTimebasePath(), ctx->getOperationContext()->getTime(), ctx->getOperationContext()->getInterpmode(), &data, &datatype, &numDims, dims, false))
		      throw UALBackendException("Internal error: expected valid slice in resolveApdField",LOG);

	  MDSplus::Data *currData = assembleData(data, datatype, numDims, dims);
	  free((char *)data);
	  MDSplus::Data **dscs = apd->getDscArray();
	  MDSplus::deleteData(dscs[1]);
	  dscs[1] = currData;	
      }
 }





		  
//Gabriele 2017
  void MDSplusBackend::putInArraystruct(ArraystructContext *ctx,
				std::string fieldname,     //Relative to the curent AoS root
				std::string timebasename,  //Relative to the current AoS root. Empty for non time dependent fields
				int idx,
				void* data,
				int datatype,
				int dim,
				int* size)
  {
//std::cout << "PUT IN ARRAYSTRUCT" << fieldname  << "Time: " << timebasename << std::endl;
	MDSplus::Apd *currApd = getApdFromContext(ctx);
	std::string path = ctx->getPath();
//	std::string path = fieldname;
	int currPos = path.find_last_of("/");
	std::string rootName;
	if(currPos == (int)std::string::npos)
	    rootName = path;
	else
	    rootName = path.substr(currPos+1, path.size() - currPos);

	insertNewInApd(ctx, rootName, currApd, idx, fieldname, timebasename, ctx->getOperationContext()->getRangemode() != GLOBAL_OP, NULL, data, datatype, dim, size);
  }


    int MDSplusBackend::getFromArraystruct(ArraystructContext *ctx,
				  std::string fieldname,
				  int idx,
				  void** retData,
				  int* datatype,
				  int* dim,
				  int* size)
				  
    {
	MDSplus::Apd *currApd = getApdFromContext(ctx);
        MDSplus::Data *data = getFromApd(currApd, idx, fieldname, ctx);
	if(!data) return 0;
	MDSplus::Data *evaluatedData = data->data();
	MDSplusBackend::disassembleData(data, retData, datatype, dim, size);
	MDSplus::deleteData(evaluatedData);
	return 1;
    }



  void MDSplusBackend::endAction(Context *inCtx)
  {
//      if(((OperationContext *)inCtx)->getAccessmode() == SLICE_OP && ((OperationContext *)inCtx)->getType() == CTX_OPERATION_TYPE)
//	timebaseMap.clear(); //Free timebase cache for this IDS slice operation

      if(inCtx->getType() == CTX_ARRAYSTRUCT_TYPE) //Only in this case actions are required 
      {
	  ArraystructContext *ctx = (ArraystructContext *)inCtx;
 	  MDSplus::Apd *currApd = getApdFromContext(ctx);

//	  dumpArrayStruct(currApd, 0);

	  if (currApd == NULL)
	    return;
	    
	  removeContextAndApd(ctx, currApd);
	  if(ctx->getParent() != NULL)  //If the AoS is nested
	  {
	  //    if(ctx->getTimed() && !ctx->getParent()->getTimed())  //If it is a dynamic (NOT synchronous) nested AoS
	      if(!ctx->getTimebasePath().empty() && ctx->getParent()->getTimebasePath().empty())
	      {

		if(ctx->getOperationContext()->getAccessmode() == ualconst::write_op)
	          {
		    std::string nodePath = getTimedNode(ctx->getParent(), ctx->getPath(), ctx->getParent()->getIndex(), true); //Gabriele March 2018
   		      //std::string nodePath = getTimedNode(ctx->getParent(), ctx->getPath(), ctx->getIndex(), true);
 
		      std::string timebasePath = relativeToAbsolutePath(ctx, ctx->getTimebasePath());
		      if(timebasePath.find_first_of('[') != std::string::npos) //If it is the reference of an internal AoS field
//Gabriele OCT 2017: If it is the reference to a field ingternal to the AoS, it is assumed to be field "time"
			  timebasePath = "";
//		          timebasePath = getTimedNode(ctx, timebasePath, true)+"/aos";
		      if(ctx->getOperationContext()->getRangemode() == GLOBAL_OP)
		      {
			  //Handle the case in which timebase is defined within AoS itself
//			  if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If timebase is internal to the AoS
			  if(ctx->getTimebasePath().find("../") == std::string::npos && ctx->getTimebasePath()[0] != '/') //If timebase is internal to the AoS
			    writeDynamicApd(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, "", false);
			  else
			    writeDynamicApd(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, composePaths(ctx->getOperationContext()->getDataobjectName(), timebasePath), false);
		      }
		      else //SLICE_OP
		      {
//std::cout << "WRITE SLICE\n";
//dumpArrayStruct(currApd, 0);
			  //Handle the case in which timebase is defined within AoS itself
			  //if(timebasePath.empty())
//			      if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If timebase is internal to the AoS
			    if(ctx->getTimebasePath().find("../") == std::string::npos && ctx->getTimebasePath()[0] != '/') //If timebase is internal to the AoS
			    {
				if(currApd->len() > 1)
				  writeDynamicApd(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, "", true);
				else				
				  writeApdSlice(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, "", ctx->getOperationContext()->getTime());
			    }
			    else
			    {
				if(currApd->len() > 1)
				  writeDynamicApd(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, composePaths(ctx->getOperationContext()->getDataobjectName(), timebasePath), true);
				else
				  writeApdSlice(currApd, ctx->getOperationContext()->getDataobjectName()+"/"+nodePath, composePaths(ctx->getOperationContext()->getDataobjectName(), timebasePath), ctx->getOperationContext()->getTime());
			    }
		      }
		  }
	  	  MDSplus::deleteData(currApd);

	      }
	      else //Nested static AoS, need to check for trailing empty AoS fields
	      {
		  for (int idx = currApd->len(); idx < ctx->getIndex(); idx++)
			currApd->appendDesc(NULL);
              }
	  }  

	  if(ctx->getParent() == NULL)
	  {
	    if(ctx->getOperationContext()->getAccessmode() == ualconst::write_op)
	      {
		if(ctx->getOperationContext()->getRangemode() == GLOBAL_OP)
		  {
		    //  if(ctx->getTimed())   //not static AoS. Only a single dynamic AoS. In this case only TIMED_AOS/GROUP_1/ITEM_1 is used
 		      if(!ctx->getTimebasePath().empty()) 
		      {
			std::string aosPath = ctx->getOperationContext()->getDataobjectName() + "/" +ctx->getPath();
//			  if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If timebase is internal to the AoS
			  if(ctx->getTimebasePath().find("../") == std::string::npos && ctx->getTimebasePath()[0] != '/') //If timebase is internal to the AoS
			      writeDynamicApd(currApd, aosPath+"/timed_aos/group_1/item_1", "");
			  else
			  {
			      writeDynamicApd(currApd, aosPath+"/timed_aos/group_1/item_1", relativeToAbsolutePath(ctx, ctx->getTimebasePath()));
			  }
		      }
		      else
		      {
//Check for empty trailing fields

		  	  for (int idx = currApd->len(); idx < ctx->getIndex(); idx++)
			      currApd->appendDesc(NULL);
			  writeStaticApd(currApd, ctx->getOperationContext()->getDataobjectName(), ctx->getPath());
		      }	
		  }
		  else //SLICE_OP
		  {
		      //if(ctx->getTimed())   //not static AoS. Only a single dynamic AoS
		      if(!ctx->getTimebasePath().empty())   //not static AoS. Only a single dynamic AoS
		      {
			std::string aosPath = ctx->getOperationContext()->getDataobjectName() + "/" +ctx->getPath();
//			  if(ctx->getTimebasePath().find_first_of("../") == std::string::npos) //If timebase is internal to the AoS
			  if(ctx->getTimebasePath().find("../") == std::string::npos && ctx->getTimebasePath()[0] != '/') //If timebase is internal to the AoS
			  {
			      if(currApd->len() > 1)
			          writeDynamicApd(currApd, aosPath+"/timed_aos/group_1/item_1", "", true);
			      else
				writeApdSlice(currApd, aosPath+"/timed_aos/group_1/item_1", "", ctx->getOperationContext()->getTime());
			  }
			  else
			  {
			      if(currApd->len() > 1)
				  writeDynamicApd(currApd, aosPath+"/timed_aos/group_1/item_1", relativeToAbsolutePath(ctx, ctx->getTimebasePath()), true);
			      else
				writeApdSlice(currApd, aosPath+"/timed_aos/group_1/item_1", relativeToAbsolutePath(ctx, ctx->getTimebasePath()), ctx->getOperationContext()->getTime());
			  }
		      }
		      else //static AoS: needs to be written only once
		      {
	                  std::string staticName("static");
			  std::string currPath = composePaths(getTopAoSPath((ArraystructContext *)inCtx), staticName);
			  MDSplus::TreeNode *node = getNode(checkFullPath(currPath, true).c_str());
			  if(node->getLength() == 0) //If it is the first time it is written 
		      	  {
			    writeStaticApd(currApd, ctx->getOperationContext()->getDataobjectName(), ctx->getPath());
		      	  }
//			  delete node;	
		      }
		  }
//Gabriele Oct 2021: 	end of a AoS write: clear segmentIdxMap used for caching segment idx and slice idx mapping info
		  segmentIdxMap.clear();
	      }
	      MDSplus::deleteData(currApd);
	  }
      }
  }
  
  void MDSplusBackend::printFileVersionInfo(int shot, int run, std::string usr, std::string tok, std::string ver)
  {
    MDSplusBackend *be = new MDSplusBackend();
    char *uri;
    DataEntryContext::build_uri_from_legacy_parameters(MDSPLUS_BACKEND, 
                         shot,
                         run,
                         usr.c_str(),
                         tok.c_str(),
                         ver.c_str(),
                         "",
                         &uri);
    DataEntryContext *pctx = new DataEntryContext(uri);
    be->setDataEnv(pctx); 
    int shotNum = be->getMdsShot(shot, run, true, DEF_TREENAME);
    try {
      be->tree = new MDSplus::Tree(DEF_TREENAME, shotNum); 
    }
    catch(MDSplus::MdsException &exc)
      {
	throw UALBackendException(exc.what(),LOG); 
      }
    
    MDSplus::TreeNode *n1, *n2;
    char *d1, *d2;
    
    try
      {
	n1 = be->tree->getNode("VERSION:ACC_LAYER");
	d1 = n1->data()->getString();
	n2 = be->tree->getNode("VERSION:DATA_DICT");
	d2 = n2->data()->getString();
      }
    catch(MDSplus::MdsException &exc)
      {
	std::cout << "Cannot get Access Layer and Data Dictionary versions. Pulse file may be too old." << std::endl;
	exit(0);
      }
    std::cout << "This MDS+ file has been created with the following versions of IMAS:" << std::endl;
    std::cout << "Access Layer: " << d1 << std::endl;
    std::cout << "Data Dictionary: " << d2 << std::endl;

    be->resetIdsPath(DEF_TREENAME);

    delete(n1);
    delete(n2);
    delete(be->tree);
    delete(be);
  }

//Return version info
    std::pair<int,int> MDSplusBackend::getVersion(DataEntryContext *ctx)
    {
	char szTree[256] = { 0 };
	 strcpy(szTree, DEF_TREENAME);

	std::pair<int,int> version;
	if(!ctx)
	    version = {MDSPLUS_BACKEND_MAJOR, MDSPLUS_BACKEND_MINOR};
	else
	{
	    MDSplus::Tree *t;
	    try {
	  	setDataEnv(ctx); 
    	  	int shotNum = getMdsShot(ctx->getShot(), ctx->getRun(), true, szTree);

		t = new MDSplus::Tree(szTree, shotNum);
		resetIdsPath(szTree);
	    }catch(MDSplus::MdsException &exc)
	    {
		resetIdsPath(szTree);
	      	throw UALBackendException("Cannot open MDSplus tree for getting version",LOG);
	    }
	    try {
		MDSplus::TreeNode *nMajor = t->getNode("VERSION:BACK_MAJOR");
		MDSplus::Data *data = nMajor->getData();
		int major =  data->getInt();
		MDSplus::deleteData(data);
		MDSplus::TreeNode *nMinor = t->getNode("VERSION:BACK_MINOR");
		data = nMinor->getData();
		int minor =  data->getInt();
		MDSplus::deleteData(data);
		delete nMajor;
		delete nMinor;
		delete t;
		version = {major, minor};
	    } catch(MDSplus::MdsException &exc)
	    {
		delete t;
		version = {1,0};
	    }
	}
	return version;
    }
		
		
