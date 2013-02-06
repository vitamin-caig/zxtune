/**
*
* @file      sound/filter_factory.h
* @brief     Factories for filters
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_FILTER_FACTORY_H_DEFINED
#define SOUND_FILTER_FACTORY_H_DEFINED

//library includes
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    Converter::Ptr CreateSimpleInterpolationFilter();
  }
}

#endif //SOUND_FILTER_FACTORY_H_DEFINED
