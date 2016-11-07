#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "ual_low_level.h"
#include "ual_low_level_hdf5.h"
#include "ual_low_level_mdsplus.h"


#define EXPORT


#define MAX_OPEN_EXP 10000
#define IS_MDS 1
#define IS_HDF5 2
#define IS_NONE 0

//--------------- Management of the list of objects (arrays of structures) -----------
#define MAX_OBJECTS 10000000   // maximum number of simultaneous objects

#ifndef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0, 0 } } }
#endif

static void *object[MAX_OBJECTS];       // array providing the correspondance between an integer object index and the real (void *) object pointer (for use in non C languages)
static int current_obj = -1;            // index of the last used object
//static pthread_mutex_t obj_mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP; // lock to avoid simultaneous access to the object arrays
static pthread_mutex_t obj_mutex = PTHREAD_MUTEX_INITIALIZER; // lock to avoid simultaneous access to the object arrays

// lock the object array
static void lock_obj()
{
  int status = pthread_mutex_lock(&obj_mutex);
  if (status) {
    printf("Error while locking list of objects\n");
    exit(EXIT_FAILURE);
  }
}

// unlock the object array
static void unlock_obj()
{
  int status = pthread_mutex_unlock(&obj_mutex);
  if (status) {
    printf("Error while unlocking list of objects\n");
    exit(EXIT_FAILURE);
  }
}

// clear object list before first use
static void initializeObjectList()
{
  int i;
  for (i = 0; i < MAX_OBJECTS; i++) {
    object[i] = 0;
  }
}

// get object pointer from its index
void *getObjectFromList(int idx)
{
  void *result;
  lock_obj();
  if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
    printf("Error: trying to access unknown object\n");
    exit(EXIT_FAILURE);
  }
  result = object[idx];
  unlock_obj();
  return result;
}

// change the pointer associated to an index
void replaceObjectInList(int idx, void *obj)
{
  lock_obj();
  if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
    printf("Error: trying to access unknown object\n");
    exit(EXIT_FAILURE);
  }
  object[idx] = obj;
  unlock_obj();
}

// add a new object pointer to the list
int addObjectToList(void *obj)
{
  int new_obj;
  lock_obj();
  if (current_obj == -1) {  // is it the first time we use the list?
    initializeObjectList(); // then clear the list
  }

  // find first unused location in the list
  new_obj = current_obj;
  do {
    new_obj++;
    if (new_obj >= MAX_OBJECTS) new_obj = 0;
  } while (object[new_obj] && new_obj != current_obj);

  // could not find any empty location?
  if (object[new_obj]) {
    return -1;
  }

  // assign the object to the found location
  object[new_obj] = obj;
  current_obj = new_obj;
  unlock_obj();
  return new_obj;
}

// delete an object pointer from the list
void removeObjectFromList(int idx)
{
  lock_obj();
  if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
    printf("Error: trying to remove unknown object\n");
    exit(EXIT_FAILURE);
  }
  object[idx] = 0;
  unlock_obj();
}


//------------------------------- End of object management ---------------------------

//------------------------------Logging support---------------------------------------
//#define DEBUG_UAL 1

void logOperation(char *name, char *cpoPath, char *path, int expIdx, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int
dim7, void *data, double isTimed)
{
#ifdef DEBUG_UAL
    FILE *f = fopen("ual.txt", "a");
    fprintf(f, "%s ", name);
    if(cpoPath)
        fprintf(f, "%s ", cpoPath);
    if(path)
        fprintf(f, "%s ", path);
    fprintf(f, "%d\n", expIdx);
    fclose(f);
#endif
}







//---------------------------------------------------------------------------------
static char message[10000];
char *errmsg = message;

static struct {
    int shot;
    int run;
    int idx;
    int mode;
} expInfo[MAX_OPEN_EXP];

char *imas_last_errmsg()
{
    return errmsg;
}

static int isMds(int idx, int *expIdx)
{
    *expIdx = expInfo[idx].idx;
    return (expInfo[idx].mode == IS_MDS);
}

static int getExpIdx(int idx)
{
    return expInfo[idx].idx;
}

static int getShot(int idx)
{
    return expInfo[idx].shot;
}
static int getRun(int idx)
{
    return expInfo[idx].run;
}

EXPORT int ual_get_shot(int idx) { return getShot(idx);}
EXPORT int ual_get_run(int idx) { return getRun(idx);}

static int setMdsIdx(int shot, int run, int idx)
{
    int i;
    for(i = 0; i < MAX_OPEN_EXP && expInfo[i].mode != IS_NONE; i++);
    if(i == MAX_OPEN_EXP) return -1;
    expInfo[i].mode = IS_MDS;
    expInfo[i].shot = shot;
    expInfo[i].run = run;
    expInfo[i].idx = idx;
    return i;
}
static int  setHdf5Idx(int shot, int run, int idx)
{
    int i;
    for(i = 0; i < MAX_OPEN_EXP && expInfo[i].mode != IS_NONE; i++);
    if(i == MAX_OPEN_EXP) return -1;
    expInfo[i].mode = IS_HDF5;
    expInfo[i].shot = shot;
    expInfo[i].run = run;
    expInfo[i].idx = idx;
    return i;
}

static void closeExpInfo(int idx)
{
    expInfo[idx].mode = IS_NONE;
}

/**
 * Checks if shot and run indexes fit within valid range
 */

static int checkShotRun(int shot, int run)
{
    if(shot >= 214748 || shot < 0)
        {
                sprintf(errmsg, "Invalid shot number %d. Must be between 0 and 214748", shot);
                      return 0;
                        }
      if(run < 0 || run > 9999)
          {
                  sprintf(errmsg, "Invalid run number %d. Must be between 0 and 9999", shot);
                        return 0;
                          }
        return 1;
}

int imas_connect(char *ip)
{
    return mdsimasConnect(ip);
}

