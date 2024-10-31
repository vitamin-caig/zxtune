/**
 *
 * @file
 *
 * @brief Plugins operating
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "defines.h"
#include "properties.h"

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

  class Visitor : public ZXTune::PluginVisitor
  {
  public:
    Visitor(JNIEnv* env, jobject delegate)
      : Env(env)
      , Delegate(delegate)
    {}

    void Visit(const ZXTune::Plugin& plug) override
    {
      const auto caps = plug.Capabilities();
      const auto id = String{plug.Id()};
      const auto desc = String{plug.Description()};

      // TODO: remove hardcode
      using namespace ZXTune::Capabilities;
      switch (caps & Category::MASK)
      {
      case Category::MODULE:
        NativePluginJni::CallOnPlayerPlugin(Env, Delegate, (caps & Module::Device::MASK) >> 4, id, desc);
        break;
      case Category::CONTAINER:
        NativePluginJni::CallOnContainerPlugin(Env, Delegate, (caps & Container::Type::MASK) >> 0, id, desc);
        break;
      default:
        break;
      }
    }

  private:
    JNIEnv* const Env;
    const jobject Delegate;
  };
}  // namespace Plugin

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniApi_enumeratePlugins(JNIEnv* env, jobject /*self*/, jobject visitor)
{
  Plugin::Visitor adapter(env, visitor);
  ZXTune::EnumeratePlugins(adapter);
}
