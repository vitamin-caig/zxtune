/**
*
* @file     core/module_types.h
* @brief    Modules types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
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
      //! Current active channels count (up to Information::Channels)
      virtual uint_t Channels() const = 0;
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
      //! Channels count
      virtual uint_t ChannelsCount() const = 0;
      //! Initial tempo
      virtual uint_t Tempo() const = 0;
    };

    //! @brief %Sound analyzer interface
    class Analyzer
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const Analyzer> Ptr;

      virtual ~Analyzer() {}

      struct ChannelState
      {
        bool Enabled;
        uint_t Band;
        uint_t Level;
      };

      virtual void GetState(std::vector<ChannelState>& channels) const = 0;
    };
  }
}

#endif //__CORE_MODULE_TYPES_H_DEFINED__
