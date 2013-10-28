/*
Abstract:
  Global parameters implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "global_options.h"
#include "properties.h"
#include "zxtune.h"

namespace
{
  Parameters::Container::Ptr instance;
}

namespace Parameters
{
  Container::Ptr GlobalOptions()
  {
    if (!instance)
    {
      instance = Container::Create();
    }
    return instance;
  }
}

JNIEXPORT jlong JNICALL Java_app_zxtune_ZXTune_GlobalOptions_1GetProperty__Ljava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jstring propName, jlong defVal)
{
  const Parameters::Accessor::Ptr params = Parameters::GlobalOptions();
  const Jni::PropertiesReadHelper props(env, *params);
  return props.Get(propName, defVal);
}

JNIEXPORT jstring JNICALL Java_app_zxtune_ZXTune_GlobalOptions_1GetProperty__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jstring propName, jstring defVal)
{
  const Parameters::Accessor::Ptr params = Parameters::GlobalOptions();
  const Jni::PropertiesReadHelper props(env, *params);
  return props.Get(propName, defVal);
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_GlobalOptions_1SetProperty__Ljava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jstring propName, jlong value)
{
  const Parameters::Modifier::Ptr params = Parameters::GlobalOptions();
  Jni::PropertiesWriteHelper props(env, *params);
  props.Set(propName, value);
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_GlobalOptions_1SetProperty__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jstring propName, jstring value)
{
  const Parameters::Modifier::Ptr params = Parameters::GlobalOptions();
  Jni::PropertiesWriteHelper props(env, *params);
  props.Set(propName, value);
}