int imas_disconnect()
{
    return mdsimasDisconnect();
}

/** Execute a shell command (possibly remotely) and
 *  returns the standard output as a string.
 *  If ip is NULL then execute locally.
 *  Return NULL if error.
 *  The returned string has to be freed by the caller.
 */
char *imas_exec(char *ip, char *command)
{
    char *stdOut = spawnCommand(command,ip);
    return stdOut;
}

EXPORT int imas_open(char *name, int shot, int run, int *retIdx)
{
    int idx = -1, status = -1;
    *retIdx = -1;

    if(!checkShotRun(shot, run)) return -1;

    status = mdsimasOpen(name, shot, run, &idx);
    if(status) return status;
    *retIdx = setMdsIdx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    return status;
}

EXPORT int imas_open_public(char *name, int shot, int run, int *retIdx)
{
    int idx = -1, status = -1;
    *retIdx = -1;

    if(!checkShotRun(shot, run)) return -1;

    status = mdsimasOpenPublic(name, shot, run, &idx);
    if(status) return status;
    *retIdx = setMdsIdx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    return status;
}

EXPORT int imas_open_env(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version)
{
  int idx = -1, status = -1;
  *retIdx = -1;

     if(!checkShotRun(shot, run)) return -1;

    status = mdsimasOpenEnv(name, shot, run, &idx, user, tokamak, version);
    if(status) return status;
    *retIdx = setMdsIdx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    logOperation("imas_open_env", 0, 0, shot, run, *retIdx, 0, 0, 0, 0, 0, 0, 0);
    return status;
}


EXPORT int imas_open_hdf5(char *name, int shot, int run, int *retIdx)
{
#ifdef HDF5
  int idx = -1, status = -1;
  *retIdx = -1;

    if(!checkShotRun(shot, run)) return -1;

    status = hdf5imasOpen(name, shot, run, &idx);
    if(status) return status;
    *retIdx = setHdf5Idx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    logOperation("imas_open_hdf5", 0, 0, shot, run, *retIdx, 0,0,0,0,0,0, 0);
    return status;
#else
    strcpy(errmsg, "HDF5 Not enabled");
    return -1;
#endif
}


EXPORT int imas_create(char *name, int shot, int run, int refShot, int refRun, int *retIdx)
{
    int idx = -1, status = -1;
    *retIdx = -1;

    if(!checkShotRun(shot, run)) return -1;

    status = mdsimasCreate(name, shot, run, refShot, refRun, &idx);
    if(status) return status;
    *retIdx = setMdsIdx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    logOperation("imas_create", 0, 0, shot, run, *retIdx, 0,0,0,0,0,0, 0);
    return status;
}
EXPORT int imas_create_env(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version)
{
    int idx = -1, status = -1;
    *retIdx = -1;

    if(!checkShotRun(shot, run)) return -1;

    status = mdsimasCreateEnv(name, shot, run, refShot, refRun, &idx, user, tokamak, version);
    if(status) return status;
    *retIdx = setMdsIdx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    logOperation("imas_create_env", 0, 0, shot, run, *retIdx, 0,0,0,0,0, 0, 0);
    return status;
}

EXPORT int imas_create_hdf5(char *name, int shot, int run, int refShot, int refRun, int *retIdx)
{
    *retIdx = -1;
#ifdef HDF5
    int idx = -1, status = -1;

     if(!checkShotRun(shot, run)) return -1;

    status = hdf5imasCreate(name, shot, run, refShot, refRun, &idx);
    if(status) return status;
    *retIdx = setHdf5Idx(shot, run, idx);
    if(*retIdx == -1)
    {
        strcpy(errmsg, "Too many open pulse files");
        return -1;
    }
    logOperation("imas_create_hdf5", 0, 0, shot, run, *retIdx, 0,0,0,0,0, 0, 0);
    return status;
#else
    strcpy(errmsg, "HDF5 Not enabled");
    return -1;
#endif
}

EXPORT int imas_close(int idx)
{
    int status = 0;
    int currIdx;

    logOperation("imas_close", 0, 0, idx, 0, 0, 0,0,0,0,0, 0, 0);
    if(isMds(idx, &currIdx))
    {
        status = mdsimasClose(currIdx, "ids", getShot(idx), getRun(idx));
    	closeExpInfo(idx);
	return status;
    }
#ifdef HDF5
    else
    {
        status = hdf5imasClose(currIdx, "ids", getShot(idx), getRun(idx));
    	closeExpInfo(idx);
	return status;
    }
#endif
    sprintf(errmsg, "Error closing database: invalid index %d\n", idx);
    return -1;
}

