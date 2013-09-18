/**
*
* @file      sound/backend.h
* @brief     Sound backend interface definition
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_BACKEND_H_DEFINED
#define SOUND_BACKEND_H_DEFINED

//common includes
#include <error.h>
#include <iterator.h>
//library includes
#include <core/module_holder.h> // for Module::Holder::Ptr, Converter::Ptr and other
#include <sound/gain.h>

namespace Sound
{
  //! @brief Describes supported backend
  class BackendInformation
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const BackendInformation> Ptr;
    //! Iterator type
    typedef ObjectIterator<Ptr> Iterator;

    virtual ~BackendInformation() {}

    //! Short spaceless identifier
    virtual String Id() const = 0;
    //! Textual description
    virtual String Description() const = 0;
    //! Backend capabilities @see backend_attrs.h
    virtual uint_t Capabilities() const = 0;
    //! Actuality status
    virtual Error Status() const = 0;
  };

  //! @brief Volume control interface
  class VolumeControl
  {
  public:
    //! @brief Pointer types
    typedef boost::shared_ptr<VolumeControl> Ptr;

    virtual ~VolumeControl() {}

    //! @brief Getting current hardware mixer volume
    //! @return Result volume
    //! throw Error in case of success
    virtual Gain GetVolume() const = 0;

    //! @brief Setting hardware mixer volume
    //! @param volume Levels value
    //! @throw Error in case of success
    virtual void SetVolume(const Gain& volume) = 0;
  };

  //! @brief Playback control interface
  class PlaybackControl
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<PlaybackControl> Ptr;

    virtual ~PlaybackControl() {}

    //! @brief Starting playback after stop or pause
    //! @throw Error in case of error
    //! @note No effect if module is currently played
    virtual void Play() = 0;

    //! @brief Pausing the playback
    //! @throw Error in case of error
    //! @note No effect if module is currently paused or stopped
    virtual void Pause() = 0;

    //! @brief Stopping the playback
    //! @throw Error in case of error
    //! @note No effect if module is already stopped
    virtual void Stop() = 0;

    //! @brief Seeking
    //! @param frame Number of specified frame
    //! @throw Error in case of error
    //! @note If parameter is out of range, playback will be stopped
    virtual void SetPosition(uint_t frame) = 0;

    //! @brief Current playback state
    enum State
    {
      //! Playback is stopped
      STOPPED,
      //! Playback is paused
      PAUSED,
      //! Playback is in progress
      STARTED
    };
    //! @brief Retrieving current playback state
    //! @return Current state
    virtual State GetCurrentState() const = 0;
  };

  //! @brief %Sound backend interface
  class Backend
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Backend> Ptr;

    virtual ~Backend() {}

    //! @brief Current tracking status
    virtual Module::TrackState::Ptr GetTrackState() const = 0;

    //! @brief Getting analyzer interface
    virtual Module::Analyzer::Ptr GetAnalyzer() const = 0;

    //! @brief Gettint playback controller
    virtual PlaybackControl::Ptr GetPlaybackControl() const = 0;

    //! @brief Getting volume controller
    //! @return Pointer to volume control object if supported, empty pointer if not
    virtual VolumeControl::Ptr GetVolumeControl() const = 0;
  };

  class BackendCallback
  {
  public:
    typedef boost::shared_ptr<BackendCallback> Ptr;
    virtual ~BackendCallback() {}

    virtual void OnStart() = 0;
    virtual void OnFrame(const Module::TrackState& state) = 0;
    virtual void OnStop() = 0;
    virtual void OnPause() = 0;
    virtual void OnResume() = 0;
    virtual void OnFinish() = 0;
  };
}

#endif //SOUND_BACKEND_H_DEFINED
