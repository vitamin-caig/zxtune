#pragma once

#include <types.h>

using jstring = void*;
using jclass = void*;
using jobject = void*;
using jint = int32_t;
using jlong = int64_t;
using jarray = void*;
using jobjectArray = void*;
using jshortArray = void*;
using jintArray = void*;
using jboolean = bool;
using jmethodID = int;
using jthrowable = void*;
using jsize = std::size_t;

struct JNIEnv
{
  void DeleteLocalRef(jobject) const;

  jmethodID GetMethodID(jclass, const char*, const char*);
  jclass FindClass(const char*) const;
  jclass GetObjectClass(jobject) const;
  void CallNonvirtualVoidMethod(jobject, jclass, jmethodID, ...) const;
  void CallVoidMethod(jobject, jmethodID, ...) const;

  void ThrowNew(jclass, const char*) const;
  void Throw(jthrowable) const;
  jthrowable ExceptionOccurred() const;
  void ExceptionClear() const;
  void ExceptionDescribe() const;

  jobjectArray NewObjectArray(jint, jclass, jobject) const;
  void SetObjectArrayElement(jobjectArray, jint, jobject) const;
  const void* GetDirectBufferAddress(jobject) const;
  jlong GetDirectBufferCapacity(jobject) const;
  jsize GetArrayLength(jarray) const;
  void* GetPrimitiveArrayCritical(jarray, jboolean*) const;
  void ReleasePrimitiveArrayCritical(jarray, void*, jint) const;

  jstring NewStringUTF(const char*) const;
  jlong GetStringUTFLength(jstring) const;
  const char* GetStringUTFChars(jstring, jint) const;
  void ReleaseStringUTFChars(jstring, const char*) const;
};

#define JNIEXPORT
#define JNICALL
