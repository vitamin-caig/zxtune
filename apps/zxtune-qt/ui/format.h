/*
Abstract:
  Formatting tools  

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_FORMAT_H_DEFINED
#define ZXTUNE_QT_FORMAT_H_DEFINED

//common includes
#include <types.h>

namespace ZXTune
{
  namespace Module
  {
    struct Information;
  }
}

String GetModuleTitle(const String& format,
  const ZXTune::Module::Information& info);

String GetProgramTitle();

#endif //ZXTUNE_QT_FORMAT_H_DEFINED
