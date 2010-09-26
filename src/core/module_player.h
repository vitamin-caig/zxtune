/**
*
* @file     core/module_player.h
* @brief    Modules player interface definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_PLAYER_H_DEFINED__
#define __CORE_MODULE_PLAYER_H_DEFINED__

//library includes
#include <core/module_types.h> //for Module::Analyze::ChannelsState
#include <sound/receiver.h>    //for Sound::MultichannelReceiver
//boost includes
#include <boost/weak_ptr.hpp>

//forward declarations
class Error;

namespace Parameters
{
  class Accessor;
}

namespace ZXTune
{
  namespace Sound
  {
    struct RenderParameters;
  }

  namespace Module
  {
    //! @brief %Module player interface
    class Player
    {
    public:
      //! @brief Generic pointer type
      typedef boost::shared_ptr<Player> Ptr;
      //! @brief Read-only pointer type
      typedef boost::shared_ptr<const Player> ConstPtr;

      virtual ~Player() {}

      //! @brief Referenced module information
      virtual Information::Ptr GetInformation() const = 0;

      //! @brief Current tracking status
      virtual TrackState::Ptr GetTrackState() const = 0;

      //! @brief Getting current analyze state of loaded module
      //! @param analyzeState Reference to store analyze result
      virtual void GetAnalyzer(Analyze::ChannelsState& analyzeState) const = 0;

      //! @brief Getting analyzer interface
      virtual Analyzer::Ptr GetAnalyzer() const = 0;

      //! @brief Playing state
      enum PlaybackState
      {
        //! Currently playing
        MODULE_PLAYING,
        //! Currently stopped
        MODULE_STOPPED
      };

      //! @brief Rendering single frame and modifying internal state
      //! @param params %Sound rendering-related parameters
      //! @param state Playback state player transitioned to
      //! @param receiver Output stream receiver
      //! @return Error() in case of success
      virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
        Sound::MultichannelReceiver& receiver) = 0;

      //! @brief Performing reset to initial state
      //! @return Error() in case of success
      virtual Error Reset() = 0;

      //! @brief Seeking
      //! @param frame Number of specified frame
      //! @return Error() in case of success
      //! @note Seeking out of range is safe, but state will be MODULE_PLAYING untill next RenderFrame call happends.
      //! @note It produces only the flush
      virtual Error SetPosition(uint_t frame) = 0;

      //! @brief Changing runtime parameters
      //! @param params %Player-related parameters
      //! @return Error() in case of success
      virtual Error SetParameters(const Parameters::Accessor& params) = 0;
    };
  }
}

#endif //__CORE_MODULE_PLAYER_H_DEFINED__
