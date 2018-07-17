#include <jni.h>
#include "ual_lowlevel.h"
#include "ual_const.h"




static void raiseLowLevelException(JNIEnv *env, int errorCode)
{
    char* msg = NULL;

    
    jclass exc = env->FindClass("imasjava/UALException");
    env->ThrowNew(exc, msg);
 }

/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_print_context
 * Signature: (I)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1print_1context
  (JNIEnv *env, jclass jWrapperClass, jint jCtx)
{
    int status = -1;

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_print_context(jCtx);
    if (status < 0)
        raiseLowLevelException( env, status);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -
}

/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_begin_pulse_action
 * Signature: (IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
 jint JNICALL Java_imasjava_wrapper_LowLevel_ual_1begin_1pulse_1action
  (JNIEnv *env, jclass jWrapperClass, jint jBackendId, jint jShot, jint jRun, jstring jUser, jstring jTokamak, jstring jVersion)
{
    int status = -1;

    const char* user = env->GetStringUTFChars(jUser, 0);
    const char* tokamak = env->GetStringUTFChars(jTokamak, 0);
    const char* version = env->GetStringUTFChars(jVersion, 0);

 

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_begin_pulse_action(jBackendId, jShot, jRun, user, tokamak, version);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -


    env->ReleaseStringUTFChars(jUser, user);
    env->ReleaseStringUTFChars(jTokamak, tokamak);
    env->ReleaseStringUTFChars(jVersion, version);

    if (status < 0)
        raiseLowLevelException( env, status);

    return status;
}

/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_open_pulse
 * Signature: (IILjava/lang/String;)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1open_1pulse
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jint jMode, jstring jOptions)
{
    int status = -1;
    const char *options = env->GetStringUTFChars(jOptions, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_open_pulse(jCtx, jMode, options);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jOptions, options);

    if (status < 0)
        raiseLowLevelException( env, status);
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_close_pulse
 * Signature: (IILjava/lang/String;)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1close_1pulse
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jint jMode, jstring jOptions)
{
    int status = -1;
    const char *options = env->GetStringUTFChars(jOptions, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_close_pulse(jCtx, jMode, options);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jOptions, options);
   
    if (status < 0)
        raiseLowLevelException( env, status);
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_begin_global_action
 * Signature: (ILjava/lang/String;I)I
 */
 jint JNICALL Java_imasjava_wrapper_LowLevel_ual_1begin_1global_1action
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jDataObjectName, jint jRWMode)
{
    int status = -1;
    const char *dataObjectName = env->GetStringUTFChars(jDataObjectName, 0);


    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_begin_global_action(jCtx, dataObjectName, jRWMode);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -


    env->ReleaseStringUTFChars(jDataObjectName, dataObjectName);

    if (status < 0)
        raiseLowLevelException( env, status);

    return status;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_begin_slice_action
 * Signature: (ILjava/lang/String;IDI)I
 */
 jint JNICALL Java_imasjava_wrapper_LowLevel_ual_1begin_1slice_1action
  (JNIEnv *env, jclass jWrapperClass, jint jCtx,  jstring jDataObjectName, jint jRWMode, jdouble jTime, jint jInterpMode)
{
    int status = -1;
    const char *dataObjectName = env->GetStringUTFChars(jDataObjectName, 0);


    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_begin_slice_action(jCtx, dataObjectName, jRWMode, jTime, jInterpMode);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -


    env->ReleaseStringUTFChars(jDataObjectName, dataObjectName);

    if (status < 0)
        raiseLowLevelException( env, status);

    return status;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_end_action
 * Signature: (I)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1end_1action
  (JNIEnv *env, jclass jWrapperClass, jint jCtx)
{
    int status = -1;

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_end_action(jCtx);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    if (status < 0)
        raiseLowLevelException( env, status);
}

/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_write_data_int
 * Signature: (ILjava/lang/String;Ljava/lang/String;[II[I)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1write_1data_1int
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jintArray jData, jint jDim, jintArray jSizeArray)
{
    int status = -1;

    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jint *dataArray = env->GetIntArrayElements(jData, 0);
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_write_data(jCtx, fieldPath, timeBasePath, dataArray, INTEGER_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseIntArrayElements(jData, dataArray, 0);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);
}

/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_write_data_double
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DI[I)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1write_1data_1double
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jdoubleArray jData, jint jDim, jintArray jSizeArray)
{
    int status = -1;

    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jdouble *dataArray = env->GetDoubleArrayElements(jData, 0);
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_write_data(jCtx, fieldPath, timeBasePath, dataArray, DOUBLE_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseDoubleArrayElements(jData, dataArray, 0);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_write_data_char
 * Signature: (ILjava/lang/String;Ljava/lang/String;[BI[I)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1write_1data_1char
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jbyteArray jData, jint jDim, jintArray jSizeArray)
{
    int status = -1;

    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jbyte *dataArray = env->GetByteArrayElements(jData, 0);
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_write_data(jCtx, fieldPath, timeBasePath, dataArray, CHAR_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseByteArrayElements(jData, dataArray, 0);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_read_data_int
 * Signature: (ILjava/lang/String;Ljava/lang/String;[II[I)I
 */
 jintArray JNICALL Java_imasjava_wrapper_LowLevel_ual_1read_1data_1int
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jint jDim, jintArray jSizeArray)
{
    int status = -1;
    jsize retArraySize = 0;
    jintArray jData = NULL;
    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jint *dataArray = NULL;
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_read_data(jCtx, fieldPath, timeBasePath, (void**)&dataArray, INTEGER_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    if (sizeArray == NULL) {
     return NULL; /* out of memory error thrown */
    }   

    if(jDim == 0)
        retArraySize = 1;
    else
    {
        for (int i = 0; i < jDim; i++)
            retArraySize = retArraySize * sizeArray[i];
    }

    jData = env->NewIntArray(retArraySize);
    if (jData == NULL) {
     return NULL; /* out of memory error thrown */
    }
    env->SetIntArrayRegion(jData, 0, retArraySize, dataArray);

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);

    return jData;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_read_data_double
 * Signature: (ILjava/lang/String;Ljava/lang/String;[DI[I)I
 */
 jdoubleArray JNICALL Java_imasjava_wrapper_LowLevel_ual_1read_1data_1double
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jint jDim, jintArray jSizeArray)
{
    int status = -1;
    jsize retArraySize = 0;
    jdoubleArray jData = NULL;
    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jdouble *dataArray = NULL;
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_read_data(jCtx, fieldPath, timeBasePath, (void**)&dataArray, DOUBLE_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    if (sizeArray == NULL) {
     return NULL; /* out of memory error thrown */
    }   

    if(jDim == 0)
        retArraySize = 1;
    else
    {
        for (int i = 0; i < jDim; i++)
            retArraySize = retArraySize * sizeArray[i];
    }


    jData = env->NewDoubleArray(retArraySize);
    if (jData == NULL) {
     return NULL; /* out of memory error thrown */
    }
    env->SetDoubleArrayRegion(jData, 0, retArraySize, dataArray);
    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);

    return jData;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_read_data_char
 * Signature: (ILjava/lang/String;Ljava/lang/String;[BI[I)I
 */
 jbyteArray JNICALL Java_imasjava_wrapper_LowLevel_ual_1read_1data_1char
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jint jDim, jintArray jSizeArray)
{
    int status = -1;
    jsize retArraySize = 0;
    jbyteArray jData = NULL;
    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jbyte *dataArray =NULL;
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_read_data(jCtx, fieldPath, timeBasePath, (void**)&dataArray, CHAR_DATA, jDim, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    if (sizeArray == NULL) {
     return NULL; /* out of memory error thrown */
    }   

    if(jDim == 0)
        retArraySize = 1;
    else
    {
        for (int i = 0; i < jDim; i++)
            retArraySize = retArraySize * sizeArray[i];
    }


    jData = env->NewByteArray(retArraySize);
    if (jData == NULL) {
     return NULL; /* out of memory error thrown */
    }

    env->SetByteArrayRegion(jData, 0, retArraySize, dataArray);

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);

    return jData;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_delete_data
 * Signature: (ILjava/lang/String;)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1delete_1data
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath)
{
    int status = -1;

    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_delete_data(jCtx, fieldPath);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);

    if (status < 0)
        raiseLowLevelException( env, status);
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_begin_arraystruct_action
 * Signature: (ILjava/lang/String;Ljava/lang/String;[I)I
 */
 jint JNICALL Java_imasjava_wrapper_LowLevel_ual_1begin_1arraystruct_1action
  (JNIEnv *env, jclass jWrapperClass, jint jCtx, jstring jFieldPath, jstring jTimeBasePath, jintArray jSizeArray)
{
    int status = -1;

    const char *fieldPath = env->GetStringUTFChars(jFieldPath, 0);
    const char *timeBasePath = env->GetStringUTFChars(jTimeBasePath, 0);
    jint *sizeArray = env->GetIntArrayElements(jSizeArray, 0);

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_begin_arraystruct_action(jCtx, fieldPath, timeBasePath, sizeArray);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -

    env->ReleaseStringUTFChars(jFieldPath, fieldPath);
    env->ReleaseStringUTFChars(jTimeBasePath, timeBasePath);
    env->ReleaseIntArrayElements(jSizeArray, sizeArray, 0);

    if (status < 0)
        raiseLowLevelException( env, status);

    return status;
}
/*
 * Class:     imasjava_wrapper_LowLevel
 * Method:    ual_iterate_over_arraystruct
 * Signature: (II)I
 */
 void JNICALL Java_imasjava_wrapper_LowLevel_ual_1iterate_1over_1arraystruct
  (JNIEnv *env, jclass jWrapperClass, jint jAoSCtx, jint jStep)
{
    int status = -1;

    // - - - - - - - - - - UAL LowLevel method call - - - - - - - - - - - -
    status = ual_iterate_over_arraystruct(jAoSCtx, jStep);
    // - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - -


    if (status < 0)
        raiseLowLevelException( env, status);

}

