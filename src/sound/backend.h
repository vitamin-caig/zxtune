/**
*
* @file      sound/backend.h
* @brief     Sound backend interface definition
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_BACKEND_H_DEFINED__
#define __SOUND_BACKEND_H_DEFINED__

//common includes
#include <iterator.h>
#include <signals_collector.h>
//library includes
#include <core/module_holder.h> // for Module::Holder::Ptr, Converter::Ptr and other
//std includes
#include <memory>

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

    //! @brief Set of backend descriptors
    typedef std::vector<BackendInformation> BackendInformationArray;

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
      typedef std::auto_ptr<Backend> Ptr;

      virtual ~Backend() {}

      //! @brief Retrieving backend information
      virtual const BackendInformation& GetInformation() const = 0;

      //! @brief Binding module to backend
      //! @param holder Specified module
      //! @return Error() in case of success
      //! @note Previously binded module playback will be stopped
      virtual Error SetModule(Module::Holder::Ptr holder) = 0;
      
      //! @brief Retrieving read-only access to current module player
      //! @result Weak read-only reference to thread-safe player instance, empty if there are no binded module
      virtual Module::Player::ConstWeakPtr GetPlayer() const = 0;
      
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
        //! No any binded module
        NOTOPENED,
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

        //! New module specified
        MODULE_OPEN =   0x01,
        //! Starting playback after stop
        MODULE_START =  0x02,
        //! Starting playback after pause
        MODULE_RESUME = 0x04,
        //! Pausing
        MODULE_PAUSE =  0x08,
        //! Stopping
        MODULE_STOP  =  0x10,
        //! Playback is finished
        MODULE_FINISH = 0x20,
        //! Seeking performed
        MODULE_SEEK   = 0x40,
        //@}
      };

      //! @brief Creating new signals collector
      //! @param signalsMask Required signals mask
      //! @return Pointer to new collector registered in backend. Automatically unregistered when expired
      virtual SignalsCollector::Ptr CreateSignalsCollector(uint_t signalsMask) const = 0;

      //! @brief Setting the mixers to use
      //! @param data Input mixer matrix
      //! @return Error() in case of success
      //! @note By default all mixers are set to mono mixing. Use this function to change it
      virtual Error SetMixer(const std::vector<MultiGain>& data) = 0;
      
      //! @brief Setting the filter to sound stream post-process
      //! @param converter Filter object
      //! @return Error() in case of success
      virtual Error SetFilter(Converter::Ptr converter) = 0;

      //! @brief Getting volume controller
      //! @return Pointer to volume control object if supported, empty pointer if not
      virtual VolumeControl::Ptr GetVolumeControl() const = 0;

      //! @brief Setting parameters backend and player
      //! @param params Specified parameters map
      //! @return Error() in case of success
      virtual Error SetParameters(const Parameters::Map& params) = 0;
      
      //! @brief Getting current parameters
      //! @param params Reference to return value
      //! @return Error() in case of success
      //! @note Result is merge between default internal values and externally specified using SetParameters function, even if some values are not recognized
      virtual Error GetParameters(Parameters::Map& params) const = 0;
    };

    //! @brief Backend creator interface
    class BackendCreator : public BackendInformation
    {
    public:
      //! Pointer type
      typedef boost::shared_ptr<const BackendCreator> Ptr;
      //! Iterator type
      typedef Iterator<BackendCreator::Ptr> IteratorType;
      //! Pointer to iterator type
      typedef std::auto_ptr<IteratorType> IteratorPtr;

      //! @brief Create backend using specified parameters
      //! @param params Map of backend-related parameters
      //! @param result Reference to result value
      //! @return Error() in case of success
      virtual Error CreateBackend(const Parameters::Map& params, Backend::Ptr& result) const = 0;
    };

    //! @brief Enumerating supported sound backends
    BackendCreator::IteratorPtr EnumerateBackends();
  }
}

#endif //__SOUND_BACKEND_H_DEFINED__
