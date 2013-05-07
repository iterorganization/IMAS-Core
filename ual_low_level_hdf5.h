#define INTERPOLATION 3
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

extern int hdf5EuitmCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx);
extern int hdf5EuitmOpen(char *name, int shot, int run, int *retIdx);
extern int hdf5EuitmClose(int idx, char *name, int shot, int run);

extern int hdf5PutString(int expIdx, char *cpoPath, char *path, char *data, int strlen);
extern int hdf5PutInt(int expIdx, char *cpoPath, char *path, int data);
extern int hdf5PutFloat(int expIdx, char *cpoPath, char *path, float data);
extern int hdf5PutDouble(int expIdx, char *cpoPath, char *path, double data);
extern int hdf5PutVect1DString(int expIdx, char *cpoPath, char *path, char **data, int dim, int isTimed);
extern int hdf5PutVect1DInt(int expIdx, char *cpoPath, char *path, int *data, int dim, int isTimed);
extern int hdf5PutVect1DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim, int isTimed);
extern int hdf5PutVect1DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim, int isTimed);
extern int hdf5PutVect2DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int isTimed);
extern int hdf5PutVect2DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int isTimed);
extern int hdf5PutVect2DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int isTimed);
extern int hdf5PutVect3DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int isTimed);
extern int hdf5PutVect3DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int isTimed);
extern int hdf5PutVect3DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int isTimed);
extern int hdf5PutVect4DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int hdf5PutVect4DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int hdf5PutVect4DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int hdf5PutVect5DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int hdf5PutVect5DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int hdf5PutVect5DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int hdf5PutVect6DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int hdf5PutVect6DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int hdf5PutVect6DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int hdf5PutVect7DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int hdf5PutVect7DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int hdf5PutVect7DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int hdf5PutIntSlice(int expIdx, char *cpoPath, char *path, int data, double time);
extern int hdf5PutFloatSlice(int expIdx, char *cpoPath, char *path, float data, double time);
extern int hdf5PutDoubleSlice(int expIdx, char *cpoPath, char *path, double data, double time);
extern int hdf5PutStringSlice(int expIdx, char *cpoPath, char *path, char *data, double time);
extern int hdf5PutVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim, double time);
extern int hdf5PutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim, double time);
extern int hdf5PutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim, double time);
extern int hdf5PutVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, double time);
extern int hdf5PutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, double time);
extern int hdf5PutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, double time);
extern int hdf5PutVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, double time);
extern int hdf5PutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, double time);
extern int hdf5PutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, double time);
extern int hdf5PutVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int hdf5PutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int hdf5PutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int hdf5PutVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int hdf5PutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int hdf5PutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int hdf5PutVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int hdf5PutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int hdf5PutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);

extern int hdf5ReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data);
extern int hdf5ReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data);
extern int hdf5ReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data);
extern int hdf5ReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data);
extern int hdf5ReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim);
extern int hdf5ReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim);
extern int hdf5ReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim);
extern int hdf5ReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2);
extern int hdf5ReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2);
extern int hdf5ReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2);
extern int hdf5ReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3);
extern int hdf5ReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3);
extern int hdf5ReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3);
extern int hdf5ReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4);
extern int hdf5ReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4);
extern int hdf5ReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4);
extern int hdf5ReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int hdf5ReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int hdf5ReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int hdf5ReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int hdf5ReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int hdf5ReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern int hdf5GetString(int expIdx, char *cpoPath, char *path, char **data);
extern int hdf5GetFloat(int expIdx, char *cpoPath, char *path, float *data);
extern int hdf5GetInt(int expIdx, char *cpoPath, char *path, int *data);
extern int hdf5GetDouble(int expIdx, char *cpoPath, char *path, double *data);
extern int hdf5GetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim);
extern int hdf5GetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim);
extern int hdf5GetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim);
extern int hdf5GetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim);
extern int hdf5GetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2);
extern int hdf5GetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2);
extern int hdf5GetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2);
extern int hdf5GetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3);
extern int hdf5GetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3);
extern int hdf5GetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3);
extern int hdf5GetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int hdf5GetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int hdf5GetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int hdf5GetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int hdf5GetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int hdf5GetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int hdf5GetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int hdf5GetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int hdf5GetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int hdf5GetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int hdf5GetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int hdf5GetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

extern int hdf5GetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode);
extern int hdf5GetFloatSlice(int expIdx, char *cpoPath, char *path, float *data, double time, double *retTime, int interpolMode);
extern int hdf5GetIntSlice(int expIdx, char *cpoPath, char *path, int *data, double time, double *retTime, int interpolMode);
extern int hdf5GetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode);
extern int hdf5GetDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, double time, double *retTime, int interpolMode);
extern int hdf5GetVect1DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim, double time, double *retTime, int interpolMode);
extern int hdf5GetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim, double time, double *retTime, int interpolMode);
extern int hdf5GetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim, double time, double *retTime, int interpolMode);
extern int hdf5GetVect2DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int hdf5GetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int hdf5GetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int hdf5GetVect3DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int hdf5GetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int hdf5GetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int hdf5GetVect4DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int hdf5GetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int hdf5GetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int hdf5GetVect5DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int hdf5GetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int hdf5GetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int hdf5GetVect6DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int hdf5GetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int hdf5GetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);

