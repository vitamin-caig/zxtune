/*
Abstract:
  Streamed modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__

//library includes
#include <core/module_types.h>
#include <sound/render_params.h>// for LoopMode

namespace ZXTune
{
  namespace Module
  {
    Information::Ptr CreateStreamInfo(uint_t frames, uint_t logChannels, uint_t physChannels,
      Parameters::Accessor::Ptr props);

    class StreamStateIterator : public TrackState
    {
    public:
      typedef boost::shared_ptr<StreamStateIterator> Ptr;

      static Ptr Create(Information::Ptr info, Analyzer::Ptr analyze);

      virtual void Reset() = 0;

      virtual void ResetPosition() = 0;

      virtual bool NextFrame(uint64_t ticksToSkip, Sound::LoopMode mode) = 0;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__
