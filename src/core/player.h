/*
Abstract:
  Modules player interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PLAYER_H_DEFINED__
#define __CORE_PLAYER_H_DEFINED__

#include "module_types.h"

#include <io/container.h>
#include <sound/receiver.h>

#include <error.h>

#include <boost/function.hpp>

namespace ZXTune
{
  //forward declarations
  struct PluginInformation;
  
  namespace Sound
  {
    struct RenderParameters;
  }
  
  namespace Module
  {
    namespace Conversion
    {
      struct Parameter;
    }
    
    /// Player interface
    class Player
    {
    public:
      typedef std::auto_ptr<Player> Ptr;

      virtual ~Player() {}

      /// Module playing state
      enum PlaybackState
      {
        MODULE_PLAYING,
        MODULE_STOPPED
      };

      /// Retrieving player info itself
      virtual void GetPlayerInfo(PluginInformation& info) const = 0;

      /// Retrieving information about loaded module
      virtual void GetModuleInformation(Information& info) const = 0;

      /// Retrieving current state of loaded module
      virtual Error GetModuleState(unsigned& timeState, //current frame
                                   Tracking& trackState, //current track position
                                   Analyze::ChannelsState& analyzeState //current analyzed state
                                   ) const = 0;

      /// Rendering frame
      virtual Error RenderFrame(const Sound::RenderParameters& params, //parameters for rendering
                                PlaybackState& state, //playback state
                                Sound::MultichannelReceiver& receiver //sound stream reciever
                                ) = 0;

      /// Controlling
      virtual Error Reset() = 0;
      virtual Error SetPosition(unsigned frame) = 0;

      /// Converting
      virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const = 0;
    };
  }

  /// Creating
  struct DetectParameters
  {
    typedef boost::function<bool(const PluginInformation&)> FilterFunc;
    FilterFunc Filter;
    typedef boost::function<Error(Module::Player::Ptr player)> CallbackFunc;
    CallbackFunc Callback;
    typedef boost::function<void(const String&)> LogFunc;
    LogFunc Logger;
  };
  
  Error DetectModules(IO::DataContainer::Ptr data, const DetectParameters& params, const String& startSubpath);
}

#endif //__CORE_PLAYER_H_DEFINED__
