/*
Abstract:
  Module operating functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "data.h"
#include "debug.h"
#include "module.h"
#include "properties.h"
#include "zxtune.h"
//library includes
#include <core/module_detect.h>

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1Create
  (JNIEnv* /*env*/, jclass /*self*/, jint dataHandle)
{
  Dbg("Module::Create(handle=%x)", dataHandle);
  if (const Binary::Container::Ptr data = Data::Storage::Instance().Get(dataHandle))
  {
    Dbg("Module::Create(data=%p)", data.get());
    const Parameters::Accessor::Ptr params = Parameters::Container::Create();
    const ZXTune::Module::Holder::Ptr module = ZXTune::OpenModule(params, data, String());
    Dbg("Module::Create(data=%p)=%p", data.get(), module.get());
    return Module::Storage::Instance().Add(module);
  }
  return 0;
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Module_1Destroy
  (JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
{
  Module::Storage::Instance().Fetch(moduleHandle);
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_ModuleInfo_1GetFramesCount
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