int deleteData(int expIdx, char *cpoPath, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("deleteData", cpoPath, path, expIdx,0,0,0,0,0,0,0,0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsDeleteData(currIdx, cpoPath, path);
#ifdef HDF5
    else
        status = hdf5DeleteData(currIdx, cpoPath, path);
#endif
    return status;
}

EXPORT int beginIdsPutSlice(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsPutSlice", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsPutSlice(currIdx, path);
#ifdef HDF5
    else
        status = hdf5beginIdsPutSlice(currIdx, path);
#endif
    return status;
}

EXPORT int endIdsPutSlice(int expIdx, char *path)
{
    int status = 0;
    int currIdx;
    logOperation("endIdsPutSlice", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsendIdsPutSlice(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsPutSlice(currIdx, path);
#endif
    return status;
}
EXPORT int beginIdsReplaceLastSlice(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsReplaceLastSlice", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsReplaceLastSlice(currIdx, path);
#ifdef HDF5
    else
        status = hdf5beginIdsReplaceLastSlice(currIdx, path);
#endif
    return status;
}

EXPORT int endIdsReplaceLastSlice(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("endIdsReplaceLastSlice", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsendIdsReplaceLastSlice(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsReplaceLastSlice(currIdx, path);
#endif
    return status;
}

EXPORT int beginIdsPut(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsPut", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsPut(currIdx, path);
#ifdef HDF5
    else
        status = hdf5beginIdsPut(currIdx, path);
#endif
    return status;
}

EXPORT int endIdsPut(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("endIdsPut", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsendIdsPut(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsPut(currIdx, path);
#endif
    return status;
}
EXPORT int beginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsPutTimed", path, 0, expIdx,samples,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsPutTimed(currIdx, path, samples, inTimes);
#ifdef HDF5
    else
        status = hdf5beginIdsPutTimed(currIdx, path, samples, inTimes);
#endif
    return status;
}


EXPORT int endIdsPutTimed(int expIdx, char *path)
{
    int status = 0;
    int currIdx;
    logOperation("endIdsPutTimed", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsendIdsPutTimed(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsPutTimed(currIdx, path);
#endif
    return status;
}

EXPORT int beginIdsPutNonTimed(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsPutNonTimed", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsPutNonTimed(currIdx, path);
#ifdef HDF5
    else
        status = hdf5beginIdsPutNonTimed(currIdx, path);
#endif
    return status;
}

EXPORT int endIdsPutNonTimed(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("endIdsPutNonTimed", path, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsendIdsPutNonTimed(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsPutNonTimed(currIdx, path);
#endif
    return status;
}

EXPORT int putString(int expIdx, char *cpoPath, char *path, char *data, int strlen)
{
    int status = 0;
    int currIdx;

    logOperation("putString", cpoPath, path, expIdx,0,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsPutString(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
      status = hdf5PutString(currIdx, cpoPath, path, data, strlen);
#endif
    return status;
}

EXPORT int  putFloat(int expIdx, char *cpoPath, char *path, float data)
{
    int status = 0;
    int currIdx;

    logOperation("putFloat", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsPutFloat(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5PutFloat(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int  putInt(int expIdx, char *cpoPath, char *path, int data)
{
    int status = 0;
    int currIdx;

    logOperation("putInt", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(data == EMPTY_INT)
	return deleteData(expIdx, cpoPath, path);
    if(isMds(expIdx, &currIdx))
        status = mdsPutInt(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5PutInt(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int  putDouble(int expIdx, char *cpoPath, char *path, double data)
{
    int status = 0;
    int currIdx;

    logOperation("putDouble", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(data == EMPTY_DOUBLE)
	return deleteData(expIdx, cpoPath, path);
    if(isMds(expIdx, &currIdx))
        status = mdsPutDouble(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5PutDouble(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int putVect1DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DInt", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DInt(currIdx, cpoPath, path, timeBasePath, data, dim, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect1DInt(currIdx, cpoPath, path, data, dim, isTimed);
#endif
    return status;
}

EXPORT int putVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, int dim, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DString", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DString(currIdx, cpoPath, path, timeBasePath, data, dim, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect1DString(currIdx, cpoPath, path, data, dim, isTimed);
#endif
    return status;
}

EXPORT int putVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DDouble", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, isTimed);
  //  printf("ual_low_level : putVect1DDouble : %s , %s , %s\n", cpoPath, path, timeBasePath);

    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DDouble(currIdx, cpoPath, path, timeBasePath, data, dim, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect1DDouble(currIdx, cpoPath, path, data, dim, isTimed);
#endif
    return status;
}

EXPORT int putVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DFloat", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DFloat(currIdx, cpoPath, path, timeBasePath, data, dim, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect1DFloat(currIdx, cpoPath, path, data, dim, isTimed);
#endif
    return status;
}

EXPORT int putVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DInt", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect2DInt(currIdx, cpoPath, path, data, dim1, dim2, isTimed);
#endif
    return status;
}


EXPORT int putVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DFloat", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DFloat(currIdx, cpoPath, path, timeBasePath,data, dim1, dim2, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect2DFloat(currIdx, cpoPath, path, data, dim1, dim2, isTimed);
#endif
    return status;
}

EXPORT int putVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DDouble", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect2DDouble(currIdx, cpoPath, path, data, dim1, dim2, isTimed);
#endif
    return status;
}

EXPORT int putVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect3DInt", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect3DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect3DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, isTimed);
#endif
    return status;
}


EXPORT int putVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect3DFloat", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect3DFloat(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect3DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, isTimed);
#endif
    return status;
}



EXPORT int putVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect3DDouble", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
    {
        status = mdsPutVect3DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, isTimed);
    }
#ifdef HDF5
    else
    {
        status = hdf5PutVect3DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, isTimed);
    }
#endif
    return status;
}

EXPORT int putVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DInt", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect4DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, isTimed);
#endif
    return status;
}

EXPORT int putVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DFloat", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DFloat(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect4DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, isTimed);
#endif
    return status;
}


EXPORT int putVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DDouble", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect4DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, isTimed);
#endif
    return status;
}
EXPORT int putVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DInt", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect5DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#endif
    return status;
}

EXPORT int putVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DFloat", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DFloat(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect5DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#endif
    return status;
}


EXPORT int putVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,
   int dim5, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DDouble", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect5DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, isTimed);
#endif
    return status;
}
////6D
EXPORT int putVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DInt", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect6DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#endif
    return status;
}

EXPORT int putVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DFloat", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DFloat(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect6DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#endif
    return status;
}


EXPORT int putVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,
   int dim5, int dim6, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DDouble", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect6DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, isTimed);
#endif
    return status;
}
/////////7D
EXPORT int putVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect7DInt", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect7DInt(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect7DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#endif
    return status;
}

EXPORT int putVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect7DFloat", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect7DFloat(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect7DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#endif
    return status;
}


EXPORT int putVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,
   int dim5, int dim6, int dim7, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putVect7DDouble", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect7DDouble(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#ifdef HDF5
    else
        status = hdf5PutVect7DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7, isTimed);
#endif
    return status;
}



