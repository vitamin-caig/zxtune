/**
 *
 * @file
 *
 * @brief Module access implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-android/zxtune/src/main/jni/module.h"

#include "apps/zxtune-android/zxtune/src/main/jni/array.h"
#include "apps/zxtune-android/zxtune/src/main/jni/binary.h"
#include "apps/zxtune-android/zxtune/src/main/jni/debug.h"
#include "apps/zxtune-android/zxtune/src/main/jni/defines.h"
#include "apps/zxtune-android/zxtune/src/main/jni/exception.h"
#include "apps/zxtune-android/zxtune/src/main/jni/global_options.h"
#include "apps/zxtune-android/zxtune/src/main/jni/player.h"
#include "apps/zxtune-android/zxtune/src/main/jni/properties.h"

#include "analysis/path.h"
#include "binary/container.h"
#include "binary/view.h"
#include "core/data_location.h"
#include "core/module_detect.h"
#include "core/service.h"
#include "debug/log.h"
#include "module/additional_files.h"
#include "module/information.h"
#include "parameters/container.h"
#include "time/duration.h"
#include "tools/progress_callback.h"

#include "contract.h"
#include "error.h"
#include "pointers.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include <jni.h>

#include <cstring>
#include <exception>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace ZXTune
{
  class Plugin;
}  // namespace ZXTune

namespace
{
  // #define Require(c) while (! (c)) { throw std::runtime_error( #c ); }

  class NativeModuleJni
  {
  public:
    static void Init(JNIEnv* env)
    {
      auto* const tmpClass = env->FindClass("app/zxtune/core/jni/JniModule");
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
      auto* const res = env->NewObject(Class, Constructor, handle);
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

  class CallbacksJni
  {
  public:
    static void Init(JNIEnv* env)
    {
      auto* const detectClass = env->FindClass("app/zxtune/core/jni/DetectCallback");
      Require(detectClass);
      OnModule = env->GetMethodID(detectClass, "onModule", "(Ljava/lang/String;Lapp/zxtune/core/Module;)V");
      Require(OnModule);
      OnPicture = env->GetMethodID(detectClass, "onPicture", "(Ljava/lang/String;[B)V");
      Require(OnPicture);
      auto* const progressClass = env->FindClass("app/zxtune/utils/ProgressCallback");
      Require(progressClass);
      OnProgress = env->GetMethodID(progressClass, "onProgressUpdate", "(II)V");
      Require(OnProgress);
      auto* const dataClass = env->FindClass("app/zxtune/core/jni/DataCallback");
      Require(dataClass);
      OnData = env->GetMethodID(dataClass, "onData", "(Ljava/nio/ByteBuffer;)V");
      Require(OnData);
    }

    static void Cleanup(JNIEnv* /*env*/)
    {
      OnModule = 0;
      OnProgress = 0;
      OnData = 0;
    }

    static void CallOnModule(JNIEnv* env, jobject cb, const String& subpath, jobject module)
    {
      const Jni::TempJString tmpSubpath(env, subpath);
      env->CallVoidMethod(cb, OnModule, tmpSubpath.Get(), module);
      Jni::ThrowIfError(env);
    }

    static void CallOnPicture(JNIEnv* env, jobject cb, const String& subpath, jbyteArray buffer)
    {
      const Jni::TempJString tmpSubpath(env, subpath);
      env->CallVoidMethod(cb, OnPicture, tmpSubpath.Get(), buffer);
      Jni::ThrowIfError(env);
    }

    static void CallOnProgress(JNIEnv* env, jobject cb, int progress)
    {
      env->CallVoidMethod(cb, OnProgress, progress, 100);
      Jni::ThrowIfError(env);
    }

    static void CallOnData(JNIEnv* env, jobject cb, Binary::View data)
    {
      const auto buf = env->NewDirectByteBuffer(const_cast<void*>(data.Start()), data.Size());
      env->CallVoidMethod(cb, OnData, buf);
      Jni::ThrowIfError(env);
    }

  private:
    static jmethodID OnModule;
    static jmethodID OnPicture;
    static jmethodID OnProgress;
    static jmethodID OnData;
  };

  jmethodID CallbacksJni::OnModule;
  jmethodID CallbacksJni::OnPicture;
  jmethodID CallbacksJni::OnProgress;
  jmethodID CallbacksJni::OnData;

