/*
Abstract:
  Data operating functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "data.h"
#include "zxtune.h"
//library includes
#include <binary/container_factories.h>

extern "C"
{
  JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Data_1Create(JNIEnv* env, jclass /*self*/, jobject buffer)
  {
    const void* const dataStart = env->GetDirectBufferAddress(buffer);
    const std::size_t dataSize = env->GetDirectBufferCapacity(buffer);
    const Binary::Container::Ptr data = Binary::CreateContainer(dataStart, dataSize);
    Debug("Data::Create(%p, %u)=%p", dataStart, dataSize, data.get());
    return Data::Storage::Instance().Add(data);
  }

  JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Data_1Destroy(JNIEnv* /*env*/, jclass /*self*/, jint dataHandle)
  {
    Debug("Data::Destroy(%x)", dataHandle);
    Data::Storage::Instance().Fetch(dataHandle);
  }
}
