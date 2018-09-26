#ifndef UAL_LOW_LEVEL_H
#define UAL_LOW_LEVEL_H 1

/*#include "ual_types.h"*/
#ifdef __cplusplus
	#include <complex>
	typedef std::complex<double> Complex;
#else
	#include <complex.h>
	typedef double _Complex Complex;
#endif
#include "ual_defs.h"

#define LOG __FILE__,__LINE__

#define INTERPOLATION LINEAR_INTERP
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

#define UNDEFINED false


#if defined(_WIN32) || defined(__MINGW32__)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

/* management 
********************************************************************************/

EXPORT int *mtl_getVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *dim, int *status);

EXPORT int *mtl_getVect1DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, 
			   int *dim, int* status);

EXPORT double *mtl_getVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *status);

EXPORT double *mtl_getVect1DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, 
			      int *dim, int* status);

EXPORT double *mtl_getVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, int *status);

EXPORT int *mtl_getVect2DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, 
			   int *dim1, int *dim2, int* status);

EXPORT double *mtl_getVect2DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath,
			      int *dim1, int *dim2, int* status);

EXPORT double *mtl_getVect3DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, int *dim3, int *status);

EXPORT int *mtl_getVect3DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, 
			   int *dim1, int *dim2, int *dim3, int* status);

EXPORT double *mtl_getVect3DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,  
			      int *dim1, int *dim2, int *dim3, int* status);

EXPORT double *mtl_getVect4DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
EXPORT int *dim3, int *dim4, int *status);

EXPORT int *mtl_getVect4DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,
			   int *dim1, int *dim2, int *dim3, int *dim4, int* status);

EXPORT double *mtl_getVect4DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,
			      int *dim1, int *dim2, int *dim3, int *dim4, int* status);

EXPORT double *mtl_getVect5DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
EXPORT int *dim3, int *dim4, int *dim5, int *status);

EXPORT int *mtl_getVect5DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,
			   int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int* status);

EXPORT double *mtl_getVect5DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,
			      int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int* status);

EXPORT double *mtl_getVect6DDouble_wrapper(int opCtx, const char *fieldPath, const char *timebasePath, int *dim1, int *dim2, 
EXPORT int *dim3, int *dim4, int *dim5, int *dim6, int *status);

EXPORT int *mtl_getVect6DIntFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath,
			   int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int* status);

EXPORT double *mtl_getVect6DDoubleFromObject_wrapper(int aosCtx, const char *fieldPath, const char *timebasePath, 
			      int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int* status);

EXPORT int mtl_ual_iterate_over_arraystruct(int aosctx, int step);

EXPORT int mtl_ual_create(const char *name, int shot, int run, int refShot, int refRun, 
		 int *pulseCtx);

EXPORT int mtl_ual_open(const char *name, int shot, int run, int *pulseCtx);

EXPORT int mtl_ual_close(int pulseCtx);

//EXPORT int ual_create_hdf5(const char *name, int shot, int run, int refShot, int refRun, int *retIdx);
//EXPORT int ual_open_hdf5(const char *name, int shot, int run, int *retIdx);

EXPORT int mtl_ual_open_env(const char *name, int shot, int run, int *pulseCtx,char *user, char *tokamak, char *version);
EXPORT int mtl_ual_create_env(const char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);
EXPORT int mtl_ual_open_public(int shot, int run, int *pulseCtx, char *user, char *tokamak, char *version);
EXPORT int mtl_ual_create_public(int shot, int run, int *retIdx, char *user, char *tokamak, char *version);

EXPORT int mtl_deleteData(int opCtx, const char *fieldPath);
EXPORT int mtl_ual_discard_dataobject_mem_cache(int pulseCtx, const char *dataobjectPath);
EXPORT int mtl_ual_flush_dataobject_mem_cache(int pulseCtx, const char *dataobjectPath);

EXPORT int mtl_ual_begin_global_action(int pulseCtx, const char *path);
EXPORT int mtl_ual_end_action(int pulseCtx);

EXPORT int mtl_ual_begin_global_action_write(int pulseCtx, const char *path);

//EXPORT int beginDataobjectGetSlice(int pulseCtx, const char *path, double time, int interpolMode);
EXPORT int mtl_ual_begin_slice_action(int pulseCtx, const char *path, double time, int interpolMode);
//EXPORT int endDataobjectSlice(int pulseCtx);

//EXPORT int beginDataobjectPutSlice(int pulseCtx, const char *path, double time);
EXPORT int mtl_ual_begin_put_slice_action(int pulseCtx, const char *path, double time);


/* readers 
********************************************************************************/

