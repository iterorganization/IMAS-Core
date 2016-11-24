#define INTERPOLATION 3
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

#define DIRECTIVELENGTH		512

static int findIMASType(char *typeName);
static int findIMASIDAMType(int type);
static char *convertIdam2StringType(int type);

static int putData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int isTimed, void *data);
static int putDataX(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int dataOperation, void *data, double time);
static int putDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data, double time);
static int replaceLastDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data);

static int getData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, char **data);
//static int getDataSlices(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int dataIdx, int numSlices, char **data);
static int getDataSlices(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, char **data, double time, double *retTime, int interpolMode);


//int deleteData(int expIdx, char *cpoPath, char *path);
//int getUniqueRun(int shot);


/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

#define NON_TIMED    0
#define TIMED       1
#define TIMED_CLEAR 2

static void *putDataSliceInObject(void *obj, char *path, int index, int type, int nDims, int *dims, void *data);

// static int putDataSliceInObject(void *obj, char *path, int index, int type, int nDims, int *dims, void *data);