////////////////PUT SLICE ROUTINES

EXPORT int  putStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putStringSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutStringSlice(currIdx, cpoPath, path, timeBasePath, data, time);
#ifdef HDF5
    else
        status = hdf5PutStringSlice(currIdx, cpoPath, path, data, time);
#endif
    return status;
}
EXPORT int  putFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float data, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putFloatSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutFloatSlice(currIdx, cpoPath, path, timeBasePath, data, time);
#ifdef HDF5
    else
        status = hdf5PutFloatSlice(currIdx, cpoPath, path, data, time);
#endif
    return status;
}

EXPORT int  putIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int data, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putIntSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, time);
    if(data == EMPTY_INT)
	return 0;
    if(isMds(expIdx, &currIdx))
        status = mdsPutIntSlice(currIdx, cpoPath, path, timeBasePath, data, time);
#ifdef HDF5
    else
        status = hdf5PutIntSlice(currIdx, cpoPath, path, data, time);
#endif
    return status;
}

EXPORT int  putDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double data, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putDoubleSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, time);
    if(data == EMPTY_DOUBLE)
	return 0;
    if(isMds(expIdx, &currIdx))
        status = mdsPutDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, time);
#ifdef HDF5
    else
        status = hdf5PutDoubleSlice(currIdx, cpoPath, path, data, time);
#endif
    return status;
}

EXPORT int putVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DIntSlice", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time);
#ifdef HDF5
    else
        status = hdf5PutVect1DIntSlice(currIdx, cpoPath, path, data, dim, time);
#endif
    return status;
}



EXPORT int putVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DDoubleSlice", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time);
#ifdef HDF5
    else
        status = hdf5PutVect1DDoubleSlice(currIdx, cpoPath, path, data, dim, time);
#endif
    return status;
}

EXPORT int putVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect1DFloatSlice", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect1DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time);
#ifdef HDF5
    else
        status = hdf5PutVect1DFloatSlice(currIdx, cpoPath, path, data, dim, time);
#endif
    return status;
}


EXPORT int putVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DIntSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time);
#ifdef HDF5
    else
        status = hdf5PutVect2DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, time);
#endif
    return status;
}


EXPORT int putVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DFloatSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time);
#ifdef HDF5
    else
        status = hdf5PutVect2DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, time);
#endif
    return status;
}

EXPORT int putVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect2DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect2DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time);
#ifdef HDF5
    else
        status = hdf5PutVect2DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, time);
#endif
    return status;
}

EXPORT int putVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect3DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect3DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time);
#ifdef HDF5
    else
        status = hdf5PutVect3DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time);
#endif
    return status;
}


EXPORT int putVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, double time)

{
    int status = 0;
    int currIdx;

    logOperation("putVect3DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect3DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time);
#ifdef HDF5
    else
        status = hdf5PutVect3DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time);
#endif
    return status;
}




EXPORT int putVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect3DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect3DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time);
#ifdef HDF5
    else
        status = hdf5PutVect3DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time);
#endif
    return status;
}

EXPORT int putVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time);
#ifdef HDF5
    else
        status = hdf5PutVect4DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time);
#endif
    return status;
}
EXPORT int putVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time);
#ifdef HDF5
    else
        status = hdf5PutVect4DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time);
#endif
    return status;
}


EXPORT int putVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect4DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect4DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time);
#ifdef HDF5
    else
        status = hdf5PutVect4DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time);
#endif
    return status;
}
EXPORT int putVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time);
#ifdef HDF5
    else
        status = hdf5PutVect5DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time);
#endif
    return status;
}
EXPORT int putVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time);
#ifdef HDF5
    else
        status = hdf5PutVect5DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time);
#endif
    return status;
}


EXPORT int putVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect5DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect5DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time);
#ifdef HDF5
    else
        status = hdf5PutVect5DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time);
#endif
    return status;
}
////////6D
EXPORT int putVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#ifdef HDF5
    else
        status = hdf5PutVect6DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#endif
    return status;
}
EXPORT int putVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#ifdef HDF5
    else
        status = hdf5PutVect6DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#endif
    return status;
}


EXPORT int putVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, double time)
{
    int status = 0;
    int currIdx;

    logOperation("putVect6DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, time);
    if(isMds(expIdx, &currIdx))
        status = mdsPutVect6DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#ifdef HDF5
    else
        status = hdf5PutVect6DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time);
#endif
    return status;
}




//////////////////REPLACE LAST SLICE ROUTINES


EXPORT int  replaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastStringSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastStringSlice(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5ReplaceLastStringSlice(currIdx, cpoPath, path, data);
#endif
    return status;
}
EXPORT int  replaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastFloatSlice(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5ReplaceLastFloatSlice(currIdx, cpoPath, path, data);
#endif
    return status;
}




EXPORT int  replaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastIntSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastIntSlice(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5ReplaceLastIntSlice(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int  replaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastDoubleSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastDoubleSlice(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5ReplaceLastDoubleSlice(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int replaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastIntSlice", cpoPath, path, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect1DIntSlice(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect1DIntSlice(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}



EXPORT int replaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect1DDoubleSlice", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect1DDoubleSlice(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect1DDoubleSlice(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}

EXPORT int replaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect1DFloatSlice", cpoPath, path, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect1DFloatSlice(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect1DFloatSlice(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}


EXPORT int replaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect1DIntSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect2DIntSlice(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect2DIntSlice(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}


EXPORT int replaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect2DFloatSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect2DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect2DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}

EXPORT int replaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect2DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect2DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect2DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}

EXPORT int replaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect3DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect3DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect3DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}


EXPORT int replaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect3DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect3DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect3DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}

EXPORT int replaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect3DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect3DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect3DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}





EXPORT int replaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect4DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect4DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect4DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}
EXPORT int replaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect4DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect4DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect4DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}


