/**
*
* @file     core/module_types.h
* @brief    Modules types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_TYPES_H_DEFINED__
#define __CORE_MODULE_TYPES_H_DEFINED__

//common includes
#include <parameters.h>

namespace ZXTune
{
  namespace Module
  {
    //! @brief Track position descriptor. All numers are 0-based
    struct Tracking
    {
      Tracking()
        : Position()
        , Pattern()
        , Line()
        , Quirk()
        , Frame()
        , Channels()
      {
      }
      //! Position in order list/total positions count
      uint_t Position;
      //! Current pattern/total patterns count
      uint_t Pattern;
      //! Current line in pattern/total lines in pattern
      uint_t Line;
      //! Current quirk in line/total quirks in line (tempo)
      uint_t Quirk;
      //! Current frame in track/total frames in track
      uint_t Frame;
      //! Currently active channels/total channels count (logical)
      uint_t Channels;
    };

    //! @brief Runtime module state descriptor
    struct State
    {
      State() : Frame(), Tick()
      {
      }
      //! Current tracking state
      Tracking Track;
      //! Current tracking state' reference
      Tracking Reference;
      //! Current frame from beginning of playback
      uint_t Frame;
      //! Current tick from beginning of playback
      uint64_t Tick;
    };

    //! @brief Common module information
    struct Information
    {
      Information()
        : PositionsCount(), LoopPosition()
        , PatternsCount()
        , FramesCount(), LoopFrame()
        , LogicalChannels(), PhysicalChannels()
        , Tempo()
      {
      }
      //! Total positions
      uint_t PositionsCount;
      //! Loop position index
      uint_t LoopPosition;
      //! Total patterns count
      uint_t PatternsCount;
      //! Total frames count
      uint_t FramesCount;
      //! Loop position frame
      uint_t LoopFrame;
      //! Logical channels count
      uint_t LogicalChannels;
      //! Actual physical channels count
      uint_t PhysicalChannels;
      //! Initial tempo
      uint_t Tempo;
      //! %Module properties @see core/module_attrs.h
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
        uint_t Band;
      };

      //! @brief All channels' state
      typedef std::vector<Channel> ChannelsState;
    }
  }
}

#endif //__CORE_MODULE_TYPES_H_DEFINED__
