#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ual_low_level_hdf5.h"
#define MAX_FILES 10000
#define MAX_STRINGS 20000
#define SLICE_BLOCK 1

#define STRING 1
#define STRING_VECTOR 5
#define INT 2
#define FLOAT 3
#define DOUBLE 4
#define DIMENSION -1    // if we need to read the dimensions only

const int EMPTY_INT = -999999999;
const float EMPTY_FLOAT = -9.0E35;
const double EMPTY_DOUBLE = -9.0E40;

static hid_t hdf5Files[MAX_FILES];
extern char *errmsg;

typedef struct obj_t {
    hid_t handle;   // HDF5 handle of the group corresponding to the object
    int dim;        // number of elements contained in the object (not used for put)
    int timeIdx;    // for timed objects, index of the time to be read (not used for put)
    struct obj_t *nextObj; // a list of other objects contained inside this object (for cleaning purpose)
} obj_t;

static int findFirstHdf5Idx()
{
    int i;

    for(i = 0; i < MAX_FILES && hdf5Files[i]; i++);
    if(i == MAX_FILES)
    {
        sprintf(errmsg, "No more HDF5 files available");
        return -1;
    }
    return i;
}

extern char *TranslateLogical(char *);

static char *getHdf5FileName(char *filename, int shot, int run)
{
    static char outName[2048];
    char *base = TranslateLogical("HDF5_BASE");
    if(base && *base)
        sprintf(outName, "%s/%s_%d_%d.hd5", base, filename, shot, run);
    else
        sprintf(outName, "%s_%d_%d.hd5", filename, shot, run);
    free(base);
    return outName;
}
static char *getHdf5ModelName(char *filename)
{
    static char outName[2048];
    char *base = TranslateLogical("HDF5_MODEL_BASE");
    if(base && *base)
        sprintf(outName, "%s/%s_model.hd5", base, filename);
    else
        sprintf(outName, "%s_model.hd5", filename);
    free(base);
    return outName;
}

static void releaseHdf5File(int idx)
{
    hdf5Files[idx] = 0;
}

int hdf5EuitmCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx)
{
    int idx = findFirstHdf5Idx();

    static char cpCommand[2048];

    sprintf(cpCommand, "cp %s %s", getHdf5ModelName(name), getHdf5FileName(name, shot, run));
    system(cpCommand);

    hdf5Files[idx] = H5Fopen(getHdf5FileName(name, shot, run), H5F_ACC_RDWR, H5P_DEFAULT);
    if(hdf5Files[idx] < 0)
    {
        sprintf(errmsg, "Error creating HDF5 file %s", getHdf5FileName(name, shot, run));
        return -1;
    }
    *retIdx = idx;
    return 0;
}


int hdf5EuitmOpen(char *name, int shot, int run, int *retIdx)
{
    int idx = findFirstHdf5Idx();
    hdf5Files[idx] = H5Fopen(getHdf5FileName(name, shot, run), H5F_ACC_RDWR, H5P_DEFAULT);
    if(hdf5Files[idx] < 0)
    {
        sprintf(errmsg, "Error opening HDF5 file %s", getHdf5FileName(name, shot, run));
        return -1;
    }
    *retIdx = idx;
    return 0;
}

int hdf5EuitmClose(int idx, char *name, int shot, int run)
{
    if(H5Fclose(hdf5Files[idx]) < 0)
    {
        sprintf(errmsg, "Error closing HDF5 file %s", name);
        return -1;
    }
    hdf5Files[idx] = 0;
    return 0;

}


