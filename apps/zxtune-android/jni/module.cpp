/*
Abstract:
  Module operating functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "debug.h"
#include "module.h"
#include "player.h"
#include "properties.h"
#include "zxtune.h"
//common includes
#include <error.h>
//library includes
#include <binary/container_factories.h>
#include <core/module_open.h>

namespace Module
{
  int Create(Binary::Container::Ptr data)
  {
    try
    {
      const ZXTune::Module::Holder::Ptr module = ZXTune::Module::Open(*data);
      Dbg("Module::Create(data=%p)=%p", data.get(), module.get());
      return Storage::Instance().Add(module);
    }
    catch (const Error&)
    {
      return 0;
    }
  }
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1Create
  (JNIEnv* env, jclass /*self*/, jbyteArray buffer)
{
  const jsize size = env->GetArrayLength(buffer);
  std::auto_ptr<Dump> content(new Dump(size));
  env->GetByteArrayRegion(buffer, 0, size, safe_ptr_cast<jbyte*>(&content->front()));
  const Binary::Container::Ptr data = Binary::CreateContainer(content);
  return Module::Create(data);
}


JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1GetDuration
  (JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
{
  if (const ZXTune::Module::Holder::Ptr module = Module::Storage::Instance().Get(moduleHandle))
  {
    return module->GetModuleInformation()->FramesCount();
  }
  return -1;
}

JNIEXPORT jlong JNICALL Java_app_zxtune_ZXTune_Module_1GetProperty__ILjava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jint moduleHandle, jstring propName, jlong defVal)
{
  if (const ZXTune::Module::Holder::Ptr module = Module::Storage::Instance().Get(moduleHandle))
  {
    const Parameters::Accessor::Ptr params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT jstring JNICALL Java_app_zxtune_ZXTune_Module_1GetProperty__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jint moduleHandle, jstring propName, jstring defVal)
{
  if (const ZXTune::Module::Holder::Ptr module = Module::Storage::Instance().Get(moduleHandle))
  {
    const Parameters::Accessor::Ptr params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1CreatePlayer
  (JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
{
  Dbg("Module::CreatePlayer(handle=%x)", moduleHandle);
  if (const ZXTune::Module::Holder::Ptr module = Module::Storage::Instance().Get(moduleHandle))
  {
    return Player::Create(module);
  }
  return 0;
}
