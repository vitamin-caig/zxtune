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

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Data_1Create(JNIEnv* env, jclass /*self*/, jbyteArray buffer)
{
  const jsize size = env->GetArrayLength(buffer);
  std::auto_ptr<Dump> content(new Dump(size));
  env->GetByteArrayRegion(buffer, 0, size, safe_ptr_cast<jbyte*>(&content->front()));
  const Binary::Container::Ptr data = Binary::CreateContainer(content);
  Dbg("Data::Create(%p, %u)=%p", buffer, size, data.get());
  return Data::Storage::Instance().Add(data);
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Data_1CreateModule
  (JNIEnv* /*env*/, jclass /*self*/, jint dataHandle)
{
  Dbg("Data::CreateModule(handle=0x%x)", dataHandle);
  if (const Binary::Container::Ptr data = Data::Storage::Instance().Get(dataHandle))
  {
    return Module::Create(data);
  }
  return 0;
}
