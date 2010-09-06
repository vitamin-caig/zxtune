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

ZXTune::Module::Information::Ptr CreateMergedInformation(ZXTune::Module::Holder::Ptr module, const String& path, const String& uri);

#endif //BASE_PLAYITEM_H_DEFINED
