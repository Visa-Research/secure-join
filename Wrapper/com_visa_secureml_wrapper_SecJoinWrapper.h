/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class SecJoinWrapper */

#ifndef _Included_SecJoinWrapper
#define _Included_SecJoinWrapper
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_testApi
  (JNIEnv *, jobject);

JNIEXPORT jlong JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_initState
  (JNIEnv *, jobject, jstring, jstring, jstring, jstring, jstring, jstring, 
  jstring, jboolean, jboolean, jboolean, jboolean);

JNIEXPORT jbyteArray JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_runJoin
  (JNIEnv *, jobject, jlong, jbyteArray, jlong);

JNIEXPORT void JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_releaseState
  (JNIEnv *, jobject, jlong);

JNIEXPORT jboolean JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_isProtocolReady
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_getOtherShare
  (JNIEnv *, jobject, jlong, jboolean);

JNIEXPORT void JNICALL Java_com_visa_secureml_wrapper_SecJoinWrapper_getJoinTable
  (JNIEnv *, jobject, jlong, jstring, jstring, jboolean);

#ifdef __cplusplus
}
#endif
#endif
