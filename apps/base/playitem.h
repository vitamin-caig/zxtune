/*
Abstract:
  Playitems support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef BASE_PLAYITEM_H_DEFINED
#define BASE_PLAYITEM_H_DEFINED

//library includes
#include <core/module_holder.h>
#include <io/identifier.h>

Parameters::Accessor::Ptr CreatePathProperties(const String& path, const String& subpath);
Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath);
Parameters::Accessor::Ptr CreatePathProperties(ZXTune::IO::Identifier::Ptr id);

#endif //BASE_PLAYITEM_H_DEFINED
