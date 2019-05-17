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



/* management 
********************************************************************************/

void check_status(int status, const char *file, const int line);

int ual_close(int pulseCtx);

int ual_open_env(const char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
int ual_create_env(const char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);







/* readers 
********************************************************************************/

int getChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data);
int getInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data);
int getDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data);
int getComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data);
int getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim);
int getVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim);
int getVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim);
int getVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim);
int getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int *dim1, int *dim2);
int getVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2);
int getVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2);
int getVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2);
int getVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3);
int getVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3);
int getVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3);
int getVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex **data, int *dim1, int* dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);




/* writers
********************************************************************************/

/*int putString(int opCtx, const char *fieldPath, char *data);*/
int putChar(int opCtx, const char *fieldPath, const char *timebasePath, char data);
int putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data);
int putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data);
int putComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex data);
/*int putVect1DString(int opCtx, const char *fieldPath, const char *timebasePath, char **data, int dim);*/
int putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim);
int putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim);
int putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim);
int putVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim);
int putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, char *data, int dim1, int dim2);
int putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2);
int putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2);
int putVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2);
int putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3);
int putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3);
int putVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3);
int putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4);
int putVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4);
int putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
int putVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
int putVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
int putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
int putVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, Complex *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);




#endif
