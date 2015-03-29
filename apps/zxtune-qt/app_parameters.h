/**
* 
* @file
*
* @brief Application parameters definitions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTuneQT
  {
    const NameType PREFIX("zxtune-qt");

    const NameType SINGLE_INSTANCE = PREFIX + "SingleInstance";
    const IntType SINGLE_INSTANCE_DEFAULT = 0;
  }
}