EXPORT int replaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect4DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect4DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect4DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}


///
EXPORT int replaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect5DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect5DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect5DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}
EXPORT int replaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2,
    int dim3, int dim4, int dim5)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect5DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect5DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect5DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}


EXPORT int replaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect5DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect5DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect5DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}


////6D

EXPORT int replaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect6DFloatSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect6DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect6DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}
EXPORT int replaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2,
    int dim3, int dim4, int dim5, int dim6)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect6DDoubleSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect6DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect6DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}


EXPORT int replaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int status = 0;
    int currIdx;

    logOperation("replaceLastVect6DIntSlice", cpoPath, path, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsReplaceLastVect6DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5ReplaceLastVect6DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}




/////////////GET ROUTINES////////////////////////


EXPORT int getDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetDimension(currIdx, cpoPath, path, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        status = hdf5GetDimension(currIdx, cpoPath, path, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    if (status) {
        *numDims=*dim1=*dim2=*dim3=*dim4=*dim5=*dim6=*dim7=0;
    }
    return status;
}

EXPORT int getString(int expIdx, char *cpoPath, char *path, char **data)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetString(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5GetString(currIdx, cpoPath, path, data);
#endif
    return status;
}


EXPORT int getFloat(int expIdx, char *cpoPath, char *path, float *data)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetFloat(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5GetFloat(currIdx, cpoPath, path, data);
#endif
    return status;
}


EXPORT int getInt(int expIdx, char *cpoPath, char *path, int *data)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetInt(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5GetInt(currIdx, cpoPath, path, data);
#endif
    return status;
	return status;
}

EXPORT int getDouble(int expIdx, char *cpoPath, char *path, double *data)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetDouble(currIdx, cpoPath, path, data);
#ifdef HDF5
    else
        status = hdf5GetDouble(currIdx, cpoPath, path, data);
#endif
    return status;
}

EXPORT int getVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DString(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5GetVect1DString(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}


EXPORT int getVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DInt(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5GetVect1DInt(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}


EXPORT int getVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DFloat(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5GetVect1DFloat(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}

EXPORT int getVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DDouble(currIdx, cpoPath, path, data, dim);
#ifdef HDF5
    else
        status = hdf5GetVect1DDouble(currIdx, cpoPath, path, data, dim);
#endif
    return status;
}

EXPORT int getVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DInt(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5GetVect2DInt(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}

EXPORT int getVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DFloat(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5GetVect2DFloat(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}

EXPORT int getVect2DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DDouble(currIdx, cpoPath, path, data, dim1, dim2);
#ifdef HDF5
    else
        status = hdf5GetVect2DDouble(currIdx, cpoPath, path, data, dim1, dim2);
#endif
    return status;
}

EXPORT int getVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5GetVect3DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}


EXPORT int getVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5GetVect3DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}

EXPORT int getVect3DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        status = hdf5GetVect3DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3);
#endif
    return status;
}


EXPORT int getVect4DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5GetVect4DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}

EXPORT int getVect4DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5GetVect4DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}

EXPORT int getVect4DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        status = hdf5GetVect4DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4);
#endif
    return status;
}
////
EXPORT int getVect5DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5GetVect5DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}

EXPORT int getVect5DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5GetVect5DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}

EXPORT int getVect5DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3, int *dim4,
    int *dim5)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        status = hdf5GetVect5DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return status;
}

////
EXPORT int getVect6DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5GetVect6DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}

EXPORT int getVect6DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5GetVect6DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}

EXPORT int getVect6DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3, int *dim4,
    int *dim5, int *dim6)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        status = hdf5GetVect6DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return status;
}

/////7D
EXPORT int getVect7DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect7DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        status = hdf5GetVect7DDouble(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return status;
}

EXPORT int getVect7DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect7DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6,dim7);
#ifdef HDF5
    else
        status = hdf5GetVect7DFloat(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6,dim7);
#endif
    return status;
}

EXPORT int getVect7DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3, int *dim4,
    int *dim5, int *dim6, int *dim7)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect7DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        status = hdf5GetVect7DInt(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return status;
}



