/**
 *
 * @file
 *
 * @brief Plugins operating
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "jni_api.h"
#include "properties.h"
// library includes
#include <core/plugin.h>
#include <core/plugin_attrs.h>

namespace
{
  class NativePluginJni
  {
  public:
    static void Init(JNIEnv* env)
    {
      const jclass classType = env->FindClass("app/zxtune/core/jni/Plugins$Visitor");
      OnPlayerPlugin = env->GetMethodID(classType, "onPlayerPlugin", "(ILjava/lang/String;Ljava/lang/String;)V");
      OnContainerPlugin = env->GetMethodID(classType, "onContainerPlugin", "(ILjava/lang/String;Ljava/lang/String;)V");
    }

    static void Cleanup(JNIEnv* /*env*/)
    {
      OnPlayerPlugin = OnContainerPlugin = 0;
    }

    static void CallOnPlayerPlugin(JNIEnv* env, jobject delegate, jint type, const String& plugId,
                                   const String& plugDescr)
    {
      const Jni::TempJString id(env, plugId);
      const Jni::TempJString descr(env, plugDescr);
      env->CallVoidMethod(delegate, OnPlayerPlugin, type, id.Get(), descr.Get());
    }

    static void CallOnContainerPlugin(JNIEnv* env, jobject delegate, jint type, const String& plugId,
                                      const String& plugDescr)
    {
      const Jni::TempJString id(env, plugId);
      const Jni::TempJString descr(env, plugDescr);
      env->CallVoidMethod(delegate, OnContainerPlugin, type, id.Get(), descr.Get());
    }

  private:
    static jmethodID OnPlayerPlugin;
    static jmethodID OnContainerPlugin;
  };

  jmethodID NativePluginJni::OnPlayerPlugin;
  jmethodID NativePluginJni::OnContainerPlugin;
}  // namespace

namespace Plugin
{
  void InitJni(JNIEnv* env)
  {
    NativePluginJni::Init(env);
  }

  void CleanupJni(JNIEnv* env)
  {
    NativePluginJni::Cleanup(env);
  }
}  // namespace Plugin

JNIEXPORT void JNICALL Java_app_zxtune_core_jni_JniApi_enumeratePlugins(JNIEnv* env, jobject /*self*/, jobject visitor)
{
  for (const auto iter = ZXTune::EnumeratePlugins(); iter->IsValid(); iter->Next())
  {
    const ZXTune::Plugin::Ptr plug = iter->Get();
    const uint_t caps = plug->Capabilities();
    const String id = plug->Id();
    const String desc = plug->Description();

    // TODO: remove hardcode
    using namespace ZXTune::Capabilities;
    switch (caps & Category::MASK)
    {
    case Category::MODULE:
      NativePluginJni::CallOnPlayerPlugin(env, visitor, (caps & Module::Device::MASK) >> 4, id, desc);
      break;
    case Category::CONTAINER:
      NativePluginJni::CallOnContainerPlugin(env, visitor, (caps & Container::Type::MASK) >> 0, id, desc);
      break;
    default:
      break;
    }
  }
}
