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
#include "exception.h"
#include "global_options.h"
#include "jni_module.h"
#include "module.h"
#include "player.h"
#include "properties.h"
//common includes
#include <contract.h>
//library includes
#include <binary/container_factories.h>
#include <core/module_open.h>
#include <core/module_detect.h>
#include <module/additional_files.h>

namespace
{
  class NativeModuleJni
  {
  public:
    static void Init(JNIEnv* env)
    {
      const auto tmpClass = env->FindClass("app/zxtune/core/jni/JniModule");
      Class = static_cast<jclass>(env->NewGlobalRef(tmpClass));
      Require(Class);
      Constructor = env->GetMethodID(Class, "<init>", "(I)V");
      Require(Constructor);
      Handle = env->GetFieldID(Class, "handle", "I");
      Require(Handle);
    }

    static void Cleanup(JNIEnv* env)
    {
      env->DeleteGlobalRef(Class);
      Class = nullptr;
      Constructor = 0;
      Handle = 0;
    }

    static Module::Storage::HandleType GetHandle(JNIEnv* env, jobject self)
    {
      return env->GetIntField(self, Handle);
    }

    static jobject Create(JNIEnv* env, Module::Storage::HandleType handle)
    {
      const auto res = env->NewObject(Class, Constructor, handle);
      Jni::ThrowIfError(env);
      return res;
    }

  private:
    static jclass Class;
    static jmethodID Constructor;
    static jfieldID Handle;
  };

  jclass NativeModuleJni::Class;
  jmethodID NativeModuleJni::Constructor;
  jfieldID NativeModuleJni::Handle;
}

namespace
{
  Module::Holder::Ptr CreateModule(Binary::Container::Ptr data, const String& subpath)
  {
    const auto& options = Parameters::GlobalOptions();
    return subpath.empty()
      ? Module::Open(options, *data)
      : Module::Open(options, data, subpath);
  }

  jobject CreateJniObject(JNIEnv* env, Module::Holder::Ptr module)
  {
    const auto handle = Module::Storage::Instance().Add(std::move(module));
    return NativeModuleJni::Create(env, handle);
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
      const Jni::TempJString subpath(Env, location->GetPath()->AsString());
      const auto object = CreateJniObject(Env, std::move(holder));
      Env->CallVoidMethod(Delegate, GetMethodId(), subpath.Get(), object);
      Jni::ThrowIfError(Env);
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
        OnModuleMethod = Env->GetMethodID(CallbackClass, "onModule", "(Ljava/lang/String;Lapp/zxtune/core/Module;)V");
      }
      return OnModuleMethod;
    }
  private:
    JNIEnv* const Env;
    const jobject Delegate;
    mutable jclass CallbackClass;
    mutable jmethodID OnModuleMethod;
  };

  Binary::Container::Ptr CreateContainer(JNIEnv* env, jobject buffer)
  {
    const auto addr = env->GetDirectBufferAddress(buffer);
    const auto capacity = env->GetDirectBufferCapacity(buffer);
    Jni::CheckArgument(capacity && addr, "Empty buffer");
    return Binary::CreateNonCopyContainer(addr, capacity);
  }
}

namespace Module
{
  void InitJni(JNIEnv* env)
  {
    NativeModuleJni::Init(env);
  }

  void CleanupJni(JNIEnv* env)
  {
    NativeModuleJni::Cleanup(env);
  }
}

JNIEXPORT jobject JNICALL Java_app_zxtune_core_jni_JniModule_load
  (JNIEnv* env, jclass /*self*/, jobject buffer, jstring subpath)
{
  return Jni::Call(env, [=] ()
  {
    auto module = CreateModule(CreateContainer(env, buffer), Jni::MakeString(env, subpath));
    return CreateJniObject(env, std::move(module));
  });
}

JNIEXPORT void JNICALL Java_app_zxtune_core_jni_JniModule_detect
  (JNIEnv* env, jclass /*self*/, jobject buffer, jobject cb)
{
  return Jni::Call(env, [=] ()
  {
    DetectCallback callbackAdapter(env, cb);
    Module::Detect(Parameters::GlobalOptions(), CreateContainer(env, buffer), callbackAdapter);
  });
}


JNIEXPORT void JNICALL Java_app_zxtune_core_jni_JniModule_close
  (JNIEnv* /*env*/, jclass /*self*/, jint handle)
{
  if (Module::Storage::Instance().Fetch(handle))
  {
    Dbg("Module::Close(handle=%1%)", handle);
  }
}

JNIEXPORT jint JNICALL Java_app_zxtune_core_jni_JniModule_getDuration
  (JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    return Module::Storage::Instance().Get(moduleHandle)->GetModuleInformation()->FramesCount();
  });
}

JNIEXPORT jlong JNICALL Java_app_zxtune_core_jni_JniModule_getProperty__Ljava_lang_String_2J
  (JNIEnv* env, jobject self, jstring propName, jlong defVal)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto& module = Module::Storage::Instance().Get(moduleHandle);
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  });
}

JNIEXPORT jstring JNICALL Java_app_zxtune_core_jni_JniModule_getProperty__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jobject self, jstring propName, jstring defVal)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto& module = Module::Storage::Instance().Get(moduleHandle);
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  });
}

JNIEXPORT jobject JNICALL Java_app_zxtune_core_jni_JniModule_createPlayer
  (JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto& module = Module::Storage::Instance().Get(moduleHandle);
    return Player::Create(env, module);
  });
}

JNIEXPORT jobjectArray JNICALL Java_app_zxtune_core_jni_JniModule_getAdditionalFiles
  (JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto& module = Module::Storage::Instance().Get(moduleHandle);
    if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(module.get()))
    {
      const auto& filenames = files->Enumerate();
      if (const auto count = filenames.size())
      {
        const auto result = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
        for (std::size_t i = 0; i < count; ++i)
        {
          env->SetObjectArrayElement(result, i, Jni::MakeJstring(env, filenames[i]));
        }
        return result;
      }
    }
    return jobjectArray(nullptr);
  });
}

JNIEXPORT void JNICALL Java_app_zxtune_core_jni_JniModule_resolveAdditionalFile
  (JNIEnv* env, jobject self, jstring fileName, jobject data)
{
  return Jni::Call(env, [=] ()
  {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto& module = Module::Storage::Instance().Get(moduleHandle);
    auto& files = const_cast<Module::AdditionalFiles&>(dynamic_cast<const Module::AdditionalFiles&>(*module));
    files.Resolve(Jni::MakeString(env, fileName), CreateContainer(env, data));
  });
}
