/*
Abstract:
  Modules player interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_MODULE_PLAYER_H_DEFINED__
#define __CORE_MODULE_PLAYER_H_DEFINED__

//for typedef'ed Parameters::Map
#include <parameters.h>

//for Module::Analyze::ChannelsState
#include <core/module_types.h>
//for Sound::MultichannelReceiver
#include <sound/receiver.h>

//forward declarations
class Error;

namespace ZXTune
{
  //forward declarations
  namespace Sound
  {
    struct RenderParameters;
  }
  
  namespace Module
  {
    class Holder;

    /// Player interface
    class Player
    {
    public:
      typedef boost::shared_ptr<Player> Ptr;
      typedef boost::shared_ptr<const Player> ConstPtr;
      typedef boost::weak_ptr<const Player> ConstWeakPtr;

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
  }
}

#endif //__CORE_MODULE_PLAYER_H_DEFINED__
