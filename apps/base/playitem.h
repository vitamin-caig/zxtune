/*
Abstract:
  Playitems support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BASE_PLAYITEM_H_DEFINED
#define BASE_PLAYITEM_H_DEFINED

//library includes
#include <core/module_holder.h>

Parameters::Accessor::Ptr CreatePathProperties(const String& path, const String& subpath);

ZXTune::Module::Information::Ptr CreateMixinPropertiesInformation(ZXTune::Module::Information::Ptr info, Parameters::Accessor::Ptr mixinProps);

ZXTune::Module::Holder::Ptr CreateMixinPropertiesModule(ZXTune::Module::Holder::Ptr module,
                                                        Parameters::Accessor::Ptr moduleProps, Parameters::Accessor::Ptr playerProps);

#endif //BASE_PLAYITEM_H_DEFINED
