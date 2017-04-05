/**
* 
* @file
*
* @brief Module access implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "debug.h"
#include "global_options.h"
#include "module.h"
#include "player.h"
#include "properties.h"
#include "zxtune.h"
//common includes
#include <error.h>
//library includes
#include <binary/container_factories.h>
#include <core/module_open.h>
#include <core/module_detect.h>

namespace
{
  Module::Storage::HandleType CreateModule(Binary::Container::Ptr data, const String& subpath)
  {
    try
    {
      const Parameters::Accessor::Ptr options = Parameters::GlobalOptions();
      auto module = subpath.empty()
          ? Module::Open(*options, *data)
          : Module::Open(*options, data, subpath);
      Dbg("Module::Create(data=%p, subpath=%s)=%p", data.get(), subpath, module.get());
      return Module::Storage::Instance().Add(std::move(module));
    }
    catch (const Error&)
    {
      return 0;
    }
  }

  class DetectCallback : public Module::DetectCallback
  {
  public:
    explicit DetectCallback(JNIEnv* env, jobject delegate)
      : Env(env)
      , Delegate(delegate)
      , CallbackClass()
      , OnModuleMethod()
    {
    }

    void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr /*decoder*/,
      Module::Holder::Ptr holder) const override
    {
      const jmethodID methodId = GetMethodId();
      const Jni::TempJString subpath(Env, location->GetPath()->AsString());
      const int handle = Module::Storage::Instance().Add(std::move(holder));
      Env->CallNonvirtualVoidMethod(Delegate, CallbackClass, methodId, subpath.Get(), handle);
      CheckException();
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return nullptr;
    }
  private:
    jmethodID GetMethodId() const
    {
      if (!OnModuleMethod)
      {
        CallbackClass = Env->GetObjectClass(Delegate);
        OnModuleMethod = Env->GetMethodID(CallbackClass, "onModule", "(Ljava/lang/String;I)V");
      }
      return OnModuleMethod;
    }

    void CheckException() const
    {
      if (const jthrowable e = Env->ExceptionOccurred())
      {
        Env->ExceptionDescribe();
        throw e;
      }
    }
  private:
    JNIEnv* const Env;
    const jobject Delegate;
    mutable jclass CallbackClass;
    mutable jmethodID OnModuleMethod;
  };

  void DetectModules(Binary::Container::Ptr data, Module::DetectCallback& cb)
  {
    Module::Detect(*Parameters::GlobalOptions(), std::move(data), cb);
  }
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1Create
  (JNIEnv* env, jclass /*self*/, jobject buffer, jstring subpath)
{
  const jlong capacity = env->GetDirectBufferCapacity(buffer);
  const void* addr = env->GetDirectBufferAddress(buffer);
  if (capacity && addr)
  {
    return CreateModule(Binary::CreateNonCopyContainer(addr, capacity), Jni::MakeString(env, subpath));
  }
  else
  {
    return 0;
  }
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Module_1Detect
  (JNIEnv* env, jclass /*self*/, jobject buffer, jobject cb)
{
  const jlong capacity = env->GetDirectBufferCapacity(buffer);
  const void* addr = env->GetDirectBufferAddress(buffer);
  if (capacity && addr)
  {
    DetectCallback callbackAdapter(env, cb);
    try
    {
      DetectModules(Binary::CreateNonCopyContainer(addr, capacity), callbackAdapter);
    }
    catch (jthrowable e)
    {
      env->ExceptionClear();
      env->Throw(e);
    }
  }
}


JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1GetDuration
  (JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
{
  if (const auto& module = Module::Storage::Instance().Get(moduleHandle))
  {
    return module->GetModuleInformation()->FramesCount();
  }
  return -1;
}

JNIEXPORT jlong JNICALL Java_app_zxtune_ZXTune_Module_1GetProperty__ILjava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jint moduleHandle, jstring propName, jlong defVal)
{
  if (const auto& module = Module::Storage::Instance().Get(moduleHandle))
  {
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT jstring JNICALL Java_app_zxtune_ZXTune_Module_1GetProperty__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jint moduleHandle, jstring propName, jstring defVal)
{
  if (const auto& module = Module::Storage::Instance().Get(moduleHandle))
  {
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Module_1CreatePlayer
  (JNIEnv* /*env*/, jclass /*self*/, jint moduleHandle)
{
  Dbg("Module::CreatePlayer(handle=%x)", moduleHandle);
  if (const auto& module = Module::Storage::Instance().Get(moduleHandle))
  {
    return Player::Create(module);
  }
  return 0;
}