#undef Require
  const ZXTune::Service& GetService()
  {
    // TODO: cleanup
    static const auto instance = ZXTune::Service::Create(MakeSingletonPointer(Parameters::GlobalOptions()));
    return *instance;
  }

  bool IsPng(Binary::View data)
  {
    static const uint8_t HEADER[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return data.Size() > std::size(HEADER) && 0 == std::memcmp(data.Start(), HEADER, std::size(HEADER));
  }

  bool IsJpeg(Binary::View data)
  {
    static const uint8_t SIGNATURE[] = {0xff, 0xd8, 0xff};
    return data.Size() > std::size(SIGNATURE) && 0 == std::memcmp(data.Start(), SIGNATURE, std::size(SIGNATURE));
  }

  bool IsPictureFormat(Binary::View data)
  {
    return IsPng(data) || IsJpeg(data);
  }
}  // namespace

namespace
{
  Module::Holder::Ptr CreateModule(Binary::Container::Ptr data, StringView subpath)
  {
    try
    {
      auto initialProperties = Parameters::Container::Create();
      return GetService().OpenModule(std::move(data), subpath, std::move(initialProperties));
    }
    catch (const Error& e)
    {
      throw Jni::ResolvingException(e.GetText());
    }
  }

  jobject CreateJniObject(JNIEnv* env, Module::Holder::Ptr module)
  {
    const auto handle = Module::Storage::Instance().Add(std::move(module));
    return NativeModuleJni::Create(env, handle);
  }

  class ProgressCallback : public Log::ProgressCallback
  {
  public:
    ProgressCallback(JNIEnv* env, jobject delegate)
      : Env(env)
      , Delegate(delegate)
    {}

    Log::ProgressCallback* Get()
    {
      return Delegate ? this : nullptr;
    }

    void OnProgress(uint_t current) override
    {
      if (LastProgress != current)
      {
        CallbacksJni::CallOnProgress(Env, Delegate, LastProgress = current);
      }
    }

    void OnProgress(uint_t current, StringView) override
    {
      OnProgress(current);
    }

  private:
    JNIEnv* const Env;
    const jobject Delegate;
    uint_t LastProgress = 0;
  };

  class DetectCallback : public Module::DetectCallback
  {
  public:
    DetectCallback(JNIEnv* env, jobject delegate, Log::ProgressCallback* log)
      : Env(env)
      , Delegate(delegate)
      , Log(log)
    {}

    Parameters::Container::Ptr CreateInitialProperties(StringView /*subpath*/) const override
    {
      return Parameters::Container::Create();
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& /*decoder*/,
                       Module::Holder::Ptr holder) override
    {
      auto* const object = CreateJniObject(Env, std::move(holder));
      CallbacksJni::CallOnModule(Env, Delegate, location.GetPath()->AsString(), object);
    }

    void ProcessUnknownData(const ZXTune::DataLocation& location) override
    {
      const auto rawData = location.GetData();
      const auto data = Binary::View(*rawData);
      const std::size_t MAX_PICTURE_SIZE = 1048576 * 2;  // 2Mb
      if (data.Size() <= MAX_PICTURE_SIZE && IsPictureFormat(data))
      {
        CallbacksJni::CallOnPicture(Env, Delegate, location.GetPath()->AsString(), Jni::MakeByteArray(Env, data));
      }
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Log;
    }

  private:
    JNIEnv* const Env;
    const jobject Delegate;
    Log::ProgressCallback* const Log;
  };
}  // namespace

namespace Module
{
  void InitJni(JNIEnv* env)
  {
    NativeModuleJni::Init(env);
    CallbacksJni::Init(env);
  }

  void CleanupJni(JNIEnv* env)
  {
    CallbacksJni::Cleanup(env);
    NativeModuleJni::Cleanup(env);
  }
}  // namespace Module

