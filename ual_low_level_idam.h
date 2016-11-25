#define INTERPOLATION   3
#define CLOSEST_SAMPLE  1
#define PREVIOUS_SAMPLE 2

extern int idamimasConnect(char *ip);
extern char *spawnCommand(char *command, char *ipAddress);
extern int idamimasCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx);
extern int idamimasCreateEnv(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);
extern int idamimasOpen(char *name, int shot, int run, int *retIdx);
extern int idamimasOpenEnv(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
extern int idamimasClose(int idx, char *name, int shot, int run);
extern void disableMemCache();
extern void discardMem(int expIdx, char *cpoPath, char *path);
extern void discardOldMem(int expIdx, char *cpoPath, char *path, double time);
extern void flush(int expIdx, char *cpoPath, char *path);
extern char *idamLastErrmsg();

extern int idamPutString(int expIdx, char *cpoPath, char *path, char *data, int strlen);
extern int idamPutInt(int expIdx, char *cpoPath, char *path, int data);
extern int idamPutFloat(int expIdx, char *cpoPath, char *path, float data);
extern int idamPutDouble(int expIdx, char *cpoPath, char *path, double data);
extern int idamPutVect1DString(int expIdx, char *cpoPath, char *path, char **data, int dim, int isTimed);
extern int idamPutVect1DInt(int expIdx, char *cpoPath, char *path, int *data, int dim, int isTimed);
extern int idamPutVect1DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim, int isTimed);
extern int idamPutVect1DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim, int isTimed);
extern int idamPutVect2DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int isTimed);
extern int idamPutVect2DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int isTimed);
extern int idamPutVect2DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int isTimed);
extern int idamPutVect3DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int isTimed);
extern int idamPutVect3DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int isTimed);
extern int idamPutVect3DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int isTimed);
extern int idamPutVect4DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int idamPutVect4DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int idamPutVect4DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int idamPutVect5DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int idamPutVect5DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int idamPutVect5DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int idamPutVect6DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int idamPutVect6DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int idamPutVect6DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int idamPutVect7DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int idamPutVect7DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int idamPutVect7DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int idamPutIntSlice(int expIdx, char *cpoPath, char *path, int data, double time);
extern int idamPutFloatSlice(int expIdx, char *cpoPath, char *path, float data, double time);
extern int idamPutDoubleSlice(int expIdx, char *cpoPath, char *path, double data, double time);
extern int idamPutStringSlice(int expIdx, char *cpoPath, char *path, char *data, double time);
extern int idamPutVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim, double time);
extern int idamPutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim, double time);
extern int idamPutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim, double time);
extern int idamPutVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, double time);
extern int idamPutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, double time);
extern int idamPutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, double time);
extern int idamPutVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, double time);
extern int idamPutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, double time);
extern int idamPutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, double time);
extern int idamPutVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int idamPutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int idamPutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int idamPutVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int idamPutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int idamPutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int idamPutVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int idamPutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int idamPutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);

extern int idamReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data);
extern int idamReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data);
extern int idamReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data);
extern int idamReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data);
extern int idamReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim);
extern int idamReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim);
extern int idamReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim);
extern int idamReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2);
extern int idamReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2);
extern int idamReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2);
extern int idamReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3);
extern int idamReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3);
extern int idamReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3);
extern int idamReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4);
extern int idamReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4);
extern int idamReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4);
extern int idamReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int idamReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int idamReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int idamReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int idamReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int idamReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern int idamGetString(int expIdx, char *cpoPath, char *path, char **data);
extern int idamGetFloat(int expIdx, char *cpoPath, char *path, float *data);
extern int idamGetInt(int expIdx, char *cpoPath, char *path, int *data);
extern int idamGetDouble(int expIdx, char *cpoPath, char *path, double *data);
extern int idamGetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim);
extern int idamGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim);
extern int idamGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim);
extern int idamGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim);
extern int idamGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2);
extern int idamGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2);
extern int idamGetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2);
extern int idamGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3);
extern int idamGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3);
extern int idamGetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3);
extern int idamGetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int idamGetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int idamGetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int idamGetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int idamGetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int idamGetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int idamGetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int idamGetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int idamGetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int idamGetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int idamGetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int idamGetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);


extern int idamGetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode);
extern int idamGetFloatSlice(int expIdx, char *cpoPath, char *path, float *data, double time, double *retTime, int interpolMode);
extern int idamGetIntSlice(int expIdx, char *cpoPath, char *path, int *data, double time, double *retTime, int interpolMode);
extern int idamGetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode);
extern int idamGetDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, double time, double *retTime, int interpolMode);
extern int idamGetVect1DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim, double time, double *retTime, int interpolMode);
extern int idamGetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim, double time, double *retTime, int interpolMode);
extern int idamGetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim, double time, double *retTime, int interpolMode);
extern int idamGetVect2DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int idamGetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int idamGetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int idamGetVect3DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int idamGetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int idamGetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int idamGetVect4DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int idamGetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int idamGetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int idamGetVect5DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int idamGetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int idamGetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int idamGetVect6DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int idamGetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int idamGetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);

