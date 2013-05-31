/**
*
* @file      sound/backend_attrs.h
* @brief     Backends attributes
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_BACKEND_ATTRS_H_DEFINED
#define SOUND_BACKEND_ATTRS_H_DEFINED

namespace Sound
{
  //! @brief Set of backend capabilities
  enum
  {
    //! Type-related capabilities
    CAP_TYPE_MASK = 0xff,
    //! Stub purposes
    CAP_TYPE_STUB = 1,
    //! Real sound subsystem playback
    CAP_TYPE_SYSTEM = 2,
    //! Saving to file capabilities
    CAP_TYPE_FILE = 4,
    //! Specific devices playback
    CAP_TYPE_HARDWARE = 8,

    //! Features-related capabilities
    CAP_FEAT_MASK = 0xff00,
    //! Hardware volume control
    CAP_FEAT_HWVOLUME = 0x100
  };
}

#endif //SOUND_BACKEND_ATTRS_H_DEFINED