EXPORT int endIdsGet(int expIdx, char *path)
{
    int status = 0;
    int currIdx;

    logOperation("endIdsGet", path, 0, expIdx, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsendIdsGet(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsGet(currIdx, path);
#endif
    return status;
}


EXPORT int beginIdsGet(int expIdx, char *path, int isTimed, int *retSamples)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsGet", path, 0, expIdx, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsGet(currIdx, path, isTimed, retSamples);
#ifdef HDF5
    else
        status = hdf5beginIdsGet(currIdx, path, isTimed, retSamples);
#endif

    return status;
}




////////////////////GET SLICE ROUTINES ////////////////////////////////////
EXPORT int endIdsGetSlice(int expIdx, char *path)
{
    int status = 0;
    int currIdx;
    logOperation("endIdsGetSlice", path, 0, expIdx, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if(isMds(expIdx, &currIdx))
        status = mdsendIdsGetSlice(currIdx, path);
#ifdef HDF5
    else
        status = hdf5endIdsGetSlice(currIdx, path);
#endif
    return status;
}

EXPORT int beginIdsGetSlice(int expIdx, char *path, double time)
{
    int status = 0;
    int currIdx;

    logOperation("beginIdsGetSlice", path, 0, expIdx, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if(isMds(expIdx, &currIdx))
        status = mdsbeginIdsGetSlice(currIdx, path, time);
#ifdef HDF5
    else
        status = hdf5beginIdsGetSlice(currIdx, path, time);
#endif
    return status;
}




EXPORT int getIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetIntSlice(currIdx, cpoPath, path, timeBasePath, data, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetIntSlice(currIdx, cpoPath, path, data, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetFloatSlice(currIdx, cpoPath, path, timeBasePath, data, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetFloatSlice(currIdx, cpoPath, path, data, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetDoubleSlice(currIdx, cpoPath, path, data, time, retTime, interpolMode);
#endif
    return status;
}
EXPORT int getStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetStringSlice(currIdx, cpoPath, path, timeBasePath, data, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetStringSlice(currIdx, cpoPath, path, data, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect1DIntSlice(currIdx, cpoPath, path, data, dim, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect1DFloatSlice(currIdx, cpoPath, path, data, dim, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect1DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect1DDoubleSlice(currIdx, cpoPath, path, data, dim, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect2DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect2DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect2DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect2DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect3DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect3DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time, retTime, interpolMode);
#endif
    return status;
}



EXPORT int getVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect3DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect3DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect4DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect4DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#endif
    return status;
}



EXPORT int getVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect4DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect4DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, time, retTime, interpolMode);
#endif
    return status;
}


EXPORT int getVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect5DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect5DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#endif
    return status;
}



EXPORT int getVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect5DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect5DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, time, retTime, interpolMode);
#endif
    return status;
}

//////6D Slice

EXPORT int getVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DIntSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect6DIntSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#endif
    return status;
}

EXPORT int getVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DFloatSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect6DFloatSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#endif
    return status;
}



EXPORT int getVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
    int status = 0;
    int currIdx;

    if(isMds(expIdx, &currIdx))
        status = mdsGetVect6DDoubleSlice(currIdx, cpoPath, path, timeBasePath, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#ifdef HDF5
    else
        status = hdf5GetVect6DDoubleSlice(currIdx, cpoPath, path, data, dim1, dim2, dim3, dim4, dim5, dim6, time, retTime, interpolMode);
#endif
    return status;
}

//Initialize room for a generic array of structures
void *beginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed)
{
    int currIdx;
    logOperation("beginObject", (char *)relPath, 0, expIdx,0,0,0,0,0,0,0,0, isTimed);
    if(isMds(expIdx, &currIdx))
        return mdsBeginObject();
#ifdef HDF5
    else
        return hdf5BeginObject(currIdx,obj,index,relPath,isTimed);
#endif
    return NULL;
}