static int putData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int isTimed, void *data)
{
    int i;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, dataspace;
    int exists;
    hsize_t hdims[32], maxDims[32];
    hid_t cParms;
    hsize_t chunkDims[32];
    int totSize = 0;


    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists < 0)
    {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(exists)
    {
        if(H5Ldelete(group, dataName, H5P_DEFAULT) < 0)
        {
            sprintf(errmsg, "Error deleting dataset  %s", dataName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }

    for(i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
    if(totSize == 0)
    {
        free(groupName);
        free(dataName);
        return 0;
    }


    //Ready to create a new dataset: first define the datatype
    switch(type)
    {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (datatype,132);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size (inDatatype,132);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

    //then the dataspace
    for(i = 0; i < nDims; i++)
        hdims[i] = dims[i];
    if(!isTimed)
    {
        dataspace = H5Screate_simple(nDims, hdims, NULL);
    }
    else
    {
        for(i = 0; i < nDims - 1; i++)
        {
            maxDims[i] = dims[i];
        }
        //if(type == STRING)
        //    maxDims[nDims - 1] = dims[nDims - 1];
        //else
            maxDims[nDims - 1] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(nDims, hdims, maxDims);
    }

    if(isTimed)
    {
        cParms = H5Pcreate (H5P_DATASET_CREATE);
        for(i = 0; i < nDims; i++)
            chunkDims[i] = dims[i];
        H5Pset_chunk( cParms, nDims, chunkDims);
        dataset = H5Dcreate(group, dataName, datatype, dataspace,H5P_DEFAULT,  cParms, H5P_DEFAULT);
    }
    else
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if(dataset < 0)
    {
        sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if(H5Dwrite(dataset, inDatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data)<0)
    {
        sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    return 0;
}
static int putDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data)
{
    int i, isFirst, currNDims, totSize;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;
    int exists;
    hsize_t hdims[32], maxDims[32], currHDims[32], currMaxDims[32], startOut[32],
        strideOut[32], blockOut[32], countOut[32];
    hid_t cParms;
    hsize_t chunkDims[32], extSize[32];


    for(i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
    if(totSize == 0)
    {
        free(groupName);
        free(dataName);
        return 0;
    }



    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists < 0)
    {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch(type)
    {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (datatype,H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size (inDatatype,H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

    if(!exists) //The first time we need to create the dataset
    {
        for(i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        for(i = 0; i < nDims; i++)
        {
            maxDims[i] = dims[i];
        }
        hdims[nDims] = 1;
        maxDims[nDims] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(nDims+1, hdims, maxDims);

        cParms = H5Pcreate (H5P_DATASET_CREATE);
        for(i = 0; i < nDims; i++)
            chunkDims[i] = dims[i];
        chunkDims[nDims] = SLICE_BLOCK;
        H5Pset_chunk( cParms, nDims+1, chunkDims);
        dataset = H5Dcreate(group, dataName, datatype, dataspace,H5P_DEFAULT,  cParms, H5P_DEFAULT);
        if(dataset < 0)
        {
            sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        H5Pclose(cParms);
    }
    else //get the dataset
    {
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if(dataset < 0)
        {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        dataspace = H5Dget_space(dataset);
        if(dataspace < 0)
        {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
     //Get last dimension
    }
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if(currNDims != nDims + 1)
    {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for(i = 0; i < nDims; i++)
    {
        if(dims[i] != currHDims[i])
        {
            sprintf(errmsg, "Internal error: wrong dimension in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    memDataspace = H5Screate(H5S_SIMPLE);
    if(nDims == 0)
    {
        hdims[0] = 1;
        H5Sset_extent_simple(memDataspace,1,hdims,hdims);
    }
    else
    {
        for(i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        H5Sset_extent_simple(memDataspace,nDims,hdims,hdims);
    }
   //Select out hyperslab

    for(i = 0; i < nDims; i++)
    {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
    }
    if(exists)
    {
        startOut[nDims] = currHDims[nDims];
        blockOut[nDims] = 1;
        strideOut[nDims] = 1;
        countOut[nDims] = 1;
        for(i = 0; i < nDims; i++)
        {
            extSize[i] = dims[i];
        }
        extSize[nDims] = currHDims[nDims]+1;
        if(H5Dset_extent(dataset, extSize) < 0)
        {
            sprintf(errmsg, "Internal error: extend failed in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        H5Sclose(dataspace);
        dataspace = H5Dget_space(dataset);
    }
    else //The first time the dataset is created
    {
        startOut[nDims] = 0;
        blockOut[nDims] = 1;
        strideOut[nDims] = 1;
        countOut[nDims] = 1;
    }



    if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0)
    {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if(H5Dwrite(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, data)<0)
    {
        sprintf(errmsg, "Error writing dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}
static int replaceLastDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data)
{
    int i, isFirst, currNDims;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;
    int exists;
    hsize_t hdims[32], maxDims[32], currHDims[32], currMaxDims[32], startOut[32],
        strideOut[32], blockOut[32], countOut[32];
    hid_t cParms;
    hsize_t chunkDims[32], extSize[32];

    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists <= 0)
    {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch(type)
    {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (datatype,H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size (inDatatype,H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

//get the dataset
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if(dataset < 0)
    {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    dataspace = H5Dget_space(dataset);
    if(dataspace < 0)
    {
        sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
     //Get last dimension
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if(currNDims != nDims + 1)
    {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for(i = 0; i < nDims; i++)
    {
        if(dims[i] != currHDims[i])
        {
            sprintf(errmsg, "Internal error: wrong dimension in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    memDataspace = H5Screate(H5S_SIMPLE);
    if(nDims == 0)
    {
        hdims[0] = 1;
        H5Sset_extent_simple(memDataspace,1,hdims,hdims);
    }
    else
    {
        for(i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        H5Sset_extent_simple(memDataspace,nDims,hdims,hdims);
    }
   //Select out hyperslab

    for(i = 0; i < nDims; i++)
    {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
    }
    startOut[nDims] = currHDims[nDims]-1;
    blockOut[nDims] = 1;
    strideOut[nDims] = 1;
    countOut[nDims] = 1;

    if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0)
    {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if(H5Dwrite(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, data)<0)
    {
        sprintf(errmsg, "Error writing dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}

static int getData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, char  **data)
{
    int i, savedNDims, dataSize, strLen;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char *outData;
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, savedDatatype, savedDataspace;
    int exists;
    hsize_t hdims[32], savedDims[32], savedMaxdims[32];
    hid_t cParms;
    hsize_t chunkDims[32];

    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
     }

    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists <= 0)
    {
        sprintf(errmsg, "Error: dataset %s not found", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Ready to create a new dataset: first define the datatype
    switch(type)
    {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
	    inDatatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (inDatatype,132);
	    datatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (inDatatype,132);
	  break;
        case STRING_VECTOR:
            datatype = H5Tcopy (H5T_C_S1);
            H5Tset_size (datatype,H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size (inDatatype,H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if(dataset < 0)
    {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    /*    savedDatatype = H5Dget_type(dataset);
    if(! H5Tequal(savedDatatype, datatype))
    {
        sprintf(errmsg, "Error: invalid datatype for %s, using saved datatype ", dataName);
	//free(groupName);
        //free(dataName);
	return -1;
    }
    */
    //Check dataspace
    savedDataspace = H5Dget_space(dataset);
    savedNDims = H5Sget_simple_extent_ndims(savedDataspace);
    if(nDims != savedNDims)
    {
        sprintf(errmsg, "Error: wrong number of dimensions for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }

    if(H5Sget_simple_extent_dims(savedDataspace, savedDims, savedMaxdims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for(i = 0; i < nDims; i++)
    {
        dims[i] = savedDims[i];
    }

    //Allocate space for data
    dataSize = H5Dget_storage_size(dataset);
    outData = malloc(dataSize);


    //Check passed, read data
    if(H5Dread(dataset, inDatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, outData) < 0)
    {
        sprintf(errmsg, "Error reading dataset %s ", dataName);
        free(groupName);
        free(dataName);
        free(outData);
        return -1;
    }

    if(type == STRING)
    {

      strLen = strlen(outData);
      *data = malloc(strLen+1);
      strcpy(*data, outData);

      free(outData);

    }
    else if (type == STRING_VECTOR)
    {
        *data = malloc(dims[0] * sizeof(char *));
        for(i = 0; i < dims[0]; i++)
        {
            strLen = strlen(((char **)outData)[i]);
            ((char **)*data)[i] = malloc(strLen + 1);
            strcpy(((char **)*data)[i], ((char **)outData)[i]);
        }
        H5Dvlen_reclaim(inDatatype, savedDataspace, H5P_DEFAULT, outData);
        free(outData);
    }
    else
    {
        *data = outData;
    }


    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(savedDataspace);
    return 0;
}

//Get the two consecutive data samples starting from dataIdx
static int getDataSlices(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int dataIdx, int numSlices, char **data)
{
    int i, currNDims, dataSize;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;

    hsize_t hdims[32], maxDims[32], currHDims[32], currMaxDims[32], startOut[32],
        strideOut[32], blockOut[32], countOut[32];
    hid_t cParms;
    hsize_t chunkDims[32], extSize[32];

    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(H5Lexists(group, dataName, H5P_DEFAULT) <= 0)
    {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch(type)
    {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            dataSize = sizeof(int);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            dataSize = sizeof(float);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            dataSize = sizeof(double);
            break;
        case STRING:
            sprintf(errmsg, "String slices not supported");
            return -1;
            break;
    }

    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if(dataset < 0)
    {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    /*    savedDatatype = H5Dget_type(dataset);
    if(! H5Tequal(savedDatatype, datatype))
    {
        sprintf(errmsg, "Error: invalid datatype for %s, using saved datatype ", dataName);
	printf("Datatype does not match...\n");
        datatype = savedDatatype;
	free(groupName);
        free(dataName);
	return -1;
    }
    */
    dataspace = H5Dget_space(dataset);
    if(dataspace < 0)
    {
        sprintf(errmsg, "Error getting dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Get last dimension
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if(currNDims != nDims + 1)
    {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    for(i = 0; i < nDims; i++)
    {
        dims[i] = currHDims[i];
    }
    if(dataIdx < 0 || dataIdx + numSlices > currHDims[nDims])
    {
        sprintf(errmsg, "Internal error: slice Idx outsize last dimension range in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    memDataspace = H5Scopy(dataspace);

    //Select out hyperslab

    for(i = 0; i < nDims; i++)
    {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
        dataSize *= dims[i];
        hdims[i] = dims[i];
    }
    startOut[nDims] = dataIdx;
    blockOut[nDims] = numSlices;
    strideOut[nDims] = 1;
    countOut[nDims] = 1;
    hdims[nDims] = numSlices;
    dataSize *= numSlices;

    if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0)
    {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    memDataspace = H5Screate_simple(nDims+1, hdims, NULL);


    //Write in the dataset
    *data = malloc(dataSize);
    if(H5Dread(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, *data)<0)
    {
        sprintf(errmsg, "Error reading dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        free(*data);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}

static int sliceIdx1, sliceIdx2;
static double sliceTime1, sliceTime2;

static int getSliceIdxs(int expIdx, char *cpoPath, double time)
{
    double *times;
    int status, dim, i;

    status = hdf5GetVect1DDouble(expIdx, cpoPath, "time", &times, &dim);
    if(status) return status;
    if(dim == 1 || time <= times[0])
    {
        sliceIdx1 = 0;
        sliceTime1 = times[0];
        sliceIdx2 = -1;
    }
    else if(time >= times[dim - 1])
    {
        sliceIdx1 = dim-1;
        sliceTime1 = times[dim-1];
        sliceIdx2 = -1;
    }
    else //dim > 1
    {
        for(i = 0; i < dim-1; i++)
            if(times[i] <= time && times[i+1] >= time)
                break;
        sliceIdx1 = i;
        sliceTime1 = times[i];
        sliceIdx2 = i+1;
        sliceTime2 = times[i+1];
    }
    free(times);
    return 0;
}



//Low level function prototypes
int hdf5PutString(int expIdx, char *cpoPath, char *path, char *data,int strlen)
{
    int dims = 1;
    dims = (strlen / 132) +1;
    return putData(expIdx, cpoPath, path, STRING, 1, &dims, 0, data);

}
int hdf5PutInt(int expIdx, char *cpoPath, char *path, int data)
{
    int dims = 1;
    return putData(expIdx, cpoPath, path, INT, 1, &dims, 0, &data);

}

int hdf5PutFloat(int expIdx, char *cpoPath, char *path, float data)
{
    int dims = 1;
    return putData(expIdx, cpoPath, path, FLOAT, 1, &dims, 0, &data);

}

int hdf5PutDouble(int expIdx, char *cpoPath, char *path, double data)
{
    int dims = 1;
    return putData(expIdx, cpoPath, path, DOUBLE, 1, &dims, 0, &data);

}

int hdf5PutVect1DString(int expIdx, char *cpoPath, char *path, char **data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, STRING, 1, &dim, isTimed, data);
}

int hdf5PutVect1DInt(int expIdx, char *cpoPath, char *path, int *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, INT, 1, &dim, isTimed, data);
}

int hdf5PutVect1DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, FLOAT, 1, &dim, isTimed, data);
}

int hdf5PutVect1DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, DOUBLE, 1, &dim, isTimed, data);
}

int hdf5PutVect2DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, INT, 2, dims, isTimed, data);
}
int hdf5PutVect2DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, FLOAT, 2, dims, isTimed, data);
}

int hdf5PutVect2DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, DOUBLE, 2, dims, isTimed, data);
}

int hdf5PutVect3DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putData(expIdx, cpoPath, path, INT, 3, dims, isTimed, data);
}

int hdf5PutVect3DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putData(expIdx, cpoPath, path, FLOAT, 3, dims, isTimed, data);
}

int hdf5PutVect3DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;

    return putData(expIdx, cpoPath, path, DOUBLE, 3, dims, isTimed, data);
}

int hdf5PutVect4DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, INT, 4, dims, isTimed, data);
}

int hdf5PutVect4DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, FLOAT, 4, dims, isTimed, data);
}

int hdf5PutVect4DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, DOUBLE, 4, dims, isTimed, data);
}

int hdf5PutVect5DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, INT, 5, dims, isTimed, data);
}

int hdf5PutVect5DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, FLOAT, 5, dims, isTimed, data);
}

int hdf5PutVect5DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, DOUBLE, 5, dims, isTimed, data);
}

int hdf5PutVect6DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, INT, 6, dims, isTimed, data);
}

int hdf5PutVect6DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, FLOAT, 6, dims, isTimed, data);
}

int hdf5PutVect6DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, DOUBLE, 6, dims, isTimed, data);
}

int hdf5PutVect7DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, INT, 7, dims, isTimed, data);
}

int hdf5PutVect7DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, FLOAT, 7, dims, isTimed, data);
}

int hdf5PutVect7DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, DOUBLE, 7, dims, isTimed, data);
}



int hdf5PutIntSlice(int expIdx, char *cpoPath, char *path, int data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, INT, 0, NULL, &data);
}
int hdf5PutFloatSlice(int expIdx, char *cpoPath, char *path, float data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 0, NULL, &data);
}

int hdf5PutDoubleSlice(int expIdx, char *cpoPath, char *path, double data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 0, NULL, &data);
}

int hdf5PutStringSlice(int expIdx, char *cpoPath, char *path, char *data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, STRING, 0, NULL, &data);
}

int hdf5PutVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, INT, 1, &dim, data);
}

int hdf5PutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 1, &dim, data);
}

int hdf5PutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 1, &dim, data);
}

int hdf5PutVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, INT, 2, dims, data);
}
int hdf5PutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 2, dims, data);
}

