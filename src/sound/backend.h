/**
*
* @file      sound/backend.h
* @brief     Sound backend interface definition
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_BACKEND_H_DEFINED__
#define __SOUND_BACKEND_H_DEFINED__

//common includes
#include <iterator.h>
#include <async/signals_collector.h>
//library includes
#include <core/module_holder.h> // for Module::Holder::Ptr, Converter::Ptr and other
#include <sound/mixer.h>

//forward declarations
class Error;

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Describes supported backend
    class BackendInformation
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const BackendInformation> Ptr;

      virtual ~BackendInformation() {}

      //! Short spaceless identifier
      virtual String Id() const = 0;
      //! Textual description
      virtual String Description() const = 0;
      //! Version in text format
      virtual String Version() const = 0;
      //! Backend capabilities @see backend_attrs.h
      virtual uint_t Capabilities() const = 0;
    };

    //! @brief Volume control interface
    class VolumeControl
    {
    public:
      //! @brief Pointer types
      typedef boost::shared_ptr<VolumeControl> Ptr;

      virtual ~VolumeControl() {}

      //! @brief Getting current hardware mixer volume
      //! @param volume Reference to return value
      //! @return Error() in case of success
      virtual Error GetVolume(MultiGain& volume) const = 0;

      //! @brief Setting hardware mixer volume
      //! @param volume Levels value
      //! @return Error() in case of success
      virtual Error SetVolume(const MultiGain& volume) = 0;
    };

    //! @brief %Sound backend interface
    class Backend
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<Backend> Ptr;

      virtual ~Backend() {}

      //! @brief Retrieving read-only access to current module player
      //! @result Weak read-only reference to thread-safe player instance, empty if there are no binded module
      virtual Module::Player::ConstPtr GetPlayer() const = 0;

      //! @brief Starting playback after stop or pause
      //! @return Error() in case of success
      //! @note No effect if module is currently played
      virtual Error Play() = 0;

      //! @brief Pausing the playback
      //! @result Error() in case of success
      //! @note No effect if module is currently paused or stopped
      virtual Error Pause() = 0;

      //! @brief Stopping the playback
      //! @result Error() in case of success
      //! @note No effect if module is already stopped
      virtual Error Stop() = 0;

      //! @brief Seeking
      //! @param frame Number of specified frame
      //! @return Error() in case of success
      //! @note If parameter is out of range, playback will be stopped
      virtual Error SetPosition(uint_t frame) = 0;

      //! @brief Current playback state
      enum State
      {
        //! Error ocurred, in common, equal to STOPPED
        FAILED,
        //! Playback is stopped
        STOPPED,
        //! Playback is paused
        PAUSED,
        //! Playback is in progress
        STARTED
      };
      //! @brief Retrieving current playback state
      //! @param error Optional pointer to result error
      //! @return Current state
      virtual State GetCurrentState(Error* error = 0) const = 0;

      //! @brief Signals specification
      enum Signal
      {
        //@{
        //! @name Module-related signals

        //! Starting playback after stop
        MODULE_START =  0x01,
        //! Starting playback after pause
        MODULE_RESUME = 0x02,
        //! Pausing
        MODULE_PAUSE =  0x04,
        //! Stopping
        MODULE_STOP  =  0x08,
        //! Playback is finished
        MODULE_FINISH = 0x10,
        //! Seeking performed
        MODULE_SEEK   = 0x20,
        //@}
      };

      //! @brief Creating new signals collector
      //! @param signalsMask Required signals mask
      //! @return Pointer to new collector registered in backend. Automatically unregistered when expired
      virtual Async::Signals::Collector::Ptr CreateSignalsCollector(uint_t signalsMask) const = 0;

      //! @brief Getting volume controller
      //! @return Pointer to volume control object if supported, empty pointer if not
      virtual VolumeControl::Ptr GetVolumeControl() const = 0;
    };

    class CreateBackendParameters
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const CreateBackendParameters> Ptr;
 
      virtual ~CreateBackendParameters() {}

      virtual Parameters::Accessor::Ptr GetParameters() const = 0;
      virtual Module::Holder::Ptr GetModule() const = 0;
      virtual Mixer::Ptr GetMixer() const = 0;
      virtual Converter::Ptr GetFilter() const = 0;
    };

    //! @brief Backend creator interface
    class BackendCreator : public BackendInformation
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const BackendCreator> Ptr;
      //! Iterator type
      typedef ObjectIterator<BackendCreator::Ptr> Iterator;

      //! @brief Create backend using specified parameters
      //! @param params %Backend-related parameters
      //! @param result Reference to result value
      //! @return Error() in case of success
      virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const = 0;
    };

    //! @brief Enumerating supported sound backends
    BackendCreator::Iterator::Ptr EnumerateBackends();
  }
}

#endif //__SOUND_BACKEND_H_DEFINED__
