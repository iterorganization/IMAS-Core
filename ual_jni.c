#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jni.h>

#include "ual_low_level.h"
#include "imasjava_imas.h"
#include "imasjava_UALLowLevel.h"
#ifdef USE_ITM_CATALOG
#include "ual_catalog.h"
#endif
#define INT 1
#define FLOAT 2
#define DOUBLE 3
#define STRING 4
#define BOOLEAN 5

static jobject make1DVect(JNIEnv *env, jclass cls, int type, int dim, void *data)
{
    jvalue args[5];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;
    jbooleanArray booleanArray;
    jobjectArray objectArray;
    jboolean *booleanNativeArray;
    jclass dataCls;
    int i;
    char **stringPtr;


    switch (type ) {
        case INT:
            intArray = (*env)->NewIntArray(env, dim);
            (*env)->SetIntArrayRegion(env, intArray, 0, dim, (int *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect1DInt", "([I)Limasjava/Vect1DInt;");
            args[0].l = intArray;
             return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case FLOAT:
            floatArray = (*env)->NewFloatArray(env, dim);
            (*env)->SetFloatArrayRegion(env, floatArray, 0, dim, (float *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect1DFloat", "([F)Limasjava/Vect1DFloat;");
            args[0].l = floatArray;
           return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect1DDouble", "([D)Limasjava/Vect1DDouble;");
            args[0].l = doubleArray;
          return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case BOOLEAN:
            booleanArray = (*env)->NewBooleanArray(env, dim);
            booleanNativeArray = (jboolean *)malloc(sizeof(jboolean)* dim);
            for(i = 0; i < dim; i++)
                booleanNativeArray[i] = ((int *)data)[i];
            (*env)->SetBooleanArrayRegion(env, booleanArray, 0, dim, booleanNativeArray);
            free((char *)booleanNativeArray);
            method = (*env)->GetStaticMethodID(env, cls, "getVect1DBoolean", "([Z)Limasjava/Vect1DBoolean;");
            args[0].l = booleanArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case STRING:
            dataCls = (*env)->FindClass(env, "java/lang/String");
            objectArray = (*env)->NewObjectArray(env, dim, dataCls, 0);
            stringPtr = (char **)data;
            for(i = 0; i < dim; i++)
                (*env)->SetObjectArrayElement(env, objectArray, i, (jobject)(*env)->NewStringUTF(env, stringPtr[i]));
            method = (*env)->GetStaticMethodID(env, cls, "getVect1DString", "([Ljava/lang/String;)Limasjava/Vect1DString;");
            args[0].l = objectArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}
static jobject make2DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, void *data)
{
    jvalue args[5];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case INT:
            intArray = (*env)->NewIntArray(env, dim1*dim2);
            (*env)->SetIntArrayRegion(env, intArray, 0, dim1*dim2, (int *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect2DInt", "(II[I)Limasjava/Vect2DInt;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].l = intArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case FLOAT:
            floatArray = (*env)->NewFloatArray(env, dim1*dim2);
            (*env)->SetFloatArrayRegion(env, floatArray, 0, dim1*dim2, (float *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect2DFloat", "(II[F)Limasjava/Vect2DFloat;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].l = floatArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect2DDouble", "(II[D)Limasjava/Vect2DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}
static jobject make3DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, int dim3, void *data)
{
    jvalue args[5];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case INT:
            intArray = (*env)->NewIntArray(env, dim1*dim2*dim3);
            (*env)->SetIntArrayRegion(env, intArray, 0, dim1*dim2*dim3, (int *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect3DInt", "(III[I)Limasjava/Vect3DInt;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].l = intArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
        case FLOAT:
             floatArray = (*env)->NewFloatArray(env, dim1*dim2*dim3);
            (*env)->SetFloatArrayRegion(env, floatArray, 0, dim1*dim2*dim3, (float *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect3DFloat", "(III[F)Limasjava/Vect3DFloat;");
           args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].l = floatArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);


        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2*dim3);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2*dim3, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect3DDouble", "(III[D)Limasjava/Vect3DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}
static jobject make4DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, int dim3, int dim4, void *data)
{
    jvalue args[5];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2*dim3*dim4);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2*dim3*dim4, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect4DDouble", "(IIII[D)Limasjava/Vect4DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].i = dim4;
            args[4].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}
static jobject make5DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, int dim3, int dim4, int dim5, void *data)
{
    jvalue args[6];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2*dim3*dim4*dim5);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2*dim3*dim4*dim5, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect5DDouble", "(IIIII[D)Limasjava/Vect5DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].i = dim4;
            args[4].i = dim5;
            args[5].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}
static jobject make6DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, int dim3, int dim4, int dim5,
    int dim6, void *data)
{
    jvalue args[7];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2*dim3*dim4*dim5*dim6);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2*dim3*dim4*dim5*dim6, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect6DDouble", "(IIIIII[D)Limasjava/Vect6DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].i = dim4;
            args[4].i = dim5;
            args[5].i = dim6;
            args[6].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}

static jobject make7DVect(JNIEnv *env, jclass cls, int type, int dim1, int dim2, int dim3, int dim4, int dim5,
    int dim6, int dim7, void *data)
{
    jvalue args[8];
    jmethodID method;
    jintArray intArray;
    jfloatArray floatArray;
    jdoubleArray doubleArray;

    switch (type ) {
        case DOUBLE:
            doubleArray = (*env)->NewDoubleArray(env, dim1*dim2*dim3*dim4*dim5*dim6*dim7);
            (*env)->SetDoubleArrayRegion(env, doubleArray, 0, dim1*dim2*dim3*dim4*dim5*dim6*dim7, (double *)data);
            method = (*env)->GetStaticMethodID(env, cls, "getVect7DDouble", "(IIIIIII[D)Limasjava/Vect7DDouble;");
            args[0].i = dim1;
            args[1].i = dim2;
            args[2].i = dim3;
            args[3].i = dim4;
            args[4].i = dim5;
            args[5].i = dim6;
            args[6].i = dim7;
            args[7].l = doubleArray;
            return (*env)->CallStaticObjectMethodA(env, cls, method, args);
    }
    return 0;

}

static jobject makeVect(JNIEnv *env, jclass cls, int type, int numDims, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int dim7, void *data)
{
    switch(numDims) {
        case 1: return make1DVect(env, cls, type, dim1, data);
        case 2: return make2DVect(env, cls, type, dim1, dim2, data);
        case 3: return make3DVect(env, cls, type, dim1, dim2, dim3, data);
        case 4: return make4DVect(env, cls, type, dim1, dim2, dim3, dim4, data);
        case 5: return make5DVect(env, cls, type, dim1, dim2, dim3, dim4, dim5, data);
        case 6: return make6DVect(env, cls, type, dim1, dim2, dim3, dim4, dim5, dim6, data);
        case 7: return make7DVect(env, cls, type, dim1, dim2, dim3, dim4, dim5, dim6, dim7, data);
    }
    return 0;
}






static void raiseException(JNIEnv *env, char *msg)
{
    jclass exc = (*env)->FindClass(env, "imasjava/UALException");
    (*env)->ThrowNew(env, exc, msg);
 }





/*
 * Class:     UALLowLevel
 * Method:    beginIDSPutTimed
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_beginIDSPutTimed
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath, jint samples, jdoubleArray jTimesArray)
{
    /* printf("beginIDSPutTimed: samples= %d \n",samples);*/
    if (samples > 0)  {
      const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
      double *arr = (*env)->GetDoubleArrayElements(env, jTimesArray, 0);
      int status = beginIdsPutTimed(expIdx, (char *)path, samples, arr);
      (*env)->ReleaseDoubleArrayElements(env, jTimesArray, arr, 0);
      (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
      if (status)
        raiseException(env, imas_last_errmsg());
    }
}

/*
 * Class:     UALLowLevel
 * Method:    endIDSPutTimed
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSPutTimed
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsPutTimed(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
}

/*
 * Class:     UALLowLevel
 * Method:    beginIDSPut
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_beginIDSPut
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = beginIdsPut(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    endIDSPut
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSPut
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsPut(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
}


JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_beginIDSPutNonTimed
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = beginIdsPutNonTimed(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    endIDSPutNonTimed
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSPutNonTimed
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsPutNonTimed(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
}



JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_beginIDSPutSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = beginIdsPutSlice(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    endIDSPut
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSPutSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsPutSlice(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
}


JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_beginIDSReplaceLastSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = beginIdsReplaceLastSlice(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    endIDSPut
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSReplaceLastSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsReplaceLastSlice(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
}


/*
 * Class:     UALLowLevel
 * Method:    beginIDSGet
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_beginIDSGet
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath, jboolean isTimed)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int retSamples;
    int status = beginIdsGet(expIdx, (char *)path, (int)isTimed, &retSamples);
    (*env)->ReleaseStringUTFChars(env, jPath, (char *)path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retSamples;
}



/*
 * Class:     UALLowLevel
 * Method:    endIDSGet
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSGet
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsGet(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
}


/*
 * Class:     UALLowLevel
 * Method:    beginIDSGetSlice
 * Signature: (ILjava/lang/String;D)D
 */
JNIEXPORT jdouble JNICALL Java_imasjava_UALLowLevel_beginIDSGetSlice
(JNIEnv *env, jclass class, jint expIdx, jstring jPath, jdouble time)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double retTime;
    int status = beginIdsGetSlice(expIdx, (char *)path, time);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return time;
}



/*
 * Class:     UALLowLevel
 * Method:    endIDSGetSlice
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_endIDSGetSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    endIdsGetSlice(expIdx, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
}



/*
 * Class:     UALLowLevel
 * Method:    putString
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putString
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jData)
{
    const char *idsPath;
    const char *path;
    const char *data;
    int status;
    int dataLen;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jData)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        data = (*env)->GetStringUTFChars(env, jData, 0);

        //printf("JAVA PUT STRING %s\n", data);

	// needed by new putString profile!!!
	dataLen = (*env)->GetStringLength(env, jData);

        status = putString(expIdx, (char *)idsPath, (char *)path, (char *)data, dataLen);
        (*env)->ReleaseStringUTFChars(env, jData, data);
    }
   (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
   (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jint data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = putInt(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putBoolean
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jboolean data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = putInt(expIdx, (char *)idsPath, (char *)path, (int)data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;F)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloat data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = putFloat(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;D)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdouble data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = putDouble(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jint *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect1DInt(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim, (int)isTimed);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;[ZI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DBoolean
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jbooleanArray jArray, jint dim, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jboolean *arr;
    int *intArr;
    jsize len;
    int i, status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetBooleanArrayElements(env, jArray, 0);
        len = (*env)->GetArrayLength(env, jArray);
        intArr = (int *)malloc(sizeof(int) * len);
        for(i = 0; i < len; i++)
            intArr[i] = arr[i];
        status = putVect1DInt(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, intArr, dim, (int)isTimed);
        (*env)->ReleaseBooleanArrayElements(env, jArray, arr, 0);
        free((char *)intArr);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    putVect1DString
 * Signature: (ILjava/lang/String;Ljava/lang/String;[Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DString
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jobjectArray jArray, jint dim, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jsize len;
    jstring jStr;
    int i, status;
    const char *currStr;
    char **retArr;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        len = (*env)->GetArrayLength(env, jArray);
        retArr = (char **)malloc(sizeof(char *) * len);
        for(i = 0; i < len; i++)
        {
            jStr = (jstring)(*env)->GetObjectArrayElement(env, jArray, i);
            if(jStr == 0)
            {
                retArr[i] = malloc(1);
                *retArr[i] = 0;

            }
            else
            {
                currStr = (*env)->GetStringUTFChars(env, jStr, 0);
                retArr[i] = malloc(strlen(currStr) + 1);
                strcpy(retArr[i], currStr);
                (*env)->ReleaseStringUTFChars(env, jStr, currStr);
            }
        }
        status = putVect1DString(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, retArr, dim, (int)isTimed);
        for(i = 0; i < len; i++)
            free(retArr[i]);
        free((char *)retArr);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/*
 * Class:     UALLowLevel
 * Method:    putVect1DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jfloat *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect1DFloat(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim, (int)isTimed);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect1DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[III)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2, jboolean isTimed)
  {
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jint *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect2DInt(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, (int)isTimed);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jfloat *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect2DFloat(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, (int)isTimed);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect2DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2, jint dim3, jboolean isTimed)
  {
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jint *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect3DInt(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, dim3, isTimed);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    putVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2, jint dim3, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jfloat *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect3DFloat(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, dim3, (int)isTimed);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect3DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jint dim3, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;


    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect3DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, (int)isTimed);

        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect4DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect4DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jint dim3, jint dim4, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect4DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect5DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect5DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect5DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4, dim5, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect6DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect6DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5, jint dim6, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect6DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4, dim5, dim6, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect7DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect7DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5, jint dim6, jint dim7, jboolean isTimed)
{
    const char *idsPath;
    const char *path;
    const char *timeBasePath;
    jdouble *arr;
    int status;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect7DDouble(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4, dim5, dim6, dim7, (int)isTimed);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    AppendString
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putStringSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jstring jData, jdouble time)
{
    const char *idsPath;
    const char *path;
    const char *data;
    const char *timeBasePath;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jData)
    {
       // status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        data = (*env)->GetStringUTFChars(env, jData, 0);
        status = putStringSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (char *)data, time);
        (*env)->ReleaseStringUTFChars(env, jData, data);
    }
   (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
   (*env)->ReleaseStringUTFChars(env, jPath, path);
   (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jint data, jdouble time)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int status = putIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, data,time);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putBooleanSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jboolean data, jdouble time)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int status = putIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int)data, time);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;F)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloat data, jdouble time)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int status = putFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, data, time);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;D)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble data, jdouble time)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int status = putDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, data,time);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim, jdouble time)
{
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect1DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim,time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;[ZI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DBooleanSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jbooleanArray jArray, jint dim, jdouble time)
{
    const char *idsPath;
    const char *path;
    jboolean *arr;
    int *intArr;
    jsize len;
    int i, status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetBooleanArrayElements(env, jArray, 0);
        len = (*env)->GetArrayLength(env, jArray);
        intArr = (int *)malloc(sizeof(int) * len);
        for(i = 0; i < len; i++)
            intArr[i] = arr[i];
        status = putVect1DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, intArr, dim, time);
        (*env)->ReleaseBooleanArrayElements(env, jArray, arr, 0);
        free((char *)intArr);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}



/*
 * Class:     UALLowLevel
 * Method:    appendVect1DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect1DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim,time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect1DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim,time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[III)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2, jdouble time)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect2DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect2DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2,time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect2DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2,time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2, jint dim3, jdouble time)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect3DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, dim3,time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2, jint dim3, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect3DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, dim3,time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}


JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jint dim3, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect3DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}




//////////
/*
 * Class:     UALLowLevel
 * Method:    putVect4DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect4DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2,
     jint dim3, jint dim4, jdouble time)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect4DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, dim3,dim4, time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    appendVect4DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect4DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect4DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, dim3, dim4, time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect4DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2, jint dim3, jint dim4, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect4DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4, time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}


/////
/*
 * Class:     UALLowLevel
 * Method:    putVect5DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect5DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2,
     jint dim3, jint dim4, jint dim5, jdouble time)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect5DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, dim3,dim4, dim5, time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    putVect5DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect5DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect5DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, dim3, dim4, dim5, time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}




/*
 * Class:     UALLowLevel
 * Method:    putVect4DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect5DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect5DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4,
            dim5, time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    putVect6DFloatSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect6DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jint dim6, jdouble time)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = putVect6DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, arr, dim1, dim2, dim3, dim4, dim5, dim6, time);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}




/*
 * Class:     UALLowLevel
 * Method:    putVect6DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect6DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdoubleArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jint dim6, jdouble time)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = putVect6DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (double *)arr, dim1, dim2, dim3, dim4,
            dim5, dim6, time);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    putVect6DIntSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect6DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jintArray jArray, jint dim1, jint dim2,
     jint dim3, jint dim4, jint dim5, jint dim6, jdouble time)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = putVect6DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, (int *)arr, dim1, dim2, dim3,dim4, dim5, dim6, time);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
        raiseException(env, imas_last_errmsg());
}

//Replace Last Slice
/*
 * Class:     UALLowLevel
 * Method:    replaceLastStringSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastStringSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jData)
{
    const char *idsPath;
    const char *path;
    const char *data;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jData)
    {
       // status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        data = (*env)->GetStringUTFChars(env, jData, 0);
        status = replaceLastStringSlice(expIdx, (char *)idsPath, (char *)path, (char *)data);
        (*env)->ReleaseStringUTFChars(env, jData, data);
    }
   (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
   (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jint data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = replaceLastIntSlice(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastBooleanSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jboolean data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = replaceLastIntSlice(expIdx, (char *)idsPath, (char *)path, (int)data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;F)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloat data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = replaceLastFloatSlice(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;D)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdouble data)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = replaceLastDoubleSlice(expIdx, (char *)idsPath, (char *)path, data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect1DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim)
{
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect1DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;[ZI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect1DBooleanSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jbooleanArray jArray, jint dim)
{
    const char *idsPath;
    const char *path;
    jboolean *arr;
    int *intArr;
    jsize len;
    int i, status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetBooleanArrayElements(env, jArray, 0);
        len = (*env)->GetArrayLength(env, jArray);
        intArr = (int *)malloc(sizeof(int) * len);
        for(i = 0; i < len; i++)
            intArr[i] = arr[i];
        status = replaceLastVect1DIntSlice(expIdx, (char *)idsPath, (char *)path, intArr, dim);
        (*env)->ReleaseBooleanArrayElements(env, jArray, arr, 0);
        free((char *)intArr);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}



/*
 * Class:     UALLowLevel
 * Method:    appendVect1DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect1DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect1DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect1DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect1DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect1DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[III)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect2DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim1, jint dim2)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect2DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim1, dim2);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect2DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim1, jint dim2)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect2DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim1, dim2);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect2DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect2DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim1, jint dim2)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect2DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim1, dim2);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect3DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim1, jint dim2, jint dim3)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect3DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim1, dim2, dim3);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect3DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim1, jint dim2, jint dim3)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect3DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim1, dim2, dim3);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect3DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect3DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim1, jint dim2, jint dim3)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect3DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim1, dim2, dim3);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

////////////////////
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect4DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect4DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim1, dim2, dim3, dim4);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect4DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect4DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim1, dim2, dim3, dim4);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect3DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect4DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect4DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim1, dim2, dim3, dim4);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

////////////////////
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect5DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect5DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim1, dim2, dim3, dim4, dim5);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}
/*
 * Class:     UALLowLevel
 * Method:    appendVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect5DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect5DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim1, dim2, dim3, dim4, dim5);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     UALLowLevel
 * Method:    appendVect3DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect5DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect5DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim1, dim2, dim3, dim4, dim5);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}



JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect6DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jintArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jint dim6)
  {
    const char *idsPath;
    const char *path;
    jint *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        status = replaceLastVect6DIntSlice(expIdx, (char *)idsPath, (char *)path, (int *)arr, dim1, dim2, dim3, dim4, dim5, dim6);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect6DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jfloatArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jint dim6)
{
    const char *idsPath;
    const char *path;
    jfloat *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        status = replaceLastVect6DFloatSlice(expIdx, (char *)idsPath, (char *)path, arr, dim1, dim2, dim3, dim4, dim5, dim6);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastVect6DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jdoubleArray jArray, jint dim1, jint dim2,
    jint dim3, jint dim4, jint dim5, jint dim6)
{
    const char *idsPath;
    const char *path;
    jdouble *arr;
    int status = 0;

    idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    path = (*env)->GetStringUTFChars(env, jPath, 0);
    if(!jArray)
    {
//        status = deleteData(expIdx, (char *)idsPath, (char *)path);
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        status = replaceLastVect6DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (double *)arr, dim1, dim2, dim3, dim4, dim5, dim6);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}





////////////////////////////////
/*
 * Class:     UALLowLevel
 * Method:    getString
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_imasjava_UALLowLevel_getString
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    char *data;
    jstring retString;
    //printf("getSTRING idx= %d, %s, %s\n", expIdx, idsPath, path);
    int status = getString(expIdx, (char *)idsPath, (char *)path, &data);

    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retString = (*env)->NewStringUTF(env, data);
    free(data);
    return retString;
}


/*
 * Class:     UALLowLevel
 * Method:    getInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int data = 0;
    int status = getInt(expIdx, (char *)idsPath, (char *)path, &data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_imasjava_UALLowLevel_getBoolean
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int data = 0;
    int status = getInt(expIdx, (char *)idsPath, (char *)path, &data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return (jboolean)data;
}

/*
 * Class:     UALLowLevel
 * Method:    getFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;)F
 */
JNIEXPORT jfloat JNICALL Java_imasjava_UALLowLevel_getFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    float data = 0;
    int status = getFloat(expIdx, (char *)idsPath, (char *)path, &data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)D
 */
JNIEXPORT jdouble JNICALL Java_imasjava_UALLowLevel_getDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double data = 0;
    int status = getDouble(expIdx, (char *)idsPath, (char *)path, &data);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect1DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int *data;
    int dim;
    jobject retArr;
    int status = getVect1DInt(expIdx, (char *)idsPath, (char *)path, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, INT, 1, dim, 0, 0, 0, 0, 0,0, data);
    free((int *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect1DString
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect1DString;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DString
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    char **data;
    int dim, i;
    jobject retArr;

     int status;
    status = getVect1DString(expIdx, (char *)idsPath, (char *)path, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }

    retArr = makeVect(env, class, STRING, 1, dim, 0, 0, 0, 0, 0, 0, data);
    for(i = 0; i < dim; i++)
        free(data[i]);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DBoolean
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect1DBoolean;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DBoolean
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int *data;
    int dim;
    jobject retArr;
    int status = getVect1DInt(expIdx, (char *)idsPath, (char *)path, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, BOOLEAN, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((int *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect1DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    float *data;
    int dim;
    jobject retArr;
    int status = getVect1DFloat(expIdx, (char *)idsPath, (char *)path, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, FLOAT, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect1DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim;
    jobject retArr;
    int status = getVect1DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect2DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect2DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DInt(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, INT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect2DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    float *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DFloat(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, FLOAT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect2DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DInt
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect3DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DInt
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DInt(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, INT, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DFloat
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect3DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DFloat
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    float *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DFloat(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }

    retArr = makeVect(env, class, FLOAT, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect4DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect4DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect4DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4;
    jobject retArr;
    int status = getVect4DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3, &dim4);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 4, dim1, dim2, dim3, dim4, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect5DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect5DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect5DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5;
    jobject retArr;
    int status = getVect5DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3, &dim4, &dim5);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 5, dim1, dim2, dim3, dim4, dim5, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect5DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect5DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect6DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5, dim6;
    jobject retArr;
    int status = getVect6DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3, &dim4, &dim5, &dim6);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 6, dim1, dim2, dim3, dim4, dim5, dim6, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect7DDouble
 * Signature: (ILjava/lang/String;Ljava/lang/String;)LVect7DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect7DDouble
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5, dim6, dim7;
    jobject retArr;
    int status = getVect7DDouble(expIdx, (char *)idsPath, (char *)path, &data, &dim1, &dim2, &dim3, &dim4, &dim5, &dim6, &dim7);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 7, dim1, dim2, dim3, dim4, dim5, dim6, dim7, data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getStringSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_imasjava_UALLowLevel_getStringSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    char *data;
    jstring retString;
    double retTime;
        int status = getStringSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retString = (*env)->NewStringUTF(env, data);
    free(data);
    return retString;
}

/*
 * Class:     UALLowLevel
 * Method:    getIntSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int data;
    double retTime;
    int status = getIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getBooleanSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)Z
 */
JNIEXPORT jboolean JNICALL Java_imasjava_UALLowLevel_getBooleanSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int data;
    double retTime;
    int status = getIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    return (jboolean)data;
}

/*
 * Class:     UALLowLevel
 * Method:    getFloatSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)F
 */
JNIEXPORT jfloat JNICALL Java_imasjava_UALLowLevel_getFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    float data;
    double retTime;
    int status = getFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)D
 */
JNIEXPORT jdouble JNICALL Java_imasjava_UALLowLevel_getDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double data;
    double retTime;
    int status = getDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DIntSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect1DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int *data;
    int dim;
    jobject retArr;
    double retTime;
    int status = getVect1DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, INT, 1, dim, 0, 0, 0,0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect1DFloatSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect1DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    float *data;
    int dim;
    jobject retArr;
    double retTime;
    int status = getVect1DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, FLOAT, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect1DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect1DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim;
    jobject retArr;
    double retTime;
    int status = getVect1DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DIntSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect2DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DIntSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    int *data;
    int dim1, dim2;
    jobject retArr;
    double retTime;
    int status = getVect2DIntSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, INT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DFloatSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect2DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DFloatSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    float *data;
    int dim1, dim2;
    jobject retArr;
    double retTime;
    int status = getVect2DFloatSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, FLOAT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect2DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim1, dim2;
    jobject retArr;
    double retTime;
    int status = getVect2DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect3DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim1, dim2, dim3;
    jobject retArr;
    double retTime;
    int status = getVect3DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, &dim3, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect4DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim1, dim2, dim3, dim4;
    jobject retArr;
    double retTime;
    int status = getVect4DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, &dim3, &dim4, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 4, dim1, dim2, dim3, dim4, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect5DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5;
    jobject retArr;
    double retTime;
    int status = getVect5DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, &dim3, &dim4, &dim5, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 5, dim1, dim2, dim3, dim4, dim5, 0, 0,data);
    free((char *)data);
    return retArr;
}



/*
 * Class:     UALLowLevel
 * Method:    getVect6DDoubleSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect6DDoubleSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath, jstring jTimeBasePath, jdouble time, jint interpolMode)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    const char *timeBasePath = (*env)->GetStringUTFChars(env, jTimeBasePath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5, dim6;
    jobject retArr;
    double retTime;
    int status = getVect6DDoubleSlice(expIdx, (char *)idsPath, (char *)path, (char *)timeBasePath, &data, &dim1, &dim2, &dim3, &dim4, &dim5, &dim6, time, &retTime, interpolMode);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    (*env)->ReleaseStringUTFChars(env, jTimeBasePath, timeBasePath);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    retArr = makeVect(env, class, DOUBLE, 6, dim1, dim2, dim3, dim4, dim5, dim6, 0, data);
    free((char *)data);
    return retArr;
}


JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_deleteData
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);
    int status = deleteData(expIdx, (char *)idsPath, (char *)path);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
    }
}


/*
 * Class:     IDS
 * Method:    connect
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_connect
  (JNIEnv *env, jclass class, jstring jip)
{
    const char *ip = (*env)->GetStringUTFChars(env, jip, 0);
    int status = imas_connect((char *)ip);
    (*env)->ReleaseStringUTFChars(env, jip, ip);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     IDS
 * Method:    disconnect
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_disconnect
  (JNIEnv *env, jclass class)
{
    int status = imas_disconnect();
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     IDS
 * Method:    exec
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_imasjava_imas_exec
  (JNIEnv *env, jclass class, jstring jip, jstring jcommand)
{
    jstring jstdOut;
    char errorMsg[1000];
    const char *ip = (*env)->GetStringUTFChars(env, jip, 0);
    const char *command = (*env)->GetStringUTFChars(env, jcommand, 0);
    char *stdOut = imas_exec((char *)ip,(char *)command);
    (*env)->ReleaseStringUTFChars(env, jip, ip);
    (*env)->ReleaseStringUTFChars(env, jcommand, command);
    if(stdOut==NULL) {
        sprintf(errorMsg,"Error executing command: %s",command);
        raiseException(env,errorMsg);
    }
    jstdOut = (*env)->NewStringUTF(env, stdOut);
    free(stdOut);
    return jstdOut;
}


/*
 * Class:     IDS
 * Method:    open
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_open
  (JNIEnv *env, jclass class, jstring jName, jint shot , jint run)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_open((char *)name, shot, run, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    create
 * Signature: (Ljava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_create
  (JNIEnv *env, jclass class, jstring jName, jint shot, jint run, jint refShot, jint refRun)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_create((char *)name, shot, run, refShot, refRun, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    open_public
 * Signature: (Ljava/lang/String;Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_open_public
  (JNIEnv *env, jclass class, jstring jExpName, jstring jName, jint shot , jint run)
{
    int retIdx = -1;
    const char *exp_name = (*env)->GetStringUTFChars(env, jExpName, 0);
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_open_public(exp_name, (char *)name, shot, run, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    create_public
 * Signature: (Ljava/lang/String;Ljava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_create_public
  (JNIEnv *env, jclass class, jstring jExpName, jstring jName, jint shot, jint run, jint refShot, jint refRun)
{
    int retIdx = -1;
    const char *exp_name = (*env)->GetStringUTFChars(env, jExpName, 0);
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_create_public(exp_name, (char *)name, shot, run, refShot, refRun, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    openEnv
 * Signature: (Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_openEnv
  (JNIEnv *env, jclass class, jstring jName, jint shot , jint run, jstring jUser, jstring jTokamak, jstring jVersion)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
    const char *tokamak = (*env)->GetStringUTFChars(env, jTokamak, 0);
    const char *version = (*env)->GetStringUTFChars(env, jVersion, 0);
    int status = imas_open_env((char *)name, shot, run, &retIdx, (char *)user, (char *)tokamak, (char *)version);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    (*env)->ReleaseStringUTFChars(env, jUser, user);
    (*env)->ReleaseStringUTFChars(env, jTokamak, tokamak);
    (*env)->ReleaseStringUTFChars(env, jVersion, version);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    createEnv
 * Signature: (Ljava/lang/String;IIIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_createEnv
  (JNIEnv *env, jclass class, jstring jName, jint shot, jint run, jint refShot, jint refRun, jstring jUser, jstring jTokamak, jstring jVersion)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
    const char *tokamak = (*env)->GetStringUTFChars(env, jTokamak, 0);
    const char *version = (*env)->GetStringUTFChars(env, jVersion, 0);
    int status = imas_create_env((char *)name, shot, run, refShot, refRun, &retIdx, (char *)user, (char *)tokamak, (char *)version);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    (*env)->ReleaseStringUTFChars(env, jUser, user);
    (*env)->ReleaseStringUTFChars(env, jTokamak, tokamak);
    (*env)->ReleaseStringUTFChars(env, jVersion, version);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    openHdf5
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_openHdf5
  (JNIEnv *env, jclass class, jstring jName, jint shot , jint run)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_open_hdf5((char *)name, shot, run, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    createHdf5
 * Signature: (Ljava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_createHdf5
  (JNIEnv *env, jclass class, jstring jName, jint shot, jint run, jint refShot, jint refRun)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    int status = imas_create_hdf5((char *)name, shot, run, refShot, refRun, &retIdx);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    openPublic
 * Signature: (Ljava/lang/String;IILjava/lang/String)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_openPublic
        (JNIEnv *env, jclass class, jstring jName, jint shot, jint run, jstring jExpName)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    const char *expName = (*env)->GetStringUTFChars(env, jExpName, 0);
    int status = imas_open_hdf5((char *)name, shot, run, &retIdx, expName);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    (*env)->ReleaseStringUTFChars(env, jExpName, expName);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    createPublic
 * Signature: (Ljava/lang/String;IIIILjava/lang/String)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_createPublic
        (JNIEnv *env, jclass class, jstring jName, jint shot, jint run, jint refShot, jint refRun, jstring jExpName)
{
    int retIdx = -1;
    const char *name = (*env)->GetStringUTFChars(env, jName, 0);
    const char *expName = (*env)->GetStringUTFChars(env, jExpName, 0);
    int status = imas_create_hdf5((char *)name, shot, run, refShot, refRun, &retIdx, expName);
    (*env)->ReleaseStringUTFChars(env, jName, name);
    (*env)->ReleaseStringUTFChars(env, jExpName, expName);
    if(status)
        raiseException(env, imas_last_errmsg());
    return retIdx;
}

/*
 * Class:     IDS
 * Method:    close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_close
  (JNIEnv *env, jclass class, jint idx)
{
    int status;
    imas_discard_mem_cache(idx);
    status = imas_close(idx);
    //Discard also possibly allocated memory
    //if(status)
      //  raiseException(env, imas_last_errmsg());
    return 0;
}

/*
 * Class:     IDS
 * Method:    enableMemCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_enableMemCache
  (JNIEnv *env, jclass class , jint expIdx)
{
    imas_enable_mem_cache(expIdx);
}

/*
 * Class:     IDS
 * Method:    disableMemCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_disableMemCache
  (JNIEnv *env, jclass class , jint expIdx)
{
    imas_disable_mem_cache(expIdx);
}


/*
 * Class:     IDS
 * Method:    flush
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_flush
  (JNIEnv *env, jclass class , jint expIdx, jstring jIdsPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    imas_flush_cpo_mem_cache(expIdx, (char *)idsPath);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
}
/*
 * Class:     IDS
 * Method:    discard
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_discard
  (JNIEnv *env, jclass class , jint expIdx, jstring jIdsPath)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    imas_discard_cpo_mem_cache(expIdx, (char *)idsPath);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
}

/*
 * Class:     IDS
 * Method:    discardAll
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_discardAll
  (JNIEnv *env, jclass class , jint expIdx)
{
    imas_discard_mem_cache(expIdx);
}

/*
 * Class:     IDS
 * Method:    flushAll
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_flushAll
  (JNIEnv *env, jclass class , jint expIdx)
{
    imas_flush_mem_cache(expIdx);
}

/*
 * Class:     IDS
 * Method:    setCacheLevel
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_imasjava_imas_setCacheLevel
  (JNIEnv *env, jclass class, jint expIdx, jint level)
{
  imas_set_cache_level(expIdx, level);
}

/*
 * Class:     IDS
 * Method:    getCacheLevel
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_imas_getCacheLevel
  (JNIEnv *env, jclass class, jint expIdx)
{
  return imas_get_cache_level(expIdx);
}


JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getUniqueRun
  (JNIEnv *env, jclass class, jint shot)
{
    return getUniqueRun(shot);
}

#ifdef USE_ITM_CATALOG
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putIdsDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine,
	jint shot, jint run, jstring jIdsName, jint occurrence, int isRef, jstring jRefUser, jstring jRefMachine, jint refShot, jint refRun, jint refOccurrence)
{
    const char *idsName = (*env)->GetStringUTFChars(env, jIdsName, 0);
    const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
    const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
    const char *refUser = (*env)->GetStringUTFChars(env, jRefUser, 0);
    const char *refMachine = (*env)->GetStringUTFChars(env, jRefMachine, 0);
    int status;
    printf("PUTIDSDB isRef: %d\n", isRef);
    status = ual_put_ids((char *)user, (char *)machine, shot, run, (char *)idsName, occurrence, isRef, (char *)refUser, (char *)refMachine, refShot, refRun, refOccurrence);
    if(status)
        raiseException(env, "Error accessing Catalogue DB in putIdsDb");
    (*env)->ReleaseStringUTFChars(env, jIdsName, idsName);
    (*env)->ReleaseStringUTFChars(env, jUser, user);
    (*env)->ReleaseStringUTFChars(env, jMachine, machine);
    (*env)->ReleaseStringUTFChars(env, jRefUser, refUser);
    (*env)->ReleaseStringUTFChars(env, jRefMachine, refMachine);
}

JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_openDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine, jint shot,
 jint run, jstring jIdsName, jint occurrence)
{
	const char *idsName = (*env)->GetStringUTFChars(env, jIdsName, 0);
	const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
	const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
	int status, isRef, refShot, refRun, refOccurrence, retIdx;
	long id;
	char refUser[512], refMachine[512], retDataV[512];
	char * dataV;

	status = ual_get_ids_ref((char *)user, (char *)machine, shot, run, (char *)idsName, occurrence,
	&isRef, refUser, refMachine, &refShot, &refRun, &refOccurrence);
	if(status) { // if entry is not found in the catalog, then try to open the DB directly
		printf("WARNING: the IDS could not be found in the catalog. Trying to read the database directly...\n");
		fflush(stdout);
		dataV = getenv("DATAVERSION");	// find data version using environment variable
		if (dataV == NULL)
			raiseException(env, "Error: DATAVERSION is not defined");
		else
			status = imas_open_env("imas", shot, run, &retIdx, (char *)user, (char *)machine, dataV);
			if(status)
				raiseException(env, imas_last_errmsg());
		//raiseException(env, "Error accessing Catalogue DB in openDb");
	} else { // otherwise find data version in the catalog and open the DB
		status = ual_get_entry_id(refUser, refMachine, refShot, refRun, &id, retDataV);
		if(status){
			raiseException(env, "Error accessing Catalogue DB in openDb");
		} else {
			status = imas_open_env("imas", refShot, refRun, &retIdx, refUser, refMachine, retDataV);
			if(status)
				raiseException(env, imas_last_errmsg());
		}
	}
	(*env)->ReleaseStringUTFChars(env, jIdsName, idsName);
	(*env)->ReleaseStringUTFChars(env, jUser, user);
	(*env)->ReleaseStringUTFChars(env, jMachine, machine);
	return retIdx;
}

JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_createNewRunDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine,
	jint shot , jstring jDataV)
{
    const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
    const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
    const char *dataV = (*env)->GetStringUTFChars(env, jDataV, 0);
    int status, retRun;

    status = ual_create_new_run((char *)user, (char *)machine, shot, (char *)dataV, &retRun);
    if(status)
        raiseException(env, "Error accessing Catalogue DB in createNewRunDb");

    (*env)->ReleaseStringUTFChars(env, jUser, user);
    (*env)->ReleaseStringUTFChars(env, jMachine, machine);
    (*env)->ReleaseStringUTFChars(env, jDataV, dataV);

    return retRun;
}

JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_createNewRunParentDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine,
	 jint shot, jstring jDataV, jstring jParentUser, jstring jParentMachine, jint parentShot, jint parentRun)
{
    const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
    const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
    const char *parentUser = (*env)->GetStringUTFChars(env, jParentUser, 0);
    const char *parentMachine = (*env)->GetStringUTFChars(env, jParentMachine, 0);
    const char *dataV = (*env)->GetStringUTFChars(env, jDataV, 0);
    int status, retRun;

    status = ual_create_new_run_parent((char *)user, (char *)machine, shot, (char *)dataV, &retRun, (char *)parentUser, (char *)parentMachine, parentShot, parentRun);
    if(status)
        raiseException(env, "Error accessing Catalogue DB in createNewRunParentDb");

    (*env)->ReleaseStringUTFChars(env, jUser, user);
    (*env)->ReleaseStringUTFChars(env, jMachine, machine);
    (*env)->ReleaseStringUTFChars(env, jParentUser, parentUser);
    (*env)->ReleaseStringUTFChars(env, jParentMachine, parentMachine);
    (*env)->ReleaseStringUTFChars(env, jDataV, dataV);

    printf("Create DB retRUn: %d\n", retRun);


    return retRun;

}

JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_createSpecifiedRunDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine, jint shot, jint run, jstring jDataV)
{
	const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
	const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
	const char *dataV = (*env)->GetStringUTFChars(env, jDataV, 0);
	int status;

	status = ual_create_specified_run((char *)user, (char *)machine, shot, run, (char *)dataV);
	if(status)
		raiseException(env, "Error accessing Catalogue DB in createSpecifiedRunDb");

	(*env)->ReleaseStringUTFChars(env, jUser, user);
	(*env)->ReleaseStringUTFChars(env, jMachine, machine);
	(*env)->ReleaseStringUTFChars(env, jDataV, dataV);
}

JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_createSpecifiedRunParentDb (JNIEnv *env, jclass cls, jstring jUser, jstring jMachine, jint shot, jint run, jstring jDataV, jstring jParentUser, jstring jParentMachine, jint parentShot, jint parentRun)
{
	const char *user = (*env)->GetStringUTFChars(env, jUser, 0);
	const char *machine = (*env)->GetStringUTFChars(env, jMachine, 0);
	const char *parentUser = (*env)->GetStringUTFChars(env, jParentUser, 0);
	const char *parentMachine = (*env)->GetStringUTFChars(env, jParentMachine, 0);
	const char *dataV = (*env)->GetStringUTFChars(env, jDataV, 0);
	int status;

	status = ual_create_specified_run_parent((char *)user, (char *)machine, shot, run, (char *)dataV, (char *)parentUser, (char *)parentMachine, parentShot, parentRun);
	if(status)
		raiseException(env, "Error accessing Catalogue DB in createSpecifiedRunParentDb");

	(*env)->ReleaseStringUTFChars(env, jUser, user);
	(*env)->ReleaseStringUTFChars(env, jMachine, machine);
	(*env)->ReleaseStringUTFChars(env, jParentUser, parentUser);
	(*env)->ReleaseStringUTFChars(env, jParentMachine, parentMachine);
	(*env)->ReleaseStringUTFChars(env, jDataV, dataV);

}

/*
extern int ual_get_entry_id(char *user, char *machine, int shot, int run,
	long *retId, char *retDataV);
extern int ual_create_new_run(char *user, char *machine, int shot, char *dataV, int *retRun);
extern int ual_create_new_run_reference(char *user, char *machine, int shot, char *dataV, int *retRun,
	char *parentUser, char *parentMachine, int parentShot, int parentRun);
extern int ual_put_ids(char *user, char *machine, int shot, int run, char *idsName, int idsOccurrence, int isRef,
   char *refUser, char *refMachine, int refShot, int refRun, int refOccurrence);
extern int ual_get_ids_ref(char *user, char *machine, int shot, int run, char *idsName, int idsOccurrence,
  int *isRef, char *refUser, char *refMachine, int *refShot, int *refRun, int *refOccurrence);
*/

#endif /* USE_ITM_CATALOG */


JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getShot (JNIEnv *env, jclass cls, jint idx)
{
    return ual_get_shot(idx);
}
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getRun (JNIEnv *env, jclass cls, jint idx)
{
    return ual_get_run(idx);
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    getDimension
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Limasjava/Vect1DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getDimension
(JNIEnv *env, jclass class, jint expIdx, jstring jIDSPath, jstring jFieldPath)
{
	jobject dimsVect;
	int status;
	int ndims;
	int *dim = malloc(7*sizeof(int));
	const char *IDSpath = (*env)->GetStringUTFChars(env, jIDSPath, 0);
	const char *fieldpath = (*env)->GetStringUTFChars(env, jFieldPath, 0);
	status = getDimension(expIdx,(char *)IDSpath,(char *)fieldpath,&ndims,&dim[0],&dim[1],&dim[2],&dim[3],&dim[4],&dim[5],&dim[6]);
	(*env)->ReleaseStringUTFChars(env, jIDSPath, IDSpath);
	(*env)->ReleaseStringUTFChars(env, jFieldPath, fieldpath);
	if(status) {
		raiseException(env, imas_last_errmsg());
		return 0;
	}
	dimsVect = makeVect(env,class,INT,1,ndims,0,0,0,0,0,0,dim);
	free((int *)dim);
	return dimsVect;
}

/***************** ARRAYS OF STRUCTURES ********************/

/*
 * Class:     UALLowLevel
 * Method:    beginObject
 * Signature: (IIILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_beginObject
  (JNIEnv *env, jclass class, jint expIdx, jint jParent, jint index, jstring jRelPath, jint isTimed)
{
    int jObj;
    void *obj,*parent;
    const char *relPath = (*env)->GetStringUTFChars(env, jRelPath, 0);
    if (jParent<0)
        parent = NULL;
    else
        parent = getObjectFromList(jParent);
    obj = beginObject(expIdx,parent,index,relPath,(int)isTimed);
    (*env)->ReleaseStringUTFChars(env, jRelPath, relPath);
    jObj = addObjectToList(obj);
    if (jObj < 0) {
      releaseObject(expIdx,obj);
      raiseException(env, "No more slots available for arrays of structures");
    } else {
      return jObj;
    }
    return -1;
}

/*
 * Class:     UALLowLevel
 * Method:    releaseObject
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_releaseObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj)
{
    void *obj = getObjectFromList(jObj);
    removeObjectFromList(jObj);
    releaseObject(expIdx, obj);
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    getObjectDim
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getObjectDim
  (JNIEnv *env, jclass class, jint expIdx, jint jObj)
{
    void *obj = getObjectFromList(jObj);
    return getObjectDim(expIdx, obj);
}

/*
 * Class:     UALLowLevel
 * Method:    putObject
 * Signature: (ILjava/lang/String;Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putObject
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jSubPath, jint jObjData, jint isTimed)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void *obj = getObjectFromList(jObjData);
    int status = putObject(expIdx, (char *)idsPath, (char *)path, obj, isTimed);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    getObject
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getObject
  (JNIEnv *env, jclass class, jint expIdx, jstring  jIdsPath, jstring jSubPath, jint isTimed)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void * obj;
    int jObj;
    int status = getObject(expIdx, (char *)idsPath, (char *)path, &obj, isTimed);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
/*        raiseException(env, imas_last_errmsg());*/
          return -1;
    jObj = addObjectToList(obj);
    if (jObj < 0) {
      releaseObject(expIdx,obj);
      raiseException(env, "No more slots available for arrays of structures");
    } else {
      return jObj;
    }
    return -1;
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    putObjectSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;DI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putObjectSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jSubPath, jdouble time, jint jObjData)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void *obj = getObjectFromList(jObjData);
    int status = putObjectSlice(expIdx, (char *)idsPath, (char *)path, time, obj);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    getObjectSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;D)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getObjectSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jSubPath, jdouble time)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void *obj;
    int jObj;
    int status = getObjectSlice(expIdx, (char *)idsPath, (char *)path, time, &obj);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
        return 0;
    }
    jObj = addObjectToList(obj);
    if (jObj < 0) {
      releaseObject(expIdx,obj);
      raiseException(env, "No more slots available for arrays of structures");
    } else {
      return jObj;
    }
    return -1;
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    replaceLastObjectSlice
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_replaceLastObjectSlice
  (JNIEnv *env, jclass class, jint expIdx, jstring jIdsPath, jstring jSubPath, jint jObjData)
{
    const char *idsPath = (*env)->GetStringUTFChars(env, jIdsPath, 0);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void *obj = getObjectFromList(jObjData);
    int status = replaceLastObjectSlice(expIdx, (char *)idsPath, (char *)path, obj);
    (*env)->ReleaseStringUTFChars(env, jIdsPath, idsPath);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    putObjectInObject
 * Signature: (IILjava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putObjectInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jint jObjData)
{
    void *newObj;
    void *obj = getObjectFromList(jObj);
    void *objData = getObjectFromList(jObjData);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    newObj = putObjectInObject(expIdx, obj, (char *)path, idx, objData);
    replaceObjectInList(jObj, newObj);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putStringInObject
 * Signature: (IILjava/lang/String;ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putStringInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jstring jData)
{
    const char *path;
    const char *data;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jData)
    {
        // if the string is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        data = (*env)->GetStringUTFChars(env, jData, 0);
        newObj = putStringInObject(expIdx, obj, (char *)path, idx, (char *)data);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseStringUTFChars(env, jData, data);
    }
   (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putIntInObject
 * Signature: (IILjava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putIntInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jint data)
{
    void *newObj;
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    newObj = putIntInObject(expIdx, obj, (char *)path, idx, data);
    replaceObjectInList(jObj, newObj);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putFloatInObject
 * Signature: (IILjava/lang/String;IF)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putFloatInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jfloat data)
{
    void *newObj;
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    newObj = putFloatInObject(expIdx, obj, (char *)path, idx, data);
    replaceObjectInList(jObj, newObj);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putDoubleInObject
 * Signature: (IILjava/lang/String;ID)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdouble data)
{
    void *newObj;
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    newObj = putDoubleInObject(expIdx, obj, (char *)path, idx, data);
    replaceObjectInList(jObj, newObj);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DIntInObject
 * Signature: (IILjava/lang/String;I[II)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DIntInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jintArray jArray, jint dim)
{
    const char *path;
    jint *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        newObj = putVect1DIntInObject(expIdx, obj, (char *)path, idx, (int *)arr, dim);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DStringInObject
 * Signature: (IILjava/lang/String;I[Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DStringInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jobjectArray jArray, jint dim)
{
    const char *path;
    jsize len;
    jstring jStr;
    int i;
    const char *currStr;
    char **retArr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        len = (*env)->GetArrayLength(env, jArray);
        retArr = (char **)malloc(sizeof(char *) * len);
        for(i = 0; i < len; i++)
        {
            jStr = (jstring)(*env)->GetObjectArrayElement(env, jArray, i);
            if(jStr == 0)
            {
                retArr[i] = malloc(1);
                *retArr[i] = 0;

            }
            else
            {
                currStr = (*env)->GetStringUTFChars(env, jStr, 0);
                retArr[i] = malloc(strlen(currStr) + 1);
                strcpy(retArr[i], currStr);
                (*env)->ReleaseStringUTFChars(env, jStr, currStr);
            }
        }
        newObj = putVect1DStringInObject(expIdx, obj, (char *)path, idx, retArr, dim);
        replaceObjectInList(jObj, newObj);
        for(i = 0; i < len; i++)
            free(retArr[i]);
        free((char *)retArr);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}


/*
 * Class:     UALLowLevel
 * Method:    putVect1DFloatInObject
 * Signature: (IILjava/lang/String;I[FI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DFloatInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jfloatArray jArray, jint dim)
{
    const char *path;
    jfloat *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        newObj = putVect1DFloatInObject(expIdx, obj, (char *)path, idx, arr, dim);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect1DDoubleInObject
 * Signature: (IILjava/lang/String;I[DI)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect1DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect1DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DIntInObject
 * Signature: (IILjava/lang/String;I[III)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DIntInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jintArray jArray, jint dim1, jint dim2)
  {
    const char *path;
    jint *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        newObj = putVect2DIntInObject(expIdx, obj, (char *)path, idx, (int *)arr, dim1, dim2);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DFloatInObject
 * Signature: (IILjava/lang/String;I[FII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DFloatInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jfloatArray jArray, jint dim1, jint dim2)
{
    const char *path;
    jfloat *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        newObj = putVect2DFloatInObject(expIdx, obj, (char *)path, idx, arr, dim1, dim2);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect2DDoubleInObject
 * Signature: (IILjava/lang/String;I[DII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect2DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1, jint dim2)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect2DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect3DIntInObject
 * Signature: (IILjava/lang/String;I[IIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DIntInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jintArray jArray, jint dim1, jint dim2, jint dim3)
  {
    const char *path;
    jint *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetIntArrayElements(env, jArray, 0);
        newObj = putVect3DIntInObject(expIdx, obj, (char *)path, idx, (int *)arr, dim1, dim2, dim3);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseIntArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}
/*
 * Class:     UALLowLevel
 * Method:    putVect3DFloatInObject
 * Signature: (IILjava/lang/String;I[FIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DFloatInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jfloatArray jArray, jint dim1, jint dim2, jint dim3)
{
    const char *path;
    jfloat *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetFloatArrayElements(env, jArray, 0);
        newObj = putVect3DFloatInObject(expIdx, obj, (char *)path, idx, arr, dim1, dim2, dim3);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseFloatArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect3DDoubleInObject
 * Signature: (IILjava/lang/String;I[DIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect3DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1, jint dim2, jint dim3)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect3DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2, dim3);
        replaceObjectInList(jObj, newObj);

        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect4DDoubleInObject
 * Signature: (IILjava/lang/String;I[DIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect4DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1, jint dim2, jint dim3, jint dim4)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect4DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2, dim3, dim4);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect5DDoubleInObject
 * Signature: (IILjava/lang/String;I[DIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect5DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect5DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2, dim3, dim4, dim5);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect6DDoubleInObject
 * Signature: (IILjava/lang/String;I[DIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect6DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5, jint dim6)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect6DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2, dim3, dim4, dim5, dim6);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     UALLowLevel
 * Method:    putVect7DDoubleInObject
 * Signature: (IILjava/lang/String;I[DIIIIIII)V
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_putVect7DDoubleInObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx, jdoubleArray jArray, jint dim1,
    jint dim2, jint dim3, jint dim4, jint dim5, jint dim6, jint dim7)
{
    const char *path;
    jdouble *arr;

    void *newObj;
    void *obj = getObjectFromList(jObj);
    path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    if(!jArray)
    {
        // if the array is undefined then do nothing. Is it the right thing to do?
    }
    else
    {
        arr = (*env)->GetDoubleArrayElements(env, jArray, 0);
        newObj = putVect7DDoubleInObject(expIdx, obj, (char *)path, idx, (double *)arr, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
        replaceObjectInList(jObj, newObj);
        (*env)->ReleaseDoubleArrayElements(env, jArray, arr, 0);
    }
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
}

/*
 * Class:     imasjava_UALLowLevel
 * Method:    getObjectFromObject
 * Signature: (IILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getObjectFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    void * objData;
    int jObjData;
    int status = getObjectFromObject(expIdx, obj, (char *)path, idx, &objData);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
        raiseException(env, imas_last_errmsg());
    jObjData = addObjectToList(objData);
    if (jObjData < 0) {
      releaseObject(expIdx,objData);
      raiseException(env, "No more slots available for arrays of structures");
    } else {
      return jObjData;
    }
    return -1;
}

/*
 * Class:     UALLowLevel
 * Method:    getStringFromObject
 * Signature: (IILjava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_imasjava_UALLowLevel_getStringFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    char *data;
    jstring retString;

    int status = getStringFromObject(expIdx, obj, (char *)path, idx, &data);

    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retString = (*env)->NewStringUTF(env, data);
    free(data);
    return retString;
}


/*
 * Class:     UALLowLevel
 * Method:    getIntFromObject
 * Signature: (IILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_imasjava_UALLowLevel_getIntFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int data = 0;
    int status = getIntFromObject(expIdx, obj, (char *)path, idx, &data);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
	return EMPTY_INT;
    }
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getBooleanFromObject
 * Signature: (IILjava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_imasjava_UALLowLevel_getBooleanFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int data = 0;
    int status = getIntFromObject(expIdx, obj, (char *)path, idx, &data);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
	return 0;
    }
    return (jboolean)data;
}

/*
 * Class:     UALLowLevel
 * Method:    getFloatFromObject
 * Signature: (IILjava/lang/String;I)F
 */
JNIEXPORT jfloat JNICALL Java_imasjava_UALLowLevel_getFloatFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    float data = 0;
    int status = getFloatFromObject(expIdx, obj, (char *)path, idx, &data);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
//        raiseException(env, imas_last_errmsg());
    return EMPTY_FLOAT;
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getDoubleFromObject
 * Signature: (IILjava/lang/String;I)D
 */
JNIEXPORT jdouble JNICALL Java_imasjava_UALLowLevel_getDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double data = 0;
    int status = getDoubleFromObject(expIdx, obj, (char *)path, idx, &data);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
    return EMPTY_DOUBLE;
    return data;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DIntFromObject
 * Signature: (IILjava/lang/String;I)LVect1DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DIntFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int *data;
    int dim;
    jobject retArr;
    int status = getVect1DIntFromObject(expIdx, obj, (char *)path, idx, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, INT, 1, dim, 0, 0, 0, 0, 0,0, data);
    free((int *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect1DStringFromObject
 * Signature: (IILjava/lang/String;I)LVect1DString;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DStringFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    char **data;
    int dim, i;
    jobject retArr;

     int status;
    status = getVect1DStringFromObject(expIdx, obj, (char *)path, idx, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }

    retArr = makeVect(env, class, STRING, 1, dim, 0, 0, 0, 0, 0, 0, data);
    for(i = 0; i < dim; i++)
        free(data[i]);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DBooleanFromObject
 * Signature: (IILjava/lang/String;I)LVect1DBoolean;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DBooleanFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int *data;
    int dim;
    jobject retArr;
    int status = getVect1DIntFromObject(expIdx, obj, (char *)path, idx, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, BOOLEAN, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((int *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DFloatFromObject
 * Signature: (IILjava/lang/String;I)LVect1DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DFloatFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    float *data;
    int dim;
    jobject retArr;
    int status = getVect1DFloatFromObject(expIdx, obj, (char *)path, idx, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, FLOAT, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect1DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect1DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect1DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim;
    jobject retArr;
    int status = getVect1DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
    if(status)
    {
//GABRIELE MARCH 2011: try to reduce size of Java Code
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 1, dim, 0, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect2DIntFromObject
 * Signature: (IILjava/lang/String;I)LVect2DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DIntFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DIntFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, INT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DFloatFromObject
 * Signature: (IILjava/lang/String;I)LVect2DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DFloatFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    float *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DFloatFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, FLOAT, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect2DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect2DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect2DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2;
    jobject retArr;
    int status = getVect2DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 2, dim1, dim2, 0, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DIntFromObject
 * Signature: (IILjava/lang/String;I)LVect3DInt;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DIntFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    int *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DIntFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, INT, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DFloatFromObject
 * Signature: (IILjava/lang/String;I)LVect3DFloat;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DFloatFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    float *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DFloatFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }

    retArr = makeVect(env, class, FLOAT, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect3DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect3DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect3DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2, dim3;
    jobject retArr;
    int status = getVect3DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 3, dim1, dim2, dim3, 0, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect4DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect4DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect4DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4;
    jobject retArr;
    int status = getVect4DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3, &dim4);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 4, dim1, dim2, dim3, dim4, 0, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect5DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect5DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect5DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5;
    jobject retArr;
    int status = getVect5DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3, &dim4, &dim5);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 5, dim1, dim2, dim3, dim4, dim5, 0, 0,data);
    free((char *)data);
    return retArr;
}

/*
 * Class:     UALLowLevel
 * Method:    getVect5DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect5DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect6DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5, dim6;
    jobject retArr;
    int status = getVect6DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3, &dim4, &dim5, &dim6);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 6, dim1, dim2, dim3, dim4, dim5, dim6, 0,data);
    free((char *)data);
    return retArr;
}


/*
 * Class:     UALLowLevel
 * Method:    getVect7DDoubleFromObject
 * Signature: (IILjava/lang/String;I)LVect7DDouble;
 */
JNIEXPORT jobject JNICALL Java_imasjava_UALLowLevel_getVect7DDoubleFromObject
  (JNIEnv *env, jclass class, jint expIdx, jint jObj, jstring jSubPath, jint idx)
{
    void *obj = getObjectFromList(jObj);
    const char *path = (*env)->GetStringUTFChars(env, jSubPath, 0);
    double *data;
    int dim1, dim2, dim3, dim4, dim5, dim6, dim7;
    jobject retArr;
    int status = getVect7DDoubleFromObject(expIdx, obj, (char *)path, idx, &data, &dim1, &dim2, &dim3, &dim4, &dim5, &dim6, &dim7);
    (*env)->ReleaseStringUTFChars(env, jSubPath, path);
//GABRIELE MARCH 2011: try to reduce size of Java Code
    if(status)
    {
//        raiseException(env, imas_last_errmsg());
        return NULL;
    }
    retArr = makeVect(env, class, DOUBLE, 7, dim1, dim2, dim3, dim4, dim5, dim6, dim7, data);
    free((char *)data);
    return retArr;
}

//IDS copies

/*
 * Class:     UALLowLevel
 * Method:    ualCopyIds
 * Signature: (IILjava/lang/String;II)V;
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_ualCopyIds (JNIEnv *env, jclass class, jint fromIdx, jint toIdx, jstring jIdsName, jint fromIdsOccur, jint toIdsOccur)
{
    const char *idsName = (*env)->GetStringUTFChars(env, jIdsName, 0);
    int status = ual_copy_cpo(fromIdx, toIdx, (char *)idsName, fromIdsOccur, toIdsOccur);
    (*env)->ReleaseStringUTFChars(env, jIdsName, idsName);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
    }
}

/*
 * Class:     UALLowLevel
 * Method:    ualCopyIdsEnv
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;IIILjava/lang/String;)V;
 */
JNIEXPORT void JNICALL Java_imasjava_UALLowLevel_ualCopyIdsEnv (JNIEnv *env, jclass class, jstring jTokamakFrom, jstring jVersionFrom, jstring jUserFrom, jint shotFrom, jint runFrom, jint occurrenceFrom, jstring jTokamakTo, jstring jVersionTo, jstring jUserTo, jint shotTo, jint runTo, jint occurrenceTo, jstring jIdsName)
{
    const char *tokamakFrom = (*env)->GetStringUTFChars(env, jTokamakFrom, 0);
    const char *versionFrom = (*env)->GetStringUTFChars(env, jVersionFrom, 0);
    const char *userFrom = (*env)->GetStringUTFChars(env, jUserFrom, 0);
    const char *tokamakTo = (*env)->GetStringUTFChars(env, jTokamakTo, 0);
    const char *versionTo = (*env)->GetStringUTFChars(env, jVersionTo, 0);
    const char *userTo = (*env)->GetStringUTFChars(env, jUserTo, 0);
    const char *idsName = (*env)->GetStringUTFChars(env, jIdsName, 0);
    int status = ual_copy_cpo_env((char *)tokamakFrom, (char *)versionFrom, (char *)userFrom, shotFrom, runFrom, occurrenceFrom, (char *)tokamakTo, (char *)versionTo, (char *)userTo, shotTo, runTo, occurrenceTo, (char *)idsName);
    (*env)->ReleaseStringUTFChars(env, jIdsName, idsName);
    (*env)->ReleaseStringUTFChars(env, jUserTo, userTo);
    (*env)->ReleaseStringUTFChars(env, jVersionTo, versionTo);
    (*env)->ReleaseStringUTFChars(env, jTokamakTo, tokamakTo);
    (*env)->ReleaseStringUTFChars(env, jUserFrom, userFrom);
    (*env)->ReleaseStringUTFChars(env, jVersionFrom, versionFrom);
    (*env)->ReleaseStringUTFChars(env, jTokamakFrom, tokamakFrom);
    if(status)
    {
        raiseException(env, imas_last_errmsg());
    }
}