extern int idamDeleteData(int expIdx, char *cpoPath, char *path);
extern int getUniqueRun(int shot);

extern int idambeginIdsGet(int expIdx, char *path, int isTimed, int *retSamples);
extern int idamendIdsGet(int expIdx, char *path);
extern int idambeginIdsGetResampled(int expIdx, char *path, double start, double end, double delta, int *retSamples);
extern int idamendIdsGetResampled(int expIdx, char *path);
extern int idambeginIdsGetSlice(int expIdx, char *path, double time);
extern int idamendIdsGetSlice(int expIdx, char *path);
extern int idambeginIdsPut(int expIdx, char *path);
extern int idamendIdsPut(int expIdx, char *path);
extern int idambeginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes);
extern int idamendIdsPutTimed(int expIdx, char *path);
extern int idambeginIdsPutNonTimed(int expIdx, char *path);
extern int idamendIdsPutNonTimed(int expIdx, char *path);
extern int idambeginIdsPutSlice(int expIdx, char *path);
extern int idamendIdsPutSlice(int expIdx, char *path);
extern int idambeginIdsReplaceLastSlice(int expIdx, char *path);
extern int idamendIdsReplaceLastSlice(int expIdx, char *path);

void idamDeleteAllFields(int expIdx, char *cpoPath);
int idamIsSliced(int expIdx, char *cpoPath, char *path);

//------------------ Array of structures stuff -----------------

//Initialize room for a generic array of structures
void *idamBeginObject();

//Releases memory for array of structures
void idamReleaseObject(void *obj);

//Writes an array of objects array of structures
//Flag isTimed specifies whether the array refers to time-dependent field (in this case the dimension refers to time)
int idamPutObject(int expIdx, char *cpoPath, char *idamPath, void *obj, int isTimed);

//Add elements to the structure array. Note: returns the new pointer to the object, possibly chjanged (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structues
void *idamPutIntInObject(void *obj, char *idamPath, int idx, int data);
void *idamPutStringInObject(void *obj, char *idamPath, int idx, char *data);
void *idamPutFloatInObject(void *obj, char *idamPath, int idx, float data);
void *idamPutDoubleInObject(void *obj, char *idamPath, int idx, double data);
void *idamPutVect1DStringInObject(void *obj, char *idamPath, int idx, char **data, int dim);
void *idamPutVect1DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim);
void *idamPutVect1DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim);
void *idamPutVect1DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim);
void *idamPutVect2DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2);
void *idamPutVect2DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2);
void *idamPutVect2DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2);
void *idamPutVect3DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3);
void *idamPutVect3DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3);
void *idamPutVect3DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3);
void *idamPutVect4DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4);
void *idamPutVect4DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4);
void *idamPutVect4DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4);
void *idamPutVect5DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *idamPutVect5DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *idamPutVect5DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *idamPutVect6DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *idamPutVect6DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *idamPutVect6DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *idamPutVect7DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *idamPutVect7DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *idamPutVect7DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *idamPutObjectInObject(void *obj, char *idamPath, int idx, void *dataObj);

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int idamGetObjectDim(void *obj);

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int idamGetObject(int expIdx, char *idamPath, char *cpoPath, void **obj, int isTimed);

//Retrieves components from array of structures. Returned status indicates success (0) or error(!= 0)
int idamGetStringFromObject(void *obj, char *idamPath, int idx, char **data);
int idamGetFloatFromObject(void *obj, char *idamPath, int idx, float *data);
int idamGetIntFromObject(void *obj, char *idamPath, int idx, int *data);
int idamGetDoubleFromObject(void *obj, char *idamPath, int idx, double *data);
int idamGetVect1DStringFromObject(void *obj, char *idamPath, int idx, char  ***data, int *dim);
int idamGetVect1DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim);
int idamGetVect1DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim);
int idamGetVect1DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim);
int idamGetVect2DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2);
int idamGetVect2DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2);
int idamGetVect2DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2);
int idamGetVect3DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3);
int idamGetVect3DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3);
int idamGetVect3DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3);
int idamGetVect4DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int idamGetVect4DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
int idamGetVect4DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int idamGetVect5DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int idamGetVect5DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int idamGetVect5DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int idamGetVect6DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int idamGetVect6DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int idamGetVect6DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int idamGetVect7DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int idamGetVect7DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int idamGetVect7DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int idamGetObjectFromObject(void *obj, char *idamPath, int idx, void **dataObj);

int idamGetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

//Array of structures Slice Management
int idamGetObjectSlice(int expIdx, char *idamPath, char *cpoPath, double time, void **obj);
int idamPutObjectSlice(int expIdx, char *idamPath, char *cpoPath, double time, void *obj);
int idamReplaceLastObjectSlice(int expIdx, char *cpoPath, char *idamPath, void *obj);

//cpo copy
int idamCopyCpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur);