EXPORTED jobject JNICALL Java_app_zxtune_core_jni_JniApi_loadModule(JNIEnv* env, jobject /*self*/, jobject buffer,
                                                                    jstring subpath)
{
  return Jni::Call(env, [=]() {
    auto module = CreateModule(Binary::CreateByteBufferContainer(env, buffer), Jni::JstringView(env, subpath));
    return CreateJniObject(env, std::move(module));
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniApi_loadModuleData(JNIEnv* env, jobject /*self*/, jobject buffer,
                                                                     jstring subpath, jobject cb)
{
  return Jni::Call(env, [=]() {
    try
    {
      const auto data =
          GetService().OpenData(Binary::CreateByteBufferContainer(env, buffer), Jni::JstringView(env, subpath));
      return CallbacksJni::CallOnData(env, cb, *data);
    }
    catch (const Error& e)
    {
      throw Jni::ResolvingException(e.GetText());
    }
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniApi_detectModules(JNIEnv* env, jobject /*self*/, jobject buffer,
                                                                    jobject cb, jobject progress)
{
  return Jni::Call(env, [=]() {
    ProgressCallback progressAdapter(env, progress);
    DetectCallback callbackAdapter(env, cb, progressAdapter.Get());
    GetService().DetectModules(Binary::CreateByteBufferContainer(env, buffer), callbackAdapter);
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniModule_close(JNIEnv* /*env*/, jclass /*self*/, jint handle)
{
  if (Module::Storage::Instance().Fetch(handle))
  {
    Dbg("Module::Close(handle={})", handle);
  }
}

EXPORTED jint JNICALL Java_app_zxtune_core_jni_JniModule_getDurationMs(JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=]() {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    return Module::Storage::Instance()
        .Get(moduleHandle)
        ->GetModuleInformation()
        ->Duration()
        .CastTo<Player::TimeBase>()
        .Get();
  });
}

EXPORTED jlong JNICALL Java_app_zxtune_core_jni_JniModule_getProperty__Ljava_lang_String_2J(JNIEnv* env, jobject self,
                                                                                            jstring propName,
                                                                                            jlong defVal)
{
  return Jni::Call(env, [=] {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  });
}

EXPORTED jstring JNICALL Java_app_zxtune_core_jni_JniModule_getProperty__Ljava_lang_String_2Ljava_lang_String_2(
    JNIEnv* env, jobject self, jstring propName, jstring defVal)
{
  return Jni::Call(env, [=] {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  });
}

EXPORTED jbyteArray JNICALL Java_app_zxtune_core_jni_JniModule_getProperty__Ljava_lang_String_2_3B(JNIEnv* env,
                                                                                                   jobject self,
                                                                                                   jstring propName,
                                                                                                   jbyteArray defVal)
{
  return Jni::Call(env, [=] {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    const auto& params = module->GetModuleProperties();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  });
}

EXPORTED jobject JNICALL Java_app_zxtune_core_jni_JniModule_createPlayer(JNIEnv* env, jobject self, jint samplerate)
{
  return Jni::Call(env, [=]() {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    return Player::Create(env, *module, samplerate);
  });
}

EXPORTED jobjectArray JNICALL Java_app_zxtune_core_jni_JniModule_getAdditionalFiles(JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=]() {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    if (const auto* const files = dynamic_cast<const Module::AdditionalFiles*>(module.get()))
    {
      const auto& filenames = files->Enumerate();
      if (const auto count = filenames.size())
      {
        auto* const result = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
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

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniModule_resolveAdditionalFile(JNIEnv* env, jobject self,
                                                                               jstring fileName, jobject data)
{
  return Jni::Call(env, [=]() {
    const auto moduleHandle = NativeModuleJni::GetHandle(env, self);
    const auto module = Module::Storage::Instance().Get(moduleHandle);
    auto& files = const_cast<Module::AdditionalFiles&>(dynamic_cast<const Module::AdditionalFiles&>(*module));
    files.Resolve(Jni::JstringView(env, fileName), Binary::CreateByteBufferContainer(env, data));
  });
}
