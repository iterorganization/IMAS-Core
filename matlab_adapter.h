#ifndef UAL_LOW_LEVEL_H
#define UAL_LOW_LEVEL_H 1

/*#include "ual_types.h"*/
#include <complex.h>
#include "ual_defs.h"

#define LOG __FILE__,__LINE__

#define INTERPOLATION LINEAR_INTERP
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2


#define UNDEFINED false



/* management 
********************************************************************************/

double *getVect1DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *status);

double *getVect1DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim, int* status);

int *getVect1DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim, int* status);

double *getVect2DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, int *status);

int *getVect2DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim1, int *dim2, int* status);

double *getVect2DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim1, int *dim2, int* status);

double *getVect3DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, int *dim3, int *status);

int *getVect3DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim1, int *dim2, int *dim3, int* status);

double *getVect3DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim1, int *dim2, int *dim3, int* status);

double *getVect4DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
int *dim3, int *dim4, int *status);

int *getVect4DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim1, int *dim2, int *dim3, int *dim4, int* status);

double *getVect4DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim1, int *dim2, int *dim3, int *dim4, int* status);

double *getVect5DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
int *dim3, int *dim4, int *dim5, int *status);

int *getVect5DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int* status);

double *getVect5DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int* status);

double *getVect6DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
int *dim3, int *dim4, int *dim5, int *dim6, int *status);

int *getVect6DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int* status);

double *getVect6DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int* status);


void check_status(int status, const char *file, const int line);

int ual_create(const char *name, int shot, int run, int refShot, int refRun, 
		 int *pulseCtx);

int ual_open(const char *name, int shot, int run, int *pulseCtx);

int ual_close(int pulseCtx);

int ual_create_hdf5(const char *name, int shot, int run, int refShot, int refRun, int *retIdx);
int ual_open_hdf5(const char *name, int shot, int run, int *retIdx);

int ual_open_env(const char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
int ual_create_env(const char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);

int deleteData(int opCtx, const char *fieldPath);
int ual_discard_dataobject_mem_cache(int pulseCtx, const char *dataobjectPath);
int ual_flush_dataobject_mem_cache(int pulseCtx, const char *dataobjectPath);

int mtl_ual_begin_global_action(int pulseCtx, const char *path);
int mtl_ual_end_action(int pulseCtx);

int mtl_ual_begin_global_action_write(int pulseCtx, const char *path);

//int beginDataobjectGetSlice(int pulseCtx, const char *path, double time, int interpolMode);
int mtl_ual_begin_slice_action(int pulseCtx, const char *path, double time, int interpolMode);
//int endDataobjectSlice(int pulseCtx);

//int beginDataobjectPutSlice(int pulseCtx, const char *path, double time);
int mtl_ual_begin_put_slice_action(int pulseCtx, const char *path, double time);

int endDataobjectPutSlice(int pulseCtx);







/* readers 
********************************************************************************/
int *getVect1DInt_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim, int *status);
double *getVect1DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim, int *status);

int getChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data);
int getInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data);
int getDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data);
int getComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data);
int getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim);
int getVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim);
int getVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim);
int getVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim);
int getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim1, int *dim2);
int getVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2);
int getVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2);
int getVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int *dim2);
int getVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3);
int getVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3);
int getVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int *dim2, int *dim3);
int getVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex **data, int *dim1, int* dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);




/* writers
********************************************************************************/

/*int putString(int opCtx, const char *fieldPath, char *data);*/
int putChar(int opCtx, const char *fieldPath, const char *timebasePath, char data);
int putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data);
int putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data);
int putComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex data);
/*int putVect1DString(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int dim);*/
int putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
int putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
int putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
int putVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim);
int putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
int putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
int putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
int putVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2);
int putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
int putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
int putVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2, int dim3);
int putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2, int dim3, int dim4);
int putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
int putVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
int putVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
int putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
int putVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, double _Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);






/* array of structure 
********************************************************************************/