//Releases memory for array of structures
void releaseObject(int expIdx, void *obj)
{
    int currIdx;
    logOperation("releaseObject", 0, 0, expIdx,0,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        mdsReleaseObject(obj);
#ifdef HDF5
    else
        hdf5ReleaseObject(obj);
#endif

}


//Writes an array of objects array of structures
//Flag isTImes specifies whether the array refers to time-dependent field (in this case the dimension refers to time)
int putObject(int expIdx, char *cpoPath, char *path, void *obj, int isTimed)
{
    int status = 0;
    int currIdx;

    logOperation("putObject", cpoPath, path, expIdx, 0,0,0,0,0,0,0,0,isTimed);
    if(isMds(expIdx, &currIdx))
        status = mdsPutObject(currIdx, cpoPath, path, obj, isTimed);
#ifdef HDF5
    else
        status = hdf5PutObject(currIdx, cpoPath, path, obj, isTimed);
#endif
    if(status)
         logOperation("putObject FAILED", cpoPath, path, expIdx, 0,0,0,0,0,0,0,0,isTimed);
    else
         logOperation("putObject SUCCESS", cpoPath, path, expIdx, 0,0,0,0,0,0,0,0,isTimed);
    return status;
}

//Add elements to the structure array. Note: returns the new pointer to the object, possibly chjanged (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structues
void *putIntInObject(int expIdx, void *obj, char *path, int idx, int data)
{
    int currIdx;

    logOperation("putIntInObject", path, 0, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutIntInObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5PutIntInObject(obj, path, idx, data);
#endif
    return NULL;
}

void *putStringInObject(int expIdx, void *obj, char *path, int idx, char *data)
{
    int currIdx;

    logOperation("putStringInObject", path, 0, expIdx,0,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutStringInObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5PutStringInObject(obj, path, idx, data);
#endif
    return NULL;
}

void *putFloatInObject(int expIdx, void *obj, char *path, int idx, float data)
{
    int currIdx;

    logOperation("putFloatInObject", path, 0, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutFloatInObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5PutFloatInObject(obj, path, idx, data);
#endif
    return NULL;
}

void *putDoubleInObject(int expIdx, void *obj, char *path, int idx, double data)
{
    int currIdx;

    logOperation("putDoubleInObject", path, 0, expIdx,0,0,0,0,0,0,0,&data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutDoubleInObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5PutDoubleInObject(obj, path, idx, data);
#endif
    return NULL;
}

void *putVect1DStringInObject(int expIdx, void *obj, char *path, int idx, char **data, int dim)
{
    int currIdx;

    logOperation("putVect1DStringInObject", path, 0, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect1DStringInObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5PutVect1DStringInObject(obj, path, idx, data, dim);
#endif
    return NULL;
}

void *putVect1DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim)
{
    int currIdx;

    logOperation("putVect1DIntInObject", path, 0, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect1DIntInObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5PutVect1DIntInObject(obj, path, idx, data, dim);
#endif
    return NULL;
}

void *putVect1DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim)
{
    int currIdx;

    logOperation("putVect1DFloatInObject", path, 0, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect1DFloatInObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5PutVect1DFloatInObject(obj, path, idx, data, dim);
#endif
    return NULL;
}

void *putVect1DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim)
{
    int currIdx;

    logOperation("putVect1DDoubleInObject", path, 0, expIdx,dim,0,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect1DDoubleInObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5PutVect1DDoubleInObject(obj, path, idx, data, dim);
#endif
    return NULL;
}

void *putVect2DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2)
{
    int currIdx;

    logOperation("putVect2DIntInObject", path, 0, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect2DIntInObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5PutVect2DIntInObject(obj, path, idx, data, dim1, dim2);
#endif
    return NULL;
}

void *putVect2DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2)
{
    int currIdx;

    logOperation("putVect2DFloatInObject", path, 0, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect2DFloatInObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5PutVect2DFloatInObject(obj, path, idx, data, dim1, dim2);
#endif
    return NULL;
}

void *putVect2DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2)
{
    int currIdx;

    logOperation("putVect2DDoubleInObject", path, 0, expIdx,dim1,dim2,0,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect2DDoubleInObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5PutVect2DDoubleInObject(obj, path, idx, data, dim1, dim2);
#endif
    return NULL;
}

void *putVect3DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3)
{
    int currIdx;

    logOperation("putVect3DIntInObject", path, 0, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect3DIntInObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5PutVect3DIntInObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return NULL;
}

void *putVect3DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3)
{
    int currIdx;

    logOperation("putVect3DFloatInObject", path, 0, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect3DFloatInObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5PutVect3DFloatInObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return NULL;
}

void *putVect3DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3)
{
    int currIdx;

    logOperation("putVect3DDoubleInObject", path, 0, expIdx,dim1,dim2,dim3,0,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect3DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5PutVect3DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return NULL;
}

void *putVect4DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4)
{
     int currIdx;

    logOperation("putVect4DIntInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
   if(isMds(expIdx, &currIdx))
       return mdsPutVect4DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5PutVect4DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return NULL;
}

void *putVect4DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int currIdx;

    logOperation("putVect4DFloatInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect4DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5PutVect4DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return NULL;
}

void *putVect4DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int currIdx;

    logOperation("putVect4DDoubleInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,0,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect4DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5PutVect4DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return NULL;
}

void *putVect5DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int currIdx;

    logOperation("putVect5DIntInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect5DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5PutVect5DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return NULL;
}

void *putVect5DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5)
{
    int currIdx;

    logOperation("putVect5DFloatInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect5DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5PutVect5DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return NULL;
}

void *putVect5DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5)
{
     int currIdx;

    logOperation("putVect5DDoubleInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,0,0,data, 0);
   if(isMds(expIdx, &currIdx))
       return mdsPutVect5DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5PutVect5DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return NULL;
}

void *putVect6DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int currIdx;

    logOperation("putVect6DIntInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect6DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5PutVect6DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return NULL;
}

void *putVect6DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6)
{
    int currIdx;

    logOperation("putVect6DFloatInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect6DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5PutVect6DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return NULL;
}

void *putVect6DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6)
{
     int currIdx;
    logOperation("putVect6DDoubleInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,0,data, 0);
     if(isMds(expIdx, &currIdx))
       return mdsPutVect6DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5PutVect6DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return NULL;
}

void *putVect7DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int currIdx;

    logOperation("putVect7DIntInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect7DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5PutVect7DIntInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return NULL;
}

void *putVect7DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7)
{
    int currIdx;

    logOperation("putVect7DFloatInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect7DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5PutVect7DFloatInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return NULL;
}

void *putVect7DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7)
{
    int currIdx;

    logOperation("putVect7DDoubleInObject", path, 0, expIdx,dim1,dim2,dim3,dim4,dim5,dim6,dim7,data, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutVect7DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5PutVect7DDoubleInObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return NULL;
}

void *putObjectInObject(int expIdx, void *obj, char *path, int idx, void *dataObj)
{
    int currIdx;

    logOperation("putObjectInObject", path, 0, expIdx,idx,0,0,0,0,0,0,0, 0);
    if(isMds(expIdx, &currIdx))
        return mdsPutObjectInObject(obj, path, idx, dataObj);
#ifdef HDF5
    else
        return hdf5PutObjectInObject(obj, path, idx, dataObj);
#endif
    return NULL;
}

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int getObjectDim(int expIdx, void *obj)
{
    int currIdx;

    if(isMds(expIdx, &currIdx))
       return mdsGetObjectDim(obj);
#ifdef HDF5
    else
        return hdf5GetObjectDim(obj);
#endif
    return -1;
}

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int getObject(int expIdx, char *path, char *cpoPath, void **obj, int isTimed)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetObject(currIdx, path, cpoPath, obj, isTimed);
#ifdef HDF5
    else
        return hdf5GetObject(currIdx, path, cpoPath, obj, isTimed);
#endif
    return -1;
}

//Retrieves components from array of strictures. Returned status indicates success (0) or error(!= 0)
int getStringFromObject(int expIdx, void *obj, char *path, int idx, char **data)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetStringFromObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5GetStringFromObject(obj, path, idx, data);
#endif
    return -1;
}

int getFloatFromObject(int expIdx, void *obj, char *path, int idx, float *data)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetFloatFromObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5GetFloatFromObject(obj, path, idx, data);
#endif
    return -1;
}

int getIntFromObject(int expIdx, void *obj, char *path, int idx, int *data)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetIntFromObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5GetIntFromObject(obj, path, idx, data);
#endif
    return -1;
}

int getDoubleFromObject(int expIdx, void *obj, char *path, int idx, double *data)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetDoubleFromObject(obj, path, idx, data);
#ifdef HDF5
    else
        return hdf5GetDoubleFromObject(obj, path, idx, data);
#endif
    return -1;
}

int getVect1DStringFromObject(int expIdx, void *obj, char *path, int idx, char  ***data, int *dim)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect1DStringFromObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5GetVect1DStringFromObject(obj, path, idx, data, dim);
#endif
    return -1;
}

int getVect1DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect1DIntFromObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5GetVect1DIntFromObject(obj, path, idx, data, dim);
#endif
    return -1;
}

int getVect1DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect1DFloatFromObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5GetVect1DFloatFromObject(obj, path, idx, data, dim);
#endif
    return -1;
}