int hdf5PutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 2, dims, data);
}

int hdf5PutVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, INT, 3, dims, data);
}

int hdf5PutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 3, dims, data);
}

int hdf5PutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 3, dims, data);
}

int hdf5PutVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, INT, 4, dims, data);
}

int hdf5PutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 4, dims, data);
}
int hdf5PutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 4, dims, data);
}

int hdf5PutVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, INT, 5, dims, data);
}

int hdf5PutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 5, dims, data);
}
int hdf5PutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 5, dims, data);
}


int hdf5PutVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putDataSlice(expIdx, cpoPath, path, INT, 6, dims, data);
}

int hdf5PutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim5;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 6, dims, data);
}
int hdf5PutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 6, dims, data);
}



int hdf5ReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 0, NULL, &data);
}
int hdf5ReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 0, NULL, &data);
}

int hdf5ReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 0, NULL, &data);
}

int hdf5ReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, STRING, 0, NULL, &data);
}

int hdf5ReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 1, &dim, data);
}

int hdf5ReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 1, &dim, data);
}

int hdf5ReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 1, &dim, data);
}

int hdf5ReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 2, dims, data);
}
int hdf5ReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 2, dims, data);
}

int hdf5ReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 2, dims, data);
}

int hdf5ReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 3, dims, data);
}

int hdf5ReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 3, dims, data);
}

int hdf5ReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 3, dims, data);
}

int hdf5ReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 4, dims, data);
}


int hdf5ReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 4, dims, data);
}
int hdf5ReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 4, dims, data);
}

int hdf5ReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 5, dims, data);
}


int hdf5ReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 5, dims, data);
}
int hdf5ReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 5, dims, data);
}


int hdf5ReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 6, dims, data);
}


int hdf5ReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 6, dims, data);
}
int hdf5ReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
   return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 6, dims, data);
}



int hdf5GetString(int expIdx, char *cpoPath, char *path, char **data)
{
    int dims[1];
    return getData(expIdx, cpoPath, path, STRING, 1, dims, data);
}

int hdf5GetFloat(int expIdx, char *cpoPath, char *path, float *data)
{
    int dims[1];
    int status;
    float *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, FLOAT, 1, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetInt(int expIdx, char *cpoPath, char *path, int *data)
{
    int dims[1];
    int status;
    int *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, INT, 1, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetDouble(int expIdx, char *cpoPath, char *path, double *data)
{
    int dims[1];
    int status;
    double *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, DOUBLE, 1, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim)
{
    return getData(expIdx, cpoPath, path, STRING_VECTOR, 1, dim, (char **)data);
}

int hdf5GetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim)
{
    return getData(expIdx, cpoPath, path, INT, 1, dim, (char **)data);
}

int hdf5GetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim)
{
    return getData(expIdx, cpoPath, path, FLOAT, 1, dim, (char **)data);
}

int hdf5GetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim)
{
    return getData(expIdx, cpoPath, path, DOUBLE, 1, dim, (char **)data);
}

int hdf5GetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status =  getData(expIdx, cpoPath, path, INT, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int hdf5GetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int hdf5GetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status =  getData(expIdx, cpoPath, path, DOUBLE, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int hdf5GetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status =  getData(expIdx, cpoPath, path, INT, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int hdf5GetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int hdf5GetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int hdf5GetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int hdf5GetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int hdf5GetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int hdf5GetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}

int hdf5GetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}

int hdf5GetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}
int hdf5GetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4,
    int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}

int hdf5GetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}

int hdf5GetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}
int hdf5GetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4,
    int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int hdf5GetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int hdf5GetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int hdf5BeginCPOGetSlice(int expIdx, char *cpoPath, double time)
{
    return getSliceIdxs(expIdx, cpoPath, time);

}

int hdf5GetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode)
{
    sprintf(errmsg, "getStringSlice not supported\n");
    return -1;

}
int hdf5GetIntSlice(int expIdx, char *cpoPath, char *path, int *data, double time, double *retTime, int interpolMode)
{
    int y1, y2;
    int status;
    int dims[16];
    int *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION:
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
			*retTime = time;
			break;
		case CLOSEST_SAMPLE:
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
}

int hdf5GetFloatSlice(int expIdx, char *cpoPath, char *path, float *data, double time, double *retTime, int interpolMode)
{
    float y1, y2;
    int status;
    int dims[16];
    float *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION:
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
			*retTime = time;
			break;
		case CLOSEST_SAMPLE:
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
}

int hdf5GetDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, double time, double *retTime, int interpolMode)
{
    double y1, y2;
    int status;
    int dims[16];
    double *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION:
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
			*retTime = time;
			break;
		case CLOSEST_SAMPLE:
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
}

int hdf5GetVect1DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
}
int hdf5GetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
}

int hdf5GetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim, double time, double *retTime, int interpolMode)
{
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
}