EXPORT int mtl_getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim);
EXPORT int mtl_getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim1, int *dim2);

EXPORT int mtl_getChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data);
EXPORT int mtl_getInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data);
EXPORT int mtl_getDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data);
EXPORT int mtl_getComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data);
EXPORT int mtl_getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim);
//EXPORT int mtl_getVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim);
//EXPORT int mtl_getVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim);
EXPORT int mtl_getVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim);
EXPORT int mtl_getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim1, int *dim2);
EXPORT int mtl_getVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2);
//EXPORT int mtl_getVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2);
EXPORT int mtl_getVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2);
EXPORT int mtl_getVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
EXPORT int mtl_getVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
EXPORT int mtl_getVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
EXPORT int mtl_getVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
EXPORT int mtl_getVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
EXPORT int mtl_getVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int* dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);


/* writers
********************************************************************************/

EXPORT int mtl_putChar(int opCtx, const char *fieldPath, const char *timebasePath, char data);
EXPORT int mtl_putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data);
EXPORT int mtl_putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data);
//EXPORT int mtl_putComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex data);
EXPORT int mtl_putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
EXPORT int mtl_putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
EXPORT int mtl_putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
EXPORT int mtl_putVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim);
EXPORT int mtl_putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
EXPORT int mtl_putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
EXPORT int mtl_putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
EXPORT int mtl_putVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2);
EXPORT int mtl_putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
EXPORT int mtl_putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
EXPORT int mtl_putVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5);
EXPORT int mtl_putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
EXPORT int mtl_putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
EXPORT int mtl_putVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
EXPORT int mtl_putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
EXPORT int mtl_putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
EXPORT int mtl_putVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);




/* array of structure 
********************************************************************************/

EXPORT int mtl_ual_begin_arraystruct_action(int opCtx, const char *fieldPath, const char *timebasePath, int *size);
EXPORT int releaseObject(int aosCtx);
EXPORT int putObject(int aosCtx);
EXPORT int putObjectSlice(int aosCtx);


/* array of structure readers 
`********************************************************************************/

EXPORT int mtl_getVect1DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, char **data, int *dim);
EXPORT int mtl_getVect2DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, char **data, int *dim1, int *dim2);

EXPORT int mtl_getCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, char *data);
EXPORT int mtl_getIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int *data);
EXPORT int mtl_getDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double *data);
EXPORT int mtl_getComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex *data);
EXPORT int mtl_getVect1DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, char **data, int *dim);
//EXPORT int mtl_getVect1DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim);
//EXPORT int mtl_getVect1DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim);
EXPORT int mtl_getVect1DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim);
EXPORT int mtl_getVect2DCharFromObject(int aosCtx, const char *fieldPath, const char *timebase, char **data, int *dim1, int *dim2);
//EXPORT int mtl_getVect2DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim1, int *dim2);
//EXPORT int mtl_getVect2DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim1, int *dim2);
EXPORT int mtl_getVect2DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim1, int *dim2);
EXPORT int mtl_getVect3DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect3DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect3DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim1, int *dim2, int *dim3);
EXPORT int mtl_getVect4DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect4DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect4DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4);
EXPORT int mtl_getVect5DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect5DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect5DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
EXPORT int mtl_getVect6DIntFromObject(int aosCtx, const char *fieldPath, const char *timebase, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
EXPORT int mtl_getVect6DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebase, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
EXPORT int mtl_getVect6DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebase, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);



/* array of structure writers 
********************************************************************************/

//EXPORT int mtl_putCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, char data);
EXPORT int mtl_putIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int data);
EXPORT int mtl_putDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double data);
//EXPORT int mtl_putComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex data);
//EXPORT int mtl_putVect1DStringInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char **data, int dim);
EXPORT int mtl_putVect1DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
EXPORT int mtl_putVect1DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
EXPORT int mtl_putVect1DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
EXPORT int mtl_putVect1DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim);
EXPORT int mtl_putVect2DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
EXPORT int mtl_putVect2DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
EXPORT int mtl_putVect2DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
EXPORT int mtl_putVect2DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2);
EXPORT int mtl_putVect3DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect3DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect3DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3);
EXPORT int mtl_putVect4DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect4DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect4DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4);
EXPORT int mtl_putVect5DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
EXPORT int mtl_putVect5DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
EXPORT int mtl_putVect5DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5);
EXPORT int mtl_putVect6DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
EXPORT int mtl_putVect6DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
EXPORT int mtl_putVect6DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
EXPORT int mtl_putVect7DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
EXPORT int mtl_putVect7DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
EXPORT int mtl_putVect7DComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);

#endif