int getVect1DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect1DDoubleFromObject(obj, path, idx, data, dim);
#ifdef HDF5
    else
        return hdf5GetVect1DDoubleFromObject(obj, path, idx, data, dim);
#endif
    return -1;
}

int getVect2DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect2DIntFromObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5GetVect2DIntFromObject(obj, path, idx, data, dim1, dim2);
#endif
    return -1;
}

int getVect2DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect2DFloatFromObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5GetVect2DFloatFromObject(obj, path, idx, data, dim1, dim2);
#endif
    return -1;
}

int getVect2DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect2DDoubleFromObject(obj, path, idx, data, dim1, dim2);
#ifdef HDF5
    else
        return hdf5GetVect2DDoubleFromObject(obj, path, idx, data, dim1, dim2);
#endif
    return -1;
}

int getVect3DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect3DIntFromObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5GetVect3DIntFromObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return -1;
}

int getVect3DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect3DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5GetVect3DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return -1;
}

int getVect3DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3)
{
     int currIdx;
   if(isMds(expIdx, &currIdx))
       return mdsGetVect3DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3);
#ifdef HDF5
    else
        return hdf5GetVect3DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3);
#endif
    return -1;
}

int getVect4DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect4DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5GetVect4DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return -1;
}

int getVect4DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect4DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5GetVect4DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return -1;
}

int getVect4DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect4DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#ifdef HDF5
    else
        return hdf5GetVect4DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4);
#endif
    return -1;
}

int getVect5DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect5DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5GetVect5DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return -1;
}

int getVect5DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect5DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5GetVect5DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return -1;
}

int getVect5DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect5DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#ifdef HDF5
    else
        return hdf5GetVect5DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5);
#endif
    return -1;
}

int getVect6DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect6DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5GetVect6DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return -1;
}

int getVect6DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect6DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5GetVect6DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return -1;
}

int getVect6DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect6DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#ifdef HDF5
    else
        return hdf5GetVect6DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6);
#endif
    return -1;
}

int getVect7DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
     int currIdx;
   if(isMds(expIdx, &currIdx))
       return mdsGetVect7DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5GetVect7DIntFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return -1;
}

int getVect7DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect7DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5GetVect7DFloatFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return -1;
}

int getVect7DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetVect7DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#ifdef HDF5
    else
        return hdf5GetVect7DDoubleFromObject(obj, path, idx, data, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
#endif
    return -1;
}

int getObjectFromObject(int expIdx, void *obj, char *path, int idx, void **dataObj)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetObjectFromObject(obj, path, idx, dataObj);
#ifdef HDF5
    else
        return hdf5GetObjectFromObject(obj, path, idx, dataObj);
#endif
    return -1;
}

int getDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int currIdx;
    int status;
    if(isMds(expIdx, &currIdx))
      status = mdsGetDimensionFromObject(currIdx, obj, path, idx, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7 );
#ifdef HDF5
    else
       status = hdf5GetDimensionFromObject(currIdx, obj, path, idx, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7 );
#endif
    if (status) {
        *numDims=*dim1=*dim2=*dim3=*dim4=*dim5=*dim6=*dim7=0;
    }
    return status;
}

//Array of structures Slice Management
int getObjectSlice(int expIdx, char *cpoPath, char *path, double time, void **obj)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
       return mdsGetObjectSlice(currIdx, cpoPath, path, time, obj);
#ifdef HDF5
    else
        return hdf5GetObjectSlice(currIdx, cpoPath, path, time, obj);
#endif
    return -1;
}

int putObjectSlice(int expIdx, char *cpoPath, char *path,  double time, void *obj)
{
    int currIdx;
    logOperation("putObjectSlice", cpoPath, path, expIdx, 0,0,0,0,0,0,0,0,0);
    if(isMds(expIdx, &currIdx))
       return mdsPutObjectSlice(currIdx, cpoPath, path, time, obj);
#ifdef HDF5
    else
        return hdf5PutObjectSlice(currIdx, cpoPath, path, time, obj);
#endif
    return -1;
}

int replaceLastObjectSlice(int expIdx, char *cpoPath, char *path, void *obj)
{
    int currIdx;
    if(isMds(expIdx, &currIdx))
    return mdsReplaceLastObjectSlice(currIdx, cpoPath, path, obj);
#ifdef HDF5
    else
        return hdf5ReplaceLastObjectSlice(currIdx, cpoPath, path, obj);
#endif
    return -1;
}

//CPO Copy
int ual_copy_cpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur)
{
    return mdsCopyCpo(fromIdx, toIdx, cpoName, fromCpoOccur, toCpoOccur);
}

int ual_copy_cpo_env(char *tokamakFrom, char *versionFrom, char *userFrom, int shotFrom, int runFrom, int occurrenceFrom,
    char *tokamakTo, char *versionTo, char *userTo, int shotTo, int runTo, int occurrenceTo, char *cpoName)
{
    int fromIdx, toIdx, status;
    status = imas_open_env("ids", shotFrom, runFrom, &fromIdx, userFrom, tokamakFrom, versionFrom);
    if(status) return status;
    status = imas_open_env("ids", shotTo, runTo, &toIdx, userTo, tokamakTo, versionTo);
    if(status) {
        imas_close(fromIdx);
        return status;
    }
    status = ual_copy_cpo(fromIdx, toIdx, cpoName, occurrenceFrom, occurrenceTo);
    imas_close(fromIdx);
    imas_close(toIdx);
    return status;
}

//Error Management support routine
int makeErrorStatus(int isCritical, int type, int intStatus)
{
    return (isCritical << 24) | (type << 16) | (intStatus & 0x0000FFFF);
}
int isCriticalError(int status)
{
    return ((status & 0xFF000000) != 0);
}
int getErrorType(int status)
{
     return (status & 0x00FF0000) >> 16;
}