int deleteData(int expIdx, char *cpoPath, char *path);
int getUniqueRun(int shot);

int hdf5BeginCPOGet(int expIdx, char *path, int isTimed, int *retSamples);
int hdf5EndCPOGet(int expIdx, char *path);
int hdf5BeginCPOGetResampled(int expIdx, char *path, double start, double end, double delta, int *retSamples);
int hdf5EndCPOGetResampled(int expIdx, char *path);
int hdf5BeginCPOGetSlice(int expIdx, char *path, double time);
int hdf5EndCPOGetSlice(int expIdx, char *path);
int hdf5BeginCPOPut(int expIdx, char *path);
int hdf5EndCPOPut(int expIdx, char *path);
int hdf5BeginCPOPutTimed(int expIdx, char *path, int samples, double *inTimes);
int hdf5EndCPOPutTimed(int expIdx, char *path);
int hdf5BeginCPOPutNonTimed(int expIdx, char *path);
int hdf5EndCPOPutNonTimed(int expIdx, char *path);
int hdf5BeginCPOPutNonTimed(int expIdx, char *path);
int hdf5EndCPOPutNonTimed(int expIdx, char *path);
int hdf5BeginCPOPutSlice(int expIdx, char *path);
int hdf5EndCPOPutSlice(int expIdx, char *path);
int hdf5BeginCPOReplaceLastSlice(int expIdx, char *path);
int hdf5EndCPOReplaceLastSlice(int expIdx, char *path);

/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

#define NON_TIMED    0
#define TIMED       1
#define TIMED_CLEAR 2

void *hdf5BeginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed);
void hdf5ReleaseObject(void *obj);
int hdf5PutObject(int currIdx, char *cpoPath, char *path, void *obj, int isTimed);
void *hdf5PutIntInObject(void *obj, char *path, int idx, int data);
void *hdf5PutFloatInObject(void *obj, char *path, int idx, float data);
void *hdf5PutDoubleInObject(void *obj, char *path, int idx, double data);
void *hdf5PutStringInObject(void *obj, char *path, int idx, char *data);
void *hdf5PutVect1DStringInObject(void *obj, char *path, int idx, char **data, int dim);
void *hdf5PutVect1DIntInObject(void *obj, char *path, int idx, int *data, int dim);
void *hdf5PutVect1DFloatInObject(void *obj, char *path, int idx, float *data, int dim);
void *hdf5PutVect1DDoubleInObject(void *obj, char *path, int idx, double *data, int dim);
void *hdf5PutVect2DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2);
void *hdf5PutVect2DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2);
void *hdf5PutVect2DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2);
void *hdf5PutVect3DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3);
void *hdf5PutVect3DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3);
void *hdf5PutVect3DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3);
void *hdf5PutVect4DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4);
void *hdf5PutVect4DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4);
void *hdf5PutVect4DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4);
void *hdf5PutVect5DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *hdf5PutVect5DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *hdf5PutVect5DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *hdf5PutVect6DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *hdf5PutVect6DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *hdf5PutVect6DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *hdf5PutVect7DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *hdf5PutVect7DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *hdf5PutVect7DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *hdf5PutObjectInObject(void *obj, char *path, int idx, void *data);

int hdf5GetObjectDim(void *obj);
int hdf5GetObject(int expIdx, char *hdf5Path, char *cpoPath, void **obj, int isTimed);
int hdf5GetStringFromObject(void *obj, char *hdf5Path, int idx, char **data);
int hdf5GetFloatFromObject(void *obj, char *hdf5Path, int idx, float *data);
int hdf5GetIntFromObject(void *obj, char *hdf5Path, int idx, int *data);
int hdf5GetDoubleFromObject(void *obj, char *hdf5Path, int idx, double *data);
int hdf5GetVect1DStringFromObject(void *obj, char *hdf5Path, int idx, char  ***data, int *dim);
int hdf5GetVect1DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim);
int hdf5GetVect1DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim);
int hdf5GetVect1DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim);
int hdf5GetVect2DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2);
int hdf5GetVect2DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2);
int hdf5GetVect2DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2);
int hdf5GetVect3DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3);
int hdf5GetVect3DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3);
int hdf5GetVect3DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3);
int hdf5GetVect4DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int hdf5GetVect4DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
int hdf5GetVect4DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int hdf5GetVect5DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int hdf5GetVect5DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int hdf5GetVect5DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int hdf5GetVect6DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int hdf5GetVect6DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int hdf5GetVect6DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int hdf5GetVect7DIntFromObject(void *obj, char *hdf5Path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int hdf5GetVect7DFloatFromObject(void *obj, char *hdf5Path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int hdf5GetVect7DDoubleFromObject(void *obj, char *hdf5Path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int hdf5GetObjectFromObject(void *obj, char *hdf5Path, int idx, void **dataObj);
int hdf5GetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

int hdf5GetObjectSlice(int expIdx, char *hdf5Path, char *cpoPath, double time, void **obj);
int hdf5PutObjectSlice(int expIdx, char *hdf5Path, char *cpoPath, double time, void *obj);
int hdf5ReplaceLastObjectSlice(int expIdx, char *cpoPath, char *hdf5Path, void *obj);

//int hdf5CopyCpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur);
