/*
Abstract:
  Modules types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_MODULE_TYPES_H_DEFINED__
#define __CORE_MODULE_TYPES_H_DEFINED__

#include <parameters_types.h>

namespace ZXTune
{
  namespace Module
  {
    /// Track position descriptor
    struct Tracking
    {
      Tracking() : Position(), Pattern(), Line(), Frame(), Tempo(), Channels()
      {
      }
      /// Position in order list or total positions count
      unsigned Position;
      /// Current pattern or total patterns count
      unsigned Pattern;
      /// Current line
      unsigned Line;
      /// Current frame
      unsigned Frame;
      /// Current tempo or initial tempo
      unsigned Tempo;
      /// Currently active channels or total channels count (logical)
      unsigned Channels;
    };

    /// Common module information
    struct Information
    {
      Information() : LoopPosition(), LoopFrame(), PhysicalChannels()
      {
      }
      /// Tracking statistic (values are used in second meaning)
      Tracking Statistic;
      /// Loop position index
      unsigned LoopPosition;
      /// Loop position frame
      unsigned LoopFrame;
      /// Actual channels for playback
      unsigned PhysicalChannels;
      /// Different parameters
      Parameters::Map Properties;
    };
    
    namespace Analyze
    {
      /// Level type (by default 0...255 is enough)
      typedef uint8_t LevelType;

      /// Channel voice characteristics
      struct Channel
      {
        Channel() : Enabled(), Level(), Band()
        {
        }
        bool Enabled;
        LevelType Level;
        unsigned Band;
      };

      typedef std::vector<Channel> ChannelsState;
    }
  }
}

#endif //__CORE_MODULE_TYPES_H_DEFINED__
