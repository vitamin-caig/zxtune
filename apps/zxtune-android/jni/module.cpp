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
#include "module.h"
#include "zxtune.h"
//library includes
#include <core/module_detect.h>

extern "C"
{
  JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1Create(JNIEnv* /*env*/, jclass /*self*/, jint dataHandle)
  {
    Debug("Module::Create(handle=%x)", dataHandle);
    if (const Binary::Container::Ptr data = Data::Storage::Instance().Get(dataHandle))
    {
      Debug("Module::Create(data=%p)", data.get());
      const Parameters::Accessor::Ptr params = Parameters::Container::Create();
      const ZXTune::Module::Holder::Ptr module = ZXTune::OpenModule(params, data, String());
      Debug("Module::Create(data=%p)=%p", data.get(), module.get());
      return Module::Storage::Instance().Add(module);
    }
    return 0;
  }

  JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Module_1Destroy(JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
  {
    Module::Storage::Instance().Fetch(moduleHandle);
  }

  JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_ModuleInfo_1GetFramesCount(JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
  {
    if (const ZXTune::Module::Holder::Ptr module = Module::Storage::Instance().Get(moduleHandle))
    {
      return module->GetModuleInformation()->FramesCount();
    }
    return -1;
  }
}
