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
//boost includes
#include <boost/shared_ptr.hpp>

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

    //! @brief Runtime module track status
    class TrackState
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const TrackState> Ptr;

      virtual ~TrackState() {}

      //! Current position (up to Information::PositionsCount)
      virtual uint_t Position() const = 0;
      //! Current pattern (up to Information::PatternsCount)
      virtual uint_t Pattern() const = 0;
      //! Current pattern size
      virtual uint_t PatternSize() const = 0;
      //! Current line in pattern (up to Status::PatternSize)
      virtual uint_t Line() const = 0;
      //! Current tempo
      virtual uint_t Tempo() const = 0;
      //! Current quirk in line (up to Status::Tempo)
      virtual uint_t Quirk() const = 0;
      //! Current frame in track (up to Information::FramesCount)
      virtual uint_t Frame() const = 0;
      //! Current active channels count (up to Information::LogicalChannels)
      virtual uint_t Channels() const = 0;
      //! Absolute frame played till startup
      virtual uint_t AbsoluteFrame() const = 0;
      //! Absolute tick played till startup
      virtual uint64_t AbsoluteTick() const = 0;
    };

    //! @brief Common module information
    class Information
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const Information> Ptr;

      virtual ~Information() {}

      //! Total positions
      virtual uint_t PositionsCount() const = 0;
      //! Loop position index
      virtual uint_t LoopPosition() const = 0;
      //! Total patterns count
      virtual uint_t PatternsCount() const = 0;
      //! Total frames count
      virtual uint_t FramesCount() const = 0;
      //! Loop position frame
      virtual uint_t LoopFrame() const = 0;
      //! Logical channels count
      virtual uint_t LogicalChannels() const = 0;
      //! Actual physical channels count
      virtual uint_t PhysicalChannels() const = 0;
      //! Initial tempo
      virtual uint_t Tempo() const = 0;
      //! %Module properties @see core/module_attrs.h
      virtual Parameters::Accessor::Ptr Properties() const = 0;
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
