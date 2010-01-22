/**
*
* @file     module_types.h
* @brief    Modules types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_TYPES_H_DEFINED__
#define __CORE_MODULE_TYPES_H_DEFINED__

#include <parameters.h>

namespace ZXTune
{
  namespace Module
  {
    //! @brief Track position or informational descriptor. All numers are 0-based
    struct Tracking
    {
      Tracking() : Position(), Pattern(), Line(), Frame(), Tempo(), Channels()
      {
      }
      //! Position in order list or total positions count
      unsigned Position;
      //! Current pattern or total patterns count
      unsigned Pattern;
      //! Current line
      unsigned Line;
      //! Current frame or total frames count
      unsigned Frame;
      //! Current tempo or initial tempo
      unsigned Tempo;
      //! Currently active channels or total channels count (logical)
      unsigned Channels;
    };

    //! @brief Common module information
    struct Information
    {
      Information() : LoopPosition(), LoopFrame(), PhysicalChannels()
      {
      }
      //! Tracking statistic (values are used in second meaning)
      Tracking Statistic;
      //! Loop position index
      unsigned LoopPosition;
      //! Loop position frame
      unsigned LoopFrame;
      //! Actual physical channels for playback
      unsigned PhysicalChannels;
      //! Different parameters
      Parameters::Map Properties;
    };

    //! @brief %Sound analyzing types
    namespace Analyze
    {
      //! @brief Level type
      typedef uint8_t LevelType;

      //! @brief Logical channel runtime characteristics
      struct Channel
      {
        Channel() : Enabled(), Level(), Band()
        {
        }
        //! Is channel enabled now
        bool Enabled;
        //! Current level
        LevelType Level;
        //! Tone band (in halftones)
        unsigned Band;
      };

      //! @brief All channels' state
      typedef std::vector<Channel> ChannelsState;
    }
  }
}

#endif //__CORE_MODULE_TYPES_H_DEFINED__
