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

ZXTune::Module::Holder::Ptr CreateWrappedHolder(const String& path, const String& subpath, 
  ZXTune::Module::Holder::Ptr module);

#endif //BASE_PLAYITEM_H_DEFINED
