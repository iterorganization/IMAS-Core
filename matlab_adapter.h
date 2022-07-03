#ifndef MATLAB_ADAPTER_H
#define MATLAB_ADAPTER_H 1

#include "ual_defs.h"
#include "ual_lowlevel.h"
#include <complex.h>

#define LOG __FILE__,__LINE__

#define INTERPOLATION LINEAR_INTERP
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

#define UNDEFINED false

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct c_complex_struct {
	int *shapes;
    double *array_real_part;
    double *array_imag_part;
};

//void test(struct c_struct**);
/* management 
********************************************************************************/
LIBRARY_API int mtl_ual_open(const char *uri, int mode);
LIBRARY_API int defaultBackend();
LIBRARY_API int fallbackBackend();
LIBRARY_API int mtl_ual_open_env(const char *name, int shot, int run, int *pulseCtx,char *user, char *tokamak, char *version);
LIBRARY_API int mtl_ual_create_env(const char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);
LIBRARY_API int mtl_ual_open_public(int shot, int run, int *pulseCtx, char *user, char *tokamak, char *version);
LIBRARY_API int mtl_ual_create_public(int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
LIBRARY_API int mtl_ual_close(int pulseCtx);

LIBRARY_API int mtl_deleteData(int opCtx, const char *fieldPath);

LIBRARY_API int mtl_ual_begin_global_action(int pulseCtx, const char *path);
LIBRARY_API int mtl_ual_end_action(int pulseCtx);
LIBRARY_API int mtl_ual_begin_global_action_write(int pulseCtx, const char *path);
LIBRARY_API int mtl_ual_begin_slice_action(int pulseCtx, const char *path, double time, int interpolMode);
LIBRARY_API int mtl_ual_begin_put_slice_action(int pulseCtx, const char *path, double time);
LIBRARY_API int mtl_ual_iterate_over_arraystruct(int aosctx, int step);

/* readers 
********************************************************************************/
LIBRARY_API int mtl_getInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data);
LIBRARY_API int mtl_getDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data);
LIBRARY_API int mtl_getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim);
LIBRARY_API int mtl_getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim1, int *dim2);
LIBRARY_API int mtl_getCPX_0D(int ctx, const char *fieldPath, const char *timebasePath, double *cpx_real, double *cpx_imag);
LIBRARY_API void mtl_getCPX_ND(int ctx, const char *fieldPath, const char *timebasePath, int ndim, int *status, struct c_complex_struct **st);
LIBRARY_API double *mtl_getVectNDDouble(int ctx, const char *fieldPath, const char *timebasePath, int *shapes, int dim, int *status);
LIBRARY_API int *mtl_getVectNDInt(int ctx, const char *fieldPath, const char *timebasePath, int *shapes, int dim, int *status);

/* writers
********************************************************************************/

LIBRARY_API int mtl_putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data);
LIBRARY_API int mtl_putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data);
LIBRARY_API int mtl_putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
LIBRARY_API int mtl_putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
LIBRARY_API int mtl_putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
LIBRARY_API int mtl_putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
LIBRARY_API int mtl_putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
LIBRARY_API int mtl_putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
LIBRARY_API int mtl_putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
LIBRARY_API int mtl_putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
LIBRARY_API int mtl_putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
LIBRARY_API int mtl_putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
LIBRARY_API int mtl_putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
LIBRARY_API int mtl_putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
LIBRARY_API int mtl_putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
LIBRARY_API int mtl_putCPX_0D(int ctx, const char *fieldPath, const char *timebasePath,
		double data_real, double data_imag);
LIBRARY_API int mtl_putCPX_ND(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data_real, double *data_imag, int ndim, int *shapes);

/* array of structure 
********************************************************************************/
LIBRARY_API int mtl_ual_begin_arraystruct_action(int opCtx, const char *fieldPath, const char *timebasePath, int *size);

/* array of structure writers 
********************************************************************************/
LIBRARY_API int mtl_putIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int data);
LIBRARY_API int mtl_putDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double data);
LIBRARY_API int mtl_putVect1DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
LIBRARY_API int mtl_putVect1DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
LIBRARY_API int mtl_putVect1DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
LIBRARY_API int mtl_putVect2DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect2DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect2DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
LIBRARY_API int mtl_putVect3DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
LIBRARY_API int mtl_putVect3DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
LIBRARY_API int mtl_putVect4DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
LIBRARY_API int mtl_putVect4DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
LIBRARY_API int mtl_putVect5DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
LIBRARY_API int mtl_putVect5DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
LIBRARY_API int mtl_putVect6DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
LIBRARY_API int mtl_putVect6DDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);


#ifdef __cplusplus
}
#endif

#endif // MATLAB_ADAPTER_H
