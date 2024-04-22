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
using jbyteArray = void*;
using jboolean = bool;
using jmethodID = int;
using jfieldID = int;
using jthrowable = void*;
using jsize = std::size_t;

#define JNI_VERSION_1_6 0x00010006
#define JNI_OK 0
#define JNI_ERR -1

struct JavaVM
{
  jint GetEnv(void**, jint) const;
};

struct JNIEnv
{
  jint GetJavaVM(JavaVM**) const;

  jobject NewObject(jclass, jmethodID, ...) const;
  jobject NewGlobalRef(jobject) const;
  void DeleteGlobalRef(jobject) const;
  void DeleteLocalRef(jobject) const;

  jmethodID GetMethodID(jclass, const char*, const char*);
  jfieldID GetFieldID(jclass, const char*, const char*);
  jclass FindClass(const char*) const;
  jclass GetObjectClass(jobject) const;
  void CallVoidMethod(jobject, jmethodID, ...) const;
  jint GetIntField(jobject, jfieldID) const;

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
  jbyteArray NewByteArray(jsize) const;
  void* GetPrimitiveArrayCritical(jarray, jboolean*) const;
  void ReleasePrimitiveArrayCritical(jarray, void*, jint) const;

  jstring NewStringUTF(const char*) const;
  jlong GetStringUTFLength(jstring) const;
  const char* GetStringUTFChars(jstring, jint) const;
  void ReleaseStringUTFChars(jstring, const char*) const;
};

#define JNIEXPORT
#define JNICALL