int hdf5GetVect2DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
}

int hdf5GetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
}

int hdf5GetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{

    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
}

int hdf5GetVect3DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
}

int hdf5GetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
}

int hdf5GetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{

    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
}
//////////////////
int hdf5GetVect4DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]* dims[3];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
}

int hdf5GetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
}

int hdf5GetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{

    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
}
///////////////
int hdf5GetVect5DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]* dims[3]*dims[4];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
}

int hdf5GetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
}

int hdf5GetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{

    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
}


int hdf5GetVect6DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{

    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]* dims[3]*dims[4]*dims[5];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
}

int hdf5GetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{

    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4]*dims[5];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
}

int hdf5GetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{

    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];

    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4]*dims[5];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION:
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE:
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
}




hdf5GetDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int i, savedNDims, dataSize, strLen;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2);
    char *outData, currPath[1024];
    char *dataName = malloc(strlen(path)+1);
    hid_t group, dataset, datatype, inDatatype, savedDatatype, savedDataspace;
    int exists;
    hsize_t hdims[32], savedDims[32], savedMaxdims[32];
    hid_t cParms;
    hsize_t chunkDims[32];

    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[expIdx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists <= 0)
    {
        sprintf(errmsg, "Error: dataset %s not found", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if(dataset < 0)
    {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Check dataspace
    savedDataspace = H5Dget_space(dataset);
    savedNDims = H5Sget_simple_extent_ndims(savedDataspace);
    if(H5Sget_simple_extent_dims(savedDataspace, savedDims, savedMaxdims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    *numDims = savedNDims;
    if(savedNDims > 0)
        *dim1 = savedDims[0];
    if(savedNDims > 1)
        *dim2 = savedDims[1];
    if(savedNDims > 2)
        *dim3 = savedDims[2];
    if(savedNDims > 3)
        *dim4 = savedDims[3];
    if(savedNDims > 4)
        *dim5 = savedDims[4];
    if(savedNDims > 5)
        *dim6 = savedDims[5];
    if(savedNDims > 6)
        *dim7 = savedDims[6];


    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Dclose(dataset);
    H5Sclose(savedDataspace);
    return 0;
}


int hdf5DeleteData(int expIdx, char *cpoPath, char *path)
{
    int i;
    char *groupName = malloc(strlen(cpoPath) + strlen(path) + 2);
    char *dataName = malloc(strlen(path)+1);
    char currPath[1024];
    hid_t group;
    int exists;

    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else
    {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i+1]);
    }

    group = H5Gopen( hdf5Files[expIdx], (const char *)groupName, H5P_DEFAULT);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(exists < 0)
    {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if(exists)
    {
        if(H5Ldelete(group, dataName, H5P_DEFAULT) < 0)
        {
            sprintf(errmsg, "Error deleting dataset  %s", dataName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    return 0;

}

int hdf5BeginCPOGet(int expIdx, char *path, int isTimed, int *retSamples)
{
    int status;
    double *times;
    int dim;

    if(!isTimed)
            *retSamples = 1;
    else
    {
        status = hdf5GetVect1DDouble(expIdx, path, "time", &times, &dim);
        if(status) return status;
        *retSamples = dim;
        free((char *)times);
    }
    return 0;
}



int hdf5EndCPOGet(int expIdx, char *path) {return 0;}
int hdf5EndCPOGetSlice(int expIdx, char *path) {return 0;}
int hdf5BeginCPOPut(int expIdx, char *path) {return 0;}
int hdf5EndCPOPut(int expIdx, char *path) {return 0;}
int hdf5BeginCPOPutTimed(int expIdx, char *path, int samples, double *inTimes) {return 0;}
int hdf5EndCPOPutTimed(int expIdx, char *path) {return 0;}
int hdf5BeginCPOPutNonTimed(int expIdx, char *path) {return 0;}
int hdf5EndCPOPutNonTimed(int expIdx, char *path) {return 0;}
int hdf5BeginCPOPutSlice(int expIdx, char *path)
{
    int nDims,dim1;

    // get the current number of time slices.
    // This will be used when by leaves of slice objects to know whether some times have been skipped
    // (because they were empty and therefore not written to the database)
    if (hdf5GetDimension(expIdx,path,"time",&nDims,&dim1,NULL,NULL,NULL,NULL,NULL,NULL)<0) {
        sliceIdx1 = 0;      // there is no slice yet
    } else {
        sliceIdx1 = dim1;   //we are going to write right after the last slice
    }
    return 0;
}

int hdf5EndCPOPutSlice(int expIdx, char *path) {return 0;}
int hdf5BeginCPOReplaceLastSlice(int expIdx, char *path)
{
    int nDims,dim1;

    // get the current number of time slices.
    // This will be used when by leaves of slice objects to know whether some times have been skipped
    // (because they were empty and therefore not written to the database)
    if (hdf5GetDimension(expIdx,path,"time",&nDims,&dim1,NULL,NULL,NULL,NULL,NULL,NULL)<0 || dim1==0) {
        sliceIdx1 = 0;      // there is no slice yet
    } else {
        sliceIdx1 = dim1-1;  // we are going to overwrite last slice
    }
    return 0;

}

int hdf5EndCPOReplaceLastSlice(int expIdx, char *path) {return 0;}


/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

#define CREATE 1
#define NO_CREATE 0
#define CLEAR 1
#define NO_CLEAR 0

/** Open a group.
 * If set, create_flag forces creation of the group and all necessary intermediate groups.
 * If set, clear_flag forces to erase the current content of the group */
static hid_t openGroup(hid_t root, const char *relPath, int create_flag, int clear_flag)
{
    char field[1024];
    const char *src;
    char *dst;
    hid_t group = root, newGroup;

    for (src = relPath;;src++) {

        // field next field name
        for (dst=field; *src!=0 && *src!='/'; dst++, src++)
            *dst = *src;
        *dst = 0;

        if (H5Lexists(group, field, H5P_DEFAULT)) {
            if (*src==0 && clear_flag) {
                // if it was the last field and we are asked to clear it
                H5Ldelete(group, field, H5P_DEFAULT);
                newGroup = H5Gcreate(group, (const char *)field, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            } else {
                // open the group
                newGroup = H5Gopen(group, (const char *)field, H5P_DEFAULT);
            }
        } else {
            if (create_flag) {
                // if it doesn't exist, then create it
                newGroup = H5Gcreate(group, (const char *)field, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            } else {
                newGroup = -1;
            }
        }
        if (newGroup<0) {
            if (group!=root) H5Gclose(group);
            sprintf(errmsg, "Error opening HDF5 Group %s", relPath);
            return -1;
        }

        // release old group handle
        if (group!=root) H5Gclose(group);
        group = newGroup;

        // if it was last field then return
        if (*src==0)
            return group;
    }
}

/** Append data to a field
 * If we are dealing with non timed data, the field is assumed to have
 * been cleared previously
 * The size of the field can vary between slices.
 * The size of the field is therefore stored in an associated "_size" field
 * nDims should be 0 for scalars
 * If the specified slice is not the next slice to be filled
 * then we insert empty slices */
static int putDataSliceInObject(void *obj, char *path, int index, int type, int nDims, int *dims, void *data)
{
    obj_t *object = (obj_t *)obj;
    hid_t hObj = object->handle;

    int i, totSize;
    int exists;
    char tmpstr[1024], dataName[1024], groupName[1024], sizeName[1024];
    hid_t group, dataset, datatype, memDatatype, dataspace, memDataspace;
    hid_t sizeset, sizetype, memSizetype, sizespace,memSizespace;
    hid_t stringType;
    hid_t cParms;
    hsize_t hdims[1], extDims[1], startOut[1];
    hsize_t countOut[1];
    hsize_t maxDims[1];
    hsize_t chunkDims[1];
    hvl_t *vl_data, *vl_size;
    int zero = 0;
    long lastIdx;

    // compute total size of data
    for(i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
/*    if(totSize == 0)
        return 0;*/

    // split path into group name and field name
    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    strcpy(groupName, path);
    groupName[i] = 0;
    strcpy(dataName, &path[i+1]);

    // Generate correct group path by substituting leading field by index number
    for (i=0; groupName[i]!='/' && groupName[i]!=0; i++);
    if (groupName[i]==0)
        sprintf(tmpstr,"%d",index);
    else
        sprintf(tmpstr,"%d/%s",index,&groupName[i+1]);
    strcpy(groupName,tmpstr);

    // open container group
    group = openGroup(hObj,groupName,CREATE,NO_CLEAR);
    if(group < 0)
    {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        return -1;
    }

    //====================================
    // Write dimensions (for arrays only)
    //====================================

    if (nDims != 0) {

        // create field name to store the dimensions
        sprintf(sizeName,"%s_size",dataName);

        // define datatypes
        sizetype = H5Tvlen_create(H5T_STD_I32BE);
        memSizetype = H5Tvlen_create(H5T_NATIVE_INT);

        // open/create the dataset
        exists = H5Lexists(group, sizeName, H5P_DEFAULT);
        if(!exists) //The first time we need to create the dataset
        {
            // create and empty but extendible dataspace
            hdims[0] = 0;
            maxDims[0] = H5S_UNLIMITED;
            sizespace = H5Screate_simple(1, hdims, maxDims);

            // define chunk size for the dataspace
            chunkDims[0] = SLICE_BLOCK;
            cParms = H5Pcreate (H5P_DATASET_CREATE);
            H5Pset_chunk( cParms, 1, chunkDims);

            // create dataset
            sizeset = H5Dcreate(group, sizeName, sizetype, sizespace, H5P_DEFAULT,  cParms, H5P_DEFAULT);
            if(sizeset < 0)
            {
                sprintf(errmsg, "Error creating dataset %s in group %s", sizeName, groupName);
                return -1;
            }
            H5Pclose(cParms);
        }
        else //open the dataset
        {
            sizeset = H5Dopen(group, sizeName, H5P_DEFAULT);
            if(sizeset < 0)
            {
                sprintf(errmsg, "Error opening dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            // get dataspace
            sizespace = H5Dget_space(sizeset);
            if(sizespace < 0)
            {
                sprintf(errmsg, "Error getting dataset %s in group %s", sizeName, groupName);
                return -1;
            }
        }

        // get current size
        if(H5Sget_simple_extent_dims(sizespace, hdims, maxDims) < 0)
        {
            sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
            return -1;
        }
        lastIdx = (long)hdims[0]-1;

        if (object->timeIdx<lastIdx)
        {
            sprintf(errmsg, "Internal error: trying to overwrite a slice in %s, group %s", sizeName, groupName);
            return -1;
        }

        if (object->timeIdx==lastIdx)
        {   // we are replacing the last slice
            startOut[0] = lastIdx;
            countOut[0] = 1;
        }
        else
        {   // we are writing a new slice

            // increase size
            extDims[0] = object->timeIdx+1;

            if(H5Dset_extent(sizeset, extDims) < 0)
            {
                sprintf(errmsg, "Internal error: extend failed in %s, group %s", sizeName, groupName);
                return -1;
            }

            // get new dataspace
            H5Sclose(sizespace);
            sizespace = H5Dget_space(sizeset);

            startOut[0] = hdims[0];
            countOut[0] = object->timeIdx+1-hdims[0];
        }

        // select only last slices for writing
        // including those that are going to be created empty
        if(H5Sselect_hyperslab(sizespace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0)
        {
            sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", sizeName, groupName);
            return -1;
        }

        // the memory dataspace will contain all the new slices
        memSizespace = H5Screate(H5S_SIMPLE);
        H5Sset_extent_simple(memSizespace,1,countOut,countOut);

        // prepare buffer for writing
        vl_size = (hvl_t *)malloc(countOut[0]*sizeof(hvl_t));

        // if the current index is not the next one in the dataset
        // then we need to fill with empty rows
        for (i=0; i<countOut[0]-1; i++) {
            vl_size[i].len = 1;
            vl_size[i].p = &zero;
        }
        vl_size[i].len = nDims;
        vl_size[i].p = dims;

        //Write in the sizeset
        if(H5Dwrite(sizeset, memSizetype, memSizespace, sizespace, H5P_DEFAULT, vl_size)<0)
        {
            sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
            free(vl_size);
            return -1;
        }

        // clean up
        free(vl_size);
        H5Tclose(sizetype);
        H5Tclose(memSizetype);
        H5Dclose(sizeset);
        H5Sclose(sizespace);
        H5Sclose(memSizespace);
    }

    //====================================
    //            Write data
    //====================================

    // define datatypes
    switch(type)
    {
        case INT:
            datatype = H5Tvlen_create(H5T_STD_I32BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tvlen_create(H5T_IEEE_F32BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tvlen_create(H5T_IEEE_F64BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            stringType = H5Tcopy(H5T_C_S1);
            H5Tset_size(stringType,H5T_VARIABLE);
            datatype = H5Tvlen_create(stringType);
            memDatatype = H5Tcopy(datatype);
            H5Tclose(stringType);
            break;
    }

    // open/create the dataset
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if(!exists) //The first time we need to create the dataset
    {
        // create and empty but extendible dataspace
        hdims[0] = 0;
        maxDims[0] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(1, hdims, maxDims);

        // define chunk size for the dataspace
        chunkDims[0] = SLICE_BLOCK;
        cParms = H5Pcreate (H5P_DATASET_CREATE);
        H5Pset_chunk( cParms, 1, chunkDims);

        // create dataset
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT,  cParms, H5P_DEFAULT);
        if(dataset < 0)
        {
            sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
            return -1;
        }
        H5Pclose(cParms);

   }
    else //open the dataset
    {
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if(dataset < 0)
        {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            return -1;
        }

        // get dataspace
        dataspace = H5Dget_space(dataset);
        if(dataspace < 0)
        {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            return -1;
        }

    }

    // get current size
    if(H5Sget_simple_extent_dims(dataspace, hdims, maxDims) < 0)
    {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        return -1;
    }
    lastIdx = (long)hdims[0]-1;

    if (object->timeIdx<lastIdx)
    {
        sprintf(errmsg, "Internal error: trying to overwrite a slice in %s, group %s", dataName, groupName);
        return -1;
    }

    if (object->timeIdx==lastIdx)
    {   // we are replacing the last slice
        startOut[0] = lastIdx;
        countOut[0] = 1;
    }
    else
    {   // we are writing a new slice

        // increase size
        extDims[0] = object->timeIdx+1;
        if(H5Dset_extent(dataset, extDims) < 0)
        {
            sprintf(errmsg, "Internal error: extend failed in %s, group %s", dataName, groupName);
            return -1;
        }

        // get new dataspace
        H5Sclose(dataspace);
        dataspace = H5Dget_space(dataset);

        startOut[0] = hdims[0];
        countOut[0] = object->timeIdx+1-hdims[0];
    }

    // select only last slices for writing
    // including those that are going to be created empty
    if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0)
    {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        return -1;
    }

    // the memory dataspace will contain all the new slices
    memDataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(memDataspace,1,countOut,countOut);

    // prepare buffer for writing
    vl_data = (hvl_t *)malloc(countOut[0]*sizeof(hvl_t));
    // if the current index is not the next one in the dataset
    // then we need to fill with empty rows
    for (i=0; i<countOut[0]-1; i++) {
        vl_data[i].len = 0;
        vl_data[i].p = NULL;
    }
    vl_data[i].len = totSize;
    vl_data[i].p = data;

    //Write in the dataset
    if(H5Dwrite(dataset, memDatatype, memDataspace, dataspace, H5P_DEFAULT, vl_data)<0)
    {
        sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
        free(vl_data);
        return -1;
    }

    // clean up
    free(vl_data);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(memDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}


/** Read a single slice from a field
 *  The size of the field is read in the associated "_size" field
 *  nDims should be 0 for scalars
 *  If the required slice does not exist (is higher than the last slice)
 *  then we return an empty result ("empty" value for scalars, size 0 for arrays)
 *  It is possible to read only the size (not the data) by setting type to DIMENSION */
static int getDataSliceFromObject(void *obj, char *path, int index, int type, int nDims, int *dims, void **data)
{
    obj_t *object = (obj_t *)obj;
    hid_t hObj = object->handle;
    int timeIdx = object->timeIdx;

    int i, totSize;
    char tmpstr[1024], dataName[1024], groupName[1024], sizeName[1024];
    hid_t group, dataset, datatype, memDatatype, dataspace, memDataspace;
    hid_t sizeset, sizetype, memSizetype, sizespace,memSizespace;
    hid_t stringType;
    hid_t cParms;
    hsize_t hdims[1], startOut[1];
    hsize_t countOut[1];
    hsize_t maxDims[1];
    hsize_t chunkDims[1];
    hvl_t vl_data, vl_size;
    int nDimsRead;

    // split path into group name and field name
    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    strcpy(groupName, path);
    groupName[i] = 0;
    strcpy(dataName, &path[i+1]);

    // Generate correct group path by substituting leading field by index number
    for (i=0; groupName[i]!='/' && groupName[i]!=0; i++);
    if (groupName[i]==0)
        sprintf(tmpstr,"%d",index);
    else
        sprintf(tmpstr,"%d/%s",index,&groupName[i+1]);
    strcpy(groupName,tmpstr);

    // open container group
    group = openGroup(hObj,groupName,NO_CREATE,NO_CLEAR);

    // if the field cannot be opened then consider that it is empty
    if(group < 0 || !H5Lexists(group, dataName, H5P_DEFAULT))
    {
        for (i=0; i<nDims; i++) {
            dims[i] = 0;
        }
        *data = NULL;
    }
    else
    {
        //===================================
        // Read dimensions (for arrays only)
        //===================================

        if (nDims>0) {

            // create field name to read dimensions from
            sprintf(sizeName,"%s_size",dataName);

            // define datatypes
            sizetype = H5Tvlen_create(H5T_STD_I32BE);
            memSizetype = H5Tvlen_create(H5T_NATIVE_INT);

            // memory dataspace is a single set of dimensions (single slice)
            hdims[0] = 1;
            memSizespace = H5Screate(H5S_SIMPLE);
            H5Sset_extent_simple(memSizespace,1,hdims,hdims);

            // open the dataset
            sizeset = H5Dopen(group, sizeName, H5P_DEFAULT);
            if(sizeset < 0)
            {
                sprintf(errmsg, "Error opening dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            // get dataspace
            sizespace = H5Dget_space(sizeset);
            if(sizespace < 0)
            {
                sprintf(errmsg, "Error getting dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            //Get last dimension
            if(H5Sget_simple_extent_dims(sizespace, hdims, maxDims) < 0)
            {
                sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
                return -1;
            }

            if (timeIdx>=hdims[0]) // we are trying to read a non existing slice
            {
                for (i=0; i<nDims; i++) {
                    dims[i] = 0;
                }
            } else {

                //Select only one slice for reading
                startOut[0] = timeIdx;
                countOut[0] = 1;
                if(H5Sselect_hyperslab(sizespace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0)
                {
                    sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", sizeName, groupName);
                    return -1;
                }

                // Read from the sizeset
                if(H5Dread(sizeset, memSizetype, memSizespace, sizespace, H5P_DEFAULT, &vl_size)<0)
                {
                    sprintf(errmsg, "Error reading dataset %s in group %s", dataName, groupName);
                    return -1;
                }

                // extract dimensions
                nDimsRead = vl_size.len;
                for (i=0, totSize=1; i<nDimsRead; i++) {
                    dims[i] = ((int *)(vl_size.p))[i];
                    totSize*=dims[i];
                }
                H5Dvlen_reclaim(memSizetype, memSizespace, H5P_DEFAULT, &vl_size);
            }

            // clean up
            H5Tclose(sizetype);
            H5Tclose(memSizetype);
            H5Dclose(sizeset);
            H5Sclose(sizespace);
            H5Sclose(memSizespace);
        }

        // if we only want the dimensions of the array, then stop here
        if (type==DIMENSION) {
            H5Gclose(group);
            *data = malloc(sizeof(int));
            **(int **)data = nDimsRead; // return the number of dimensions as data
            return 0;
        }

        //===================================
        //            Read data
        //===================================

        // define datatypes
        switch(type)
        {
            case INT:
                datatype = H5Tvlen_create(H5T_STD_I32BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_INT);
                break;
            case FLOAT:
                datatype = H5Tvlen_create(H5T_IEEE_F32BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_FLOAT);
                break;
            case DOUBLE:
                datatype = H5Tvlen_create(H5T_IEEE_F64BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_DOUBLE);
                break;
            case STRING:
                stringType = H5Tcopy(H5T_C_S1);
                H5Tset_size(stringType,H5T_VARIABLE);
                datatype = H5Tvlen_create(stringType);
                memDatatype = H5Tcopy(datatype);
                H5Tclose(stringType);
                break;
        }

        // memory dataspace is a single set of dimensions (single slice)
        hdims[0] = 1;
        memDataspace = H5Screate(H5S_SIMPLE);
        H5Sset_extent_simple(memDataspace,1,hdims,hdims);

        // open the dataset
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if(dataset < 0)
        {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            return -1;
        }

        // get dataspace
        dataspace = H5Dget_space(dataset);
        if(dataspace < 0)
        {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            return -1;
        }

        //Get last dimension
        if(H5Sget_simple_extent_dims(dataspace, hdims, maxDims) < 0)
        {
            sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
            return -1;
        }

        if (timeIdx>=hdims[0]) // we are trying to read a non existing slice
        {
            *data = NULL;
        } else {

            //Select only one slice for reading
            startOut[0] = timeIdx;
            countOut[0] = 1;
            if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0)
            {
                sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
                return -1;
            }

            // Read from the dataset
            if(H5Dread(dataset, memDatatype, memDataspace, dataspace, H5P_DEFAULT, &vl_data)<0)
            {
                sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
                return -1;
            }
            *data = vl_data.p;
        }

        // clean up
        H5Gclose(group);
        H5Tclose(datatype);
        H5Tclose(memDatatype);
        H5Dclose(dataset);
        H5Sclose(dataspace);
        H5Sclose(memDataspace);
    }

    // if it is a scalar and we could not read it then return
    // the default "empty" value
    if (*data==NULL && nDims==0) {
        switch(type)
        {
            case INT:
                *data = malloc(sizeof(int));
                **(int **)data = EMPTY_INT;
                break;
            case FLOAT:
                *data = malloc(sizeof(float));
                **(float **)data = EMPTY_FLOAT;
                break;
            case DOUBLE:
                *data = malloc(sizeof(double));
                **(double **)data = EMPTY_DOUBLE;
                break;
            case STRING:
                *data = malloc(sizeof(char*));
                **(char ***)data = (char *)malloc(sizeof(char));
                ***(char ***)data = 0;
                break;
        }
    }
    return 0;
}

/** Find the last element in the list of objects
 *  that are contained in the root object */
static obj_t *lastDescendant(obj_t *root)
{
    obj_t *obj;
    for (obj=root; obj->nextObj!=NULL; obj=obj->nextObj);
    return obj;
}

/** Open or create a group corresponding to an object (for writing).
 *  Stores the group handle in an object structure.
 *  If necessary, delete current content of the group. */
void *hdf5BeginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed)
{
    obj_t *object = (obj_t *)obj;
    obj_t *output;
    hid_t group;
    char *groupName, *tmpstr;
    char *subName;
    hid_t root;
    int clearContent;

    groupName = (char *)malloc(strlen(relPath)+20);
    output = (obj_t *)malloc(sizeof(obj_t));
    if (output==NULL) {
        sprintf(errmsg, "Error in hdf5BeginObject: out of memory");
        return NULL;
    }

    if (obj==NULL) {
        //-------------- Object is at top level --------------
        if (isTimed==NON_TIMED)
            sprintf(groupName,"%s/non_timed",relPath);
        else
            sprintf(groupName,"%s/timed",relPath);
        root = hdf5Files[expIdx];
    } else {
        //-------------- Object is inside another object --------------

        // if this object is a slice of a time-dependent object, then the group
        // was already opened and initialized by the containing object (that contains all the times).
        // We recognize this case because the field name is, by convention, "ALLTIMES"

        if (!strcmp(relPath,"ALLTIMES")) {
            output->handle = object->handle;
            output->dim = -99;       // not used for output objects
            output->nextObj = NULL;  // this is the last object for now
            if (object->timeIdx<0)  // we are doing a put
                output->timeIdx = index;
            else                            // we are doing a putSlice
                output->timeIdx = object->timeIdx;
            lastDescendant(object)->nextObj = output;
            free(groupName);
            return output;
        }

        // other cases correspond to real nesting of arrays of structures,
        // so we can proceed
        sprintf(groupName,"%s",relPath);
        root = object->handle;
        // Generate correct path by substituting leading field by index number
        for (subName=groupName; *subName!='/' && *subName!=0; subName++);
        if (*subName==0) {
            sprintf(errmsg, "Error in hdf5BeginObject: malformed path: %s",groupName);
            free(groupName);
            free(output);
            return NULL;
        }
        subName++;
        tmpstr = (char *)malloc(strlen(relPath)+20);
        sprintf(tmpstr,"%d/%s",index,subName);
        strcpy(groupName,tmpstr);
        free(tmpstr);
    }

    // we are asked to delete all the data for this timed object
    // or it is a non-timed object and we are going to overwrite it
    if (isTimed==TIMED_CLEAR || isTimed==NON_TIMED) {
        clearContent = CLEAR;
    } else {
        clearContent = NO_CLEAR;
    }

    // open group (create if it doesn't exist)
    group = openGroup(root, (const char *)groupName,CREATE,clearContent);
    free(groupName);
    if (group<0) {
        sprintf(errmsg, "Error in hdf5BeginObject: could not open/create group %s",groupName);
        free(output);
        return NULL;
    }

    // store handle and return it
    output->handle = group;
    output->dim = -99;       // not used for output objects
    output->nextObj = NULL;  // this is the last object for now

    // index of the slice we need to write to
    if (obj==NULL) {        // object is a top level
        if (isTimed==TIMED) // we are doing a putSlice
            output->timeIdx = sliceIdx1;
        else if (isTimed==TIMED_CLEAR) // we are doing a put
            output->timeIdx = -1;      // the index is going to be defined by the next hdf5BeginObject
        else   // non timed object
            output->timeIdx = 0;
    } else { // for nested objects, inherit the slice index
        output->timeIdx = object->timeIdx;
    }

    // if this object is not at the top level, then add it to the list
    // of descendants of the containing object
    if (obj!=NULL) {
        lastDescendant(object)->nextObj = output;
    }

    return (void *)output;
}

/** Open the group corresponding to an object
 *  and store its handle */
int hdf5GetObject(int expIdx, char *hdf5Path, char *cpoPath, void **obj, int isTimed)
{
    char fullPath[1024];
    H5G_info_t group_info;
    obj_t *newObj = (obj_t *)malloc(sizeof(obj_t));
    int nDims, dim1;

    // timed and non-timed objects are kept in different fields
    if (isTimed)
        sprintf(fullPath,"%s/%s/timed",hdf5Path,cpoPath);
    else
        sprintf(fullPath,"%s/%s/non_timed",hdf5Path,cpoPath);

    // try to open the group corresponding to the object
    newObj->handle = openGroup(hdf5Files[expIdx],fullPath,NO_CREATE,NO_CLEAR);

    if (newObj->handle<0) {
        // if it could not be read, then return an empty object
        newObj->dim = 0;
    } else if (!isTimed) {
        // if the object is not timed, then the size is the current number of elements
        if (H5Gget_info(newObj->handle,&group_info)<0) {
            free(newObj);
            *obj = NULL;
            sprintf(errmsg,"Error in hdf5GetObject: cannot get information for group %s",fullPath);
            return -1;
        }
        newObj->dim = group_info.nlinks;
    } else {
        // if the object is timed, then the size is the number of time slices
        if (hdf5GetDimension(expIdx,hdf5Path,"time",&nDims,&dim1,NULL,NULL,NULL,NULL,NULL,NULL)<0) {
            free(newObj);
            *obj = NULL;
            sprintf(errmsg,"Error in hdf5GetObject: cannot get time for group %s",fullPath);
            return -1;
        }
        newObj->dim = dim1;
    }

    // If the object is not time-dependent then we are going to read the first (and only) slice.
    // Otherwise the slice to be read is going to be defined by the next hdf5GetObjectFromObject.
    // For now, setting timeIdx to -1 means, by convention, that we want to read all time slices.
    if (!isTimed)
        newObj->timeIdx = 0;
    else
        newObj->timeIdx = -1;

    // This is necessarily a top-level object,
    // we yet have to read the contained objects
    newObj->nextObj = NULL;

    *obj = newObj;
    return 0;
}

/** Open the group corresponding to an object slice
 *  and store its handle */
int hdf5GetObjectSlice(int expIdx, char *hdf5Path, char *cpoPath, double time, void **obj)
{
    char fullPath[1024];
    obj_t *newObj = (obj_t *)malloc(sizeof(obj_t));

    // timed and non-timed objects are kept in different fields
    // here we are dealing with timed fields
    sprintf(fullPath,"%s/%s/timed",hdf5Path,cpoPath);

    // try to open the group corresponding to the object
    newObj->handle = openGroup(hdf5Files[expIdx],fullPath,NO_CREATE,NO_CLEAR);

    if (newObj->handle<0) {
        // if it could not be read, then return an empty object
        newObj->dim = 0;
    } else {
        // otherwise this object contains a single slice
        newObj->dim = 1;
    }

    // The slice to be read by the next hdf5GetObjectFromObject is the closest slice in time
    if(sliceIdx2<0 || time - sliceTime1 < sliceTime2 - time)
    {
        newObj->timeIdx = sliceIdx1;
    }
    else
    {
        newObj->timeIdx = sliceIdx2;
    }

    // This is necessarily a top-level object,
    // we yet have to read the contained objects
    newObj->nextObj = NULL;

    *obj = newObj;
    return 0;
}

/** Open an object inside another object
*/
int hdf5GetObjectFromObject(void *obj, char *hdf5Path, int idx, void **dataObj)
{
    char groupName[1024];
    char *subName;
    H5G_info_t group_info;
    obj_t *newObj = (obj_t *)malloc(sizeof(obj_t));

    //------------------------------------------------------------------------------
    // if we are starting to read a slice of a time-dependent object
    // (in this case it is contained in a bigger object that contains all the times).
    // We recognize this case because the field name is, by convention, "ALLTIMES"
    //------------------------------------------------------------------------------

    if (!strcmp(hdf5Path,"ALLTIMES")) {

        // we have already opened the HDF5 group when opening the containing object,
        // so just copy the handle
        newObj->handle = ((obj_t *)obj)->handle;

        // get the current number of elements
        if (H5Gget_info(newObj->handle,&group_info)<0) {
            free(newObj);
            sprintf(errmsg,"Error in hdf5GetObjectFromObject: cannot get information for group %s",hdf5Path);
            *dataObj = NULL;
            return -1;
        }
        newObj->dim = group_info.nlinks;

        // by convention, timeIdx has been set to -1 if we are dealing with a get rather than a getSlice.
        // In that case, use the given index.
        // Otherwise, timeIdx has been set to the index of the slice to read
        if (((obj_t *)obj)->timeIdx<0) {
            newObj->timeIdx = idx;
        } else {
            newObj->timeIdx = ((obj_t *)obj)->timeIdx;
        }

    //------------------------------------------------------------------------------
    // other cases correspond to real nesting of arrays of structures
    //------------------------------------------------------------------------------

    } else {
        // Generate correct path by substituting leading field by index number
        for (subName=hdf5Path; *subName!='/' && *subName!=0; subName++);
        if (*subName==0) {
            sprintf(errmsg, "Error in hdf5GetObjectFromObject: malformed path: %s",hdf5Path);
            free(newObj);
            *dataObj = NULL;
            return -1;
        }
        subName++;
        sprintf(groupName,"%d/%s",idx,subName);

        // try to open the group corresponding to the object
        newObj->handle = openGroup(((obj_t *)obj)->handle,groupName,NO_CREATE,NO_CLEAR);

        if (newObj->handle<0) {
            // if it could not be read, then return an empty object
            newObj->dim = 0;
        } else {
            // otherwise get the current number of elements
            if (H5Gget_info(newObj->handle,&group_info)<0) {
                free(newObj);
                sprintf(errmsg,"Error in hdf5GetObjectFromObject: cannot get information for group %s",hdf5Path);
                *dataObj = NULL;
                return -1;
            }
            newObj->dim = group_info.nlinks;
        }

        // Inherit the time to be read from the containing object
        newObj->timeIdx = ((obj_t *)obj)->timeIdx;
    }

    newObj->nextObj = NULL;  // this is the last object for now

    // add this object to the list of descendants of the containing object
    lastDescendant((obj_t *)obj)->nextObj = newObj;

    *dataObj = (void *)newObj;

    return 0;
}

/** Free memory and release HDF5 handles
 *  for the specified object and all its descendants */
void hdf5ReleaseObject(void *obj)
{
    obj_t *currentObj, *nextObj;
    hid_t handle=-1;
    hid_t topHandle = ((obj_t *)obj)->handle;

    for (currentObj=(obj_t *)obj; currentObj!=NULL;) {
        nextObj = currentObj->nextObj;
        handle = currentObj->handle;
        if (handle!=topHandle || currentObj==obj) {  // do not close the group if it was already closed!
            H5Gclose(handle);                        // this can only happen for slice objects that repeat the handle of the top level object
        }
        free(currentObj);
        currentObj = nextObj;
    }
}

int hdf5PutObject(int currIdx, char *cpoPath, char *path, void *obj, int isTimed)
{
    // Since all data has been written along the way by
    // the hdf5Put...InObject functions, then there is nothing left
    // to do except freeing the memory
    hdf5ReleaseObject(obj);
    return 0;
}

void *hdf5PutIntInObject(void *obj, char *path, int idx, int data)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, INT, 0, NULL, (void *)&data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutFloatInObject(void *obj, char *path, int idx, float data)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, FLOAT, 0, NULL, (void *)&data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutDoubleInObject(void *obj, char *path, int idx, double data)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 0, NULL, (void *)&data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutStringInObject(void *obj, char *path, int idx, char *data)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, STRING, 0, NULL, (void *)&data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect1DStringInObject(void *obj, char *path, int idx, char **data, int dim)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, STRING, 1, &dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect1DIntInObject(void *obj, char *path, int idx, int *data, int dim)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, INT, 1, &dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect1DFloatInObject(void *obj, char *path, int idx, float *data, int dim)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, FLOAT, 1, &dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect1DDoubleInObject(void *obj, char *path, int idx, double *data, int dim)
{
    int status;
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 1, &dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect2DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2)
{
    int status;
    int dim[2] = {dim1,dim2};
    status = putDataSliceInObject(obj, path, idx, INT, 2, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect2DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2)
{
    int status;
    int dim[2] = {dim1,dim2};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 2, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect2DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2)
{
    int status;
    int dim[2] = {dim1,dim2};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 2, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect3DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3)
{
    int status;
    int dim[3] = {dim1,dim2,dim3};
    status = putDataSliceInObject(obj, path, idx, INT, 3, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect3DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3)
{
    int status;
    int dim[3] = {dim1,dim2,dim3};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 3, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect3DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3)
{
    int status;
    int dim[3] = {dim1,dim2,dim3};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 3, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect4DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4)
{
    int status;
    int dim[4] = {dim1,dim2,dim3,dim4};
    status = putDataSliceInObject(obj, path, idx, INT, 4, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect4DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int status;
    int dim[4] = {dim1,dim2,dim3,dim4};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 4, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect4DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int status;
    int dim[4] = {dim1,dim2,dim3,dim4};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 4, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect5DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int status;
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    status = putDataSliceInObject(obj, path, idx, INT, 5, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect5DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int status;
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 5, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect5DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int status;
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 5, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect6DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int status;
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    status = putDataSliceInObject(obj, path, idx, INT, 6, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect6DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int status;
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 6, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect6DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int status;
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 6, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect7DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int status;
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    status = putDataSliceInObject(obj, path, idx, INT, 7, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect7DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int status;
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    status = putDataSliceInObject(obj, path, idx, FLOAT, 7, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutVect7DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int status;
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    status = putDataSliceInObject(obj, path, idx, DOUBLE, 7, dim, (void *)data);
    if (status)
        obj = NULL;
    return obj;
}

void *hdf5PutObjectInObject(void *obj, char *path, int idx, void *data)
{
    return obj;
}

int hdf5GetObjectDim(void *obj)
{
    return ((obj_t *)obj)->dim;
}

/** Get the dimensions of a field by calling the regular object read function
 *  but limiting it to the size part (thanks to the type DIMENSION) */
int hdf5GetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *nDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    int *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,path,idx,DIMENSION,7,dims,(void **)&tmp);
    if (!status) {
        if (tmp==NULL)  // the field could not be read
            *nDims = 0;
        else {
            *nDims = *tmp;
            free(tmp);
        }
        *dim1 = dims[0];
        *dim2 = dims[1];
        *dim3 = dims[2];
        *dim4 = dims[3];
        *dim5 = dims[4];
        *dim6 = dims[5];
        *dim7 = dims[6];
    }
    return status;
}

int hdf5GetStringFromObject(void *obj, char *hdf5Path, int idx, char **data)
{
    int dims[1];
    int status;
    char **tmp; //we cannot use data directly because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,hdf5Path,idx,STRING,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetFloatFromObject(void *obj, char *hdf5Path, int idx, float *data)
{
    int dims[1];
    int status;
    float *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetIntFromObject(void *obj, char *hdf5Path, int idx, int *data)
{
    int dims[1];
    int status;
    int *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,hdf5Path,idx,INT,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetDoubleFromObject(void *obj, char *hdf5Path, int idx, double *data)
{
    int dims[1];
    int status;
    double *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int hdf5GetVect1DStringFromObject(void *obj, char *hdf5Path, int idx, char  ***data, int *dim)
{
    return getDataSliceFromObject(obj,hdf5Path,idx,STRING,1,dim,(void **)data);
}

int hdf5GetVect1DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim)
{
    return getDataSliceFromObject(obj,hdf5Path,idx,INT,1,dim,(void **)data);
}

int hdf5GetVect1DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim)
{
    return getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,1,dim,(void **)data);
}

int hdf5GetVect1DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim)
{
    return getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,1,dim,(void **)data);
}

int hdf5GetVect2DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int hdf5GetVect2DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int hdf5GetVect2DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int hdf5GetVect3DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int hdf5GetVect3DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int hdf5GetVect3DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int hdf5GetVect4DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int hdf5GetVect4DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int hdf5GetVect4DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int hdf5GetVect5DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int hdf5GetVect5DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int hdf5GetVect5DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int hdf5GetVect6DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int hdf5GetVect6DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int hdf5GetVect6DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int hdf5GetVect7DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,INT,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int hdf5GetVect7DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,FLOAT,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int hdf5GetVect7DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,hdf5Path,idx,DOUBLE,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int hdf5PutObjectSlice(int expIdx, char *hdf5Path, char *cpoPath, double time, void *obj)
{
    // Since all data has been written along the way by
    // the hdf5Put...InObject functions, then there is nothing left
    // to do except freeing the memory
    hdf5ReleaseObject(obj);
    return 0;
}

int hdf5ReplaceLastObjectSlice(int expIdx, char *cpoPath, char *hdf5Path, void *obj)
{
    // Since all data has been written along the way by
    // the hdf5Put...InObject functions, then there is nothing left
    // to do except freeing the memory
    hdf5ReleaseObject(obj);
    return 0;
}


