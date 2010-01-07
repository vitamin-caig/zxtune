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

//for typedef'ed Parameters::Map
#include <parameters.h>

//for Module::Analyze::ChannelsState
#include <core/module_types.h>
//for IO::DataContainer::Ptr
#include <io/container.h>
//for Sound::MultichannelReceiver
#include <sound/receiver.h>

#include <boost/function.hpp>

//forward declarations
class Error;

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
    
    class Holder;
    /// Player interface
    class Player
    {
    public:
      typedef std::auto_ptr<Player> Ptr;

      virtual ~Player() {}

      /// Retrieving module holder instance
      virtual const Holder& GetModule() const = 0;

      /// Module playing state
      enum PlaybackState
      {
        MODULE_PLAYING,
        MODULE_STOPPED
      };

      /// Retrieving current playback state of loaded module
      virtual Error GetPlaybackState(unsigned& timeState, //current frame
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
      virtual Error SetParameters(const Parameters::Map& params) = 0;
    };

    /// Module holder interface
    class Holder
    {
    public:
      typedef boost::shared_ptr<Holder> Ptr;
      
      virtual ~Holder() {}

      /// Retrieving player info
      virtual void GetPlayerInfo(PluginInformation& info) const = 0;

      /// Retrieving information about loaded module
      virtual void GetModuleInformation(Information& info) const = 0;

      /// Building player from holder
      virtual Player::Ptr CreatePlayer() const = 0;
      
      /// Converting
      virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const = 0;
    };
  }

  /// Creating
  struct DetectParameters
  {
    typedef boost::function<bool(const PluginInformation&)> FilterFunc;
    FilterFunc Filter;
    typedef boost::function<Error(Module::Holder::Ptr player)> CallbackFunc;
    CallbackFunc Callback;
    typedef boost::function<void(const String&)> LogFunc;
    LogFunc Logger;
  };
  
  Error DetectModules(const Parameters::Map& commonParams, const DetectParameters& detectParams, 
    IO::DataContainer::Ptr data, const String& startSubpath);
}

#endif //__CORE_PLAYER_H_DEFINED__
