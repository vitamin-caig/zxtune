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
#include "debug.h"
#include "module.h"
#include "zxtune.h"
//library includes
#include <binary/container_factories.h>

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Data_1Create(JNIEnv* env, jclass /*self*/, jobject buffer)
{
  const void* const dataStart = env->GetDirectBufferAddress(buffer);
  const std::size_t dataSize = env->GetDirectBufferCapacity(buffer);
  const Binary::Container::Ptr data = Binary::CreateContainer(dataStart, dataSize);
  Dbg("Data::Create(%p, %u)=%p", dataStart, dataSize, data.get());
  return Data::Storage::Instance().Add(data);
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Data_1CreateModule
  (JNIEnv* /*env*/, jclass /*self*/, jint dataHandle)
{
  Dbg("Data::CreateModule(handle=%x)", dataHandle);
  if (const Binary::Container::Ptr data = Data::Storage::Instance().Get(dataHandle))
  {
    return Module::Create(data);
  }
  return 0;
}