int mtl_ual_begin_write_arraystruct(int ctx, int index, const char *fieldPath, const char *timebasePath, int size);
int releaseObject(int aosCtx);
int putObject(int aosCtx);
int putObjectInObject(int aosCtx, const char *fieldPath, int idx, int subAosCtx, int size);
int getObjectDim(int aosCtx);
int mtl_ual_begin_read_arraystruct(int opCtx, const char *fieldPath, const char *timebase, int *size);
int mtl_ual_begin_read_arraystruct_from_arraystruct(int aosCtx, const char *fieldPath, const char *timebase, int idx, int *size);
int getDimensionFromObject(int opCtx, void *obj, const char *fieldPath, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getObjectSlice(int opCtx, const char *fieldPath, const char *timebase, int *size);
int putObjectSlice(int aosCtx);
int replaceLastObjectSlice(int aosCtx);







/* array of structure readers 
`********************************************************************************/

/*int getStringFromObject(int aosCtx, const char *fieldPath, int idx, char **data);*/
int getCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, char *data);
int getIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int *data);
int getDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double *data);
int getComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex *data);
/*int getVect1DStringFromObject(int aosCtx, const char *fieldPath, int idx, char  ***data, int *dim);*/
int getVect1DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, char **data, int *dim);
int getVect1DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim);
int getVect1DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim);
int getVect1DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim);
int getVect2DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, char **data, int *dim1, int *dim2);
int getVect2DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim1, int *dim2);
int getVect2DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim1, int *dim2);
int getVect2DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim1, int *dim2);
int getVect3DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim1, int *dim2, int *dim3);
int getVect3DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim1, int *dim2, int *dim3);
int getVect3DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim1, int *dim2, int *dim3);
int getVect4DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect5DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect6DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, int idx, double _Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);







/* array of structure writers 
********************************************************************************/

/*int putStringInObject(int aosCtx, const char *fieldPath, int idx, char *data);*/
int putCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char data);
int putIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int data);
int putDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double data);
int putComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex data);
/*int putVect1DStringInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char **data, int dim);*/
int putVect1DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char *data, int dim);
int putVect1DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim);
int putVect1DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim);
int putVect1DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim);
int putVect2DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char *data, int dim1, int dim2);
int putVect2DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2);
int putVect2DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2);
int putVect2DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2);
int putVect3DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2, int dim3);
int putVect3DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2, int dim3);
int putVect3DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2, int dim3);
int putVect4DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2, int dim3, int dim4);
int putVect5DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int putVect5DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
int putVect5DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5);
int putVect6DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int putVect6DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
int putVect6DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
int putVect7DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
int putVect7DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
int putVect7DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, double _Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);







/* TO BE DONE OR DEPRECATED 
********************************************************************************/

/*
int getUniqueRun(int shot);

int getDimension(int opCtx, const char *dataobjectPath, const char *fieldPath, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

int ual_get_shot(int idx);
int ual_get_run(int idx);

char *ual_last_errmsg();
void ual_enable_mem_cache(int expIdx);
void ual_disable_mem_cache(int expIdx);
void ual_discard_mem_cache(int expIdx);
void ual_flush_mem_cache(int expIdx);

int ual_connect(const char *ip);
int ual_disconnect();
char *ual_exec(const char *ip, const char *command);

// management of object list
void removeObjectFromList(int idx);
int addObjectToList(void *obj);
void replaceObjectInList(int idx, void *obj);
void *getObjectFromList(int idx);

//DATAOBJECT copies
int ual_copy_dataobject(int fromIdx, int toIdx, char *dataobjectName, int fromDataobjectOccur, int toDataobjectOccur);
int ual_copy_dataobject_env(char *tokamakFrom, char *versionFrom, char *userFrom, int shotFrom, int runFrom, int occurrenceFrom,  
	char *tokamakTo, char *versionTo, char *userTo, int shotTo, int runTo, int occurrenceTo, char *dataobjectName);

//New memory cache settings
void ual_set_cache_level(int expIdx, int level);
int ual_get_cache_level(int expIdx);

int isMds(int idx, int *expIdx);
*/

//Logging
//void log_message(int priority, const char *message);
//void log_stderr (int priority, const char *message);
//void log_journal(int priority, const char *message);


#endif
