/**
 *
 * @file
 *
 * @brief Global parameters implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-android/zxtune/src/main/jni/global_options.h"

#include "apps/zxtune-android/zxtune/src/main/jni/defines.h"
#include "apps/zxtune-android/zxtune/src/main/jni/properties.h"

#include "parameters/container.h"

#include <jni.h>

#include <memory>

namespace Parameters
{
  Container& GlobalOptions()
  {
    static const Parameters::Container::Ptr instance = Parameters::Container::Create();
    return *instance;
  }
}  // namespace Parameters

EXPORTED jlong JNICALL Java_app_zxtune_core_jni_JniOptions_getProperty__Ljava_lang_String_2J(JNIEnv* env,
                                                                                             jobject /*self*/,
                                                                                             jstring propName,
                                                                                             jlong defVal)
{
  const auto& params = Parameters::GlobalOptions();
  const Jni::PropertiesReadHelper props(env, params);
  return props.Get(propName, defVal);
}

EXPORTED jstring JNICALL Java_app_zxtune_core_jni_JniOptions_getProperty__Ljava_lang_String_2Ljava_lang_String_2(
    JNIEnv* env, jobject /*self*/, jstring propName, jstring defVal)
{
  const auto& params = Parameters::GlobalOptions();
  const Jni::PropertiesReadHelper props(env, params);
  return props.Get(propName, defVal);
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniOptions_setProperty__Ljava_lang_String_2J(JNIEnv* env,
                                                                                            jobject /*self*/,
                                                                                            jstring propName,
                                                                                            jlong value)
{
  auto& params = Parameters::GlobalOptions();
  Jni::PropertiesWriteHelper props(env, params);
  props.Set(propName, value);
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniOptions_setProperty__Ljava_lang_String_2Ljava_lang_String_2(
    JNIEnv* env, jobject /*self*/, jstring propName, jstring value)
{
  auto& params = Parameters::GlobalOptions();
  Jni::PropertiesWriteHelper props(env, params);
  props.Set(propName, value);
}
