/*
Abstract:
  Sound backend interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_BACKEND_H_DEFINED__
#define __SOUND_BACKEND_H_DEFINED__

#include "receiver.h"
#include "sound_types.h"

#include <core/player.h>

#include <error.h>
#include <types.h>

#include <memory>

#include <boost/weak_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    class Backend
    {
    public:
      typedef std::auto_ptr<Backend> Ptr;

      virtual ~Backend() {}

      // informational part
      struct Info
      {
        String Id;
        String Version;
        String Description;
      };
      virtual void GetInfo(Info& info) const = 0;

      // player manipulation functions
      virtual Error SetPlayer(Module::Player::Ptr player) = 0;
      virtual boost::weak_ptr<const Module::Player> GetPlayer() const = 0;
      
      // playback control functions
      virtual Error Play() = 0;
      virtual Error Pause() = 0;
      virtual Error Stop() = 0;
      virtual Error SetPosition(unsigned frame) = 0;
      
      // state getting function
      enum State
      {
        NOTOPENED,
        STOPPED,
        PAUSED,
        STARTED
      };
      virtual State GetCurrentState() const = 0;

      // adding/changing mixer
      virtual Error SetMixer(const std::vector<MultiGain>& data) = 0;
      // adding filter
      virtual Error SetFilter(Converter::Ptr converter) = 0;

      // driver parameters
      virtual Error SetDriverParameters(const ParametersMap& params) = 0;
      virtual Error GetDriverParameters(ParametersMap& params) const = 0;
      // rendering parameters
      virtual Error SetRenderParameters(const RenderParameters& params) = 0;
      virtual Error GetRenderParameters(RenderParameters& params) const = 0;
      
      // hardware volume control
      virtual Error GetVolume(MultiGain& volume) const = 0;
      virtual Error SetVolume(const MultiGain& volume) = 0;
          };

    //common interface
    void EnumerateBackends(std::vector<Backend::Info>& backends);
    Error CreateBackend(const String& id, Backend::Ptr& result);
  }
}

#endif //__SOUND_BACKEND_H_DEFINED__
