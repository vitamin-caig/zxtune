/*
Abstract:
  Backend helper interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "backend_impl.h"
//common includes
#include <tools.h>
#include <error_tools.h>
#include <logging.h>
//library includes
#include <sound/error_codes.h>
#include <sound/sound_parameters.h>
//text includes
#include <sound/text/sound.h>

#define FILE_TAG B3D60DB5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;
  
  const std::string THIS_MODULE("BackendBase");

  typedef boost::lock_guard<boost::mutex> Locker;

  //playing thread and starting/stopping thread
  const std::size_t TOTAL_WORKING_THREADS = 2;
  
  const uint_t MIN_MIXERS_COUNT = 1;
  const uint_t MAX_MIXERS_COUNT = 8;
  
  inline void CheckChannels(uint_t chans)
  {
    if (!in_range(chans, MIN_MIXERS_COUNT, MAX_MIXERS_COUNT))
    {
      throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
        Text::SOUND_ERROR_BACKEND_INVALID_CHANNELS, chans, MIN_MIXERS_COUNT, MAX_MIXERS_COUNT);
    }
  }

  class SafePlayerWrapper : public Module::Player
  {
  public:
    explicit SafePlayerWrapper(Module::Player::Ptr player)
      : Delegate(player)
    {
      if (!Delegate.get())
      {
        throw Error(THIS_LINE, BACKEND_FAILED_CREATE, Text::SOUND_ERROR_BACKEND_INVALID_MODULE);
      }
    }

    virtual const Module::Holder& GetModule() const
    {
      //do not sync
      return Delegate->GetModule();
    }

    virtual Error GetPlaybackState(uint_t& timeState, Module::Tracking& trackState,
      Module::Analyze::ChannelsState& analyzeState) const
    {
      Locker lock(Mutex);
      return Delegate->GetPlaybackState(timeState, trackState, analyzeState);
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
      Sound::MultichannelReceiver& receiver)
    {
      Locker lock(Mutex);
      return Delegate->RenderFrame(params, state, receiver);
    }
    
    virtual Error Reset()
    {
      Locker lock(Mutex);
      return Delegate->Reset();
    }
    
    virtual Error SetPosition(uint_t frame)
    {
      Locker lock(Mutex);
      return Delegate->SetPosition(frame);
    }
    
    virtual Error SetParameters(const Parameters::Map& params)
    {
      Locker lock(Mutex);
      return Delegate->SetParameters(params);
    }
  private:
    const Module::Player::Ptr Delegate;
    mutable boost::mutex Mutex;
  };
  
  void CopyInitialParameters(const RenderParameters& renderParams, Parameters::Map& commonParams)
  {
    Parameters::IntType intVal = 0;
    commonParams[Parameters::ZXTune::Sound::FREQUENCY] = renderParams.SoundFreq;
    assert(Parameters::FindByName(commonParams, Parameters::ZXTune::Sound::FREQUENCY, intVal));
    commonParams[Parameters::ZXTune::Sound::CLOCKRATE] = renderParams.ClockFreq;
    assert(Parameters::FindByName(commonParams, Parameters::ZXTune::Sound::CLOCKRATE, intVal));
    commonParams[Parameters::ZXTune::Sound::FRAMEDURATION] = renderParams.FrameDurationMicrosec;
    assert(Parameters::FindByName(commonParams, Parameters::ZXTune::Sound::FRAMEDURATION, intVal));
    intVal = renderParams.Looping;
    commonParams[Parameters::ZXTune::Sound::LOOPMODE] = intVal;
    assert(Parameters::FindByName(commonParams, Parameters::ZXTune::Sound::FRAMEDURATION, intVal));
  }
  
  void UpdateRenderParameters(const Parameters::Map& updates, RenderParameters& renderParams)
  {
    Parameters::IntType intVal = 0;
    if (Parameters::FindByName(updates, Parameters::ZXTune::Sound::FREQUENCY, intVal))
    {
      renderParams.SoundFreq = static_cast<uint_t>(intVal);
    }
    if (Parameters::FindByName(updates, Parameters::ZXTune::Sound::CLOCKRATE, intVal))
    {
      renderParams.ClockFreq = static_cast<uint64_t>(intVal);
    }
    if (Parameters::FindByName(updates, Parameters::ZXTune::Sound::FRAMEDURATION, intVal))
    {
      renderParams.FrameDurationMicrosec = static_cast<uint_t>(intVal);
    }
    if (Parameters::FindByName(updates, Parameters::ZXTune::Sound::LOOPMODE, intVal))
    {
      renderParams.Looping = static_cast<LoopMode>(intVal);
    }
  }
  
  class Unlocker
  {
  public:
    explicit Unlocker(boost::mutex& mtx) : Obj(mtx)
    {
      Obj.unlock();
    }
    ~Unlocker()
    {
      Obj.lock();
    }
  private:
    boost::mutex& Obj;
  };
  
  class BufferRenderer : public Receiver
  {
  public:
    explicit BufferRenderer(std::vector<MultiSample>& buf) : Buffer(buf)
    {
    }
    
    virtual void ApplySample(const MultiSample& samp)
    {
      Buffer.push_back(samp);
    }
    
    void Flush()
    {
    }
  private:
    std::vector<MultiSample>& Buffer;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    BackendImpl::BackendImpl()
      : SyncBarrier(TOTAL_WORKING_THREADS)
      , CurrentState(NOTOPENED), InProcess(false)
      , Channels(0), Renderer(new BufferRenderer(Buffer))
    {
      MixersSet.resize(MAX_MIXERS_COUNT);
      CopyInitialParameters(RenderingParameters, CommonParameters);
    }

    BackendImpl::~BackendImpl()
    {
      assert(STOPPED == CurrentState ||
          NOTOPENED == CurrentState);
    }

    Error BackendImpl::SetModule(Module::Holder::Ptr holder)
    {
      try
      {
        Log::Debug(THIS_MODULE, "Opening the holder");
        if (!holder.get())
        {
          throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_MODULE);
        }
        Module::Information modInfo;
        holder->GetModuleInformation(modInfo);
        CheckChannels(modInfo.PhysicalChannels);
        
        Locker lock(PlayerMutex);
        {
          Log::Debug(THIS_MODULE, "Creating the player");
          Module::Player::Ptr tmpPlayer(new SafePlayerWrapper(holder->CreatePlayer()));
          ThrowIfError(tmpPlayer->SetParameters(CommonParameters));
          StopPlayback();
          Player = tmpPlayer;
        }

        Channels = modInfo.PhysicalChannels;
        CurrentState = STOPPED;
        Mixer::Ptr& curMixer = MixersSet[Channels - 1];
        if (!curMixer)
        {
          ThrowIfError(CreateMixer(Channels, curMixer));
        }
        curMixer->SetEndpoint(FilterObject ? FilterObject : Renderer);
        SendEvent(OPEN);
        Log::Debug(THIS_MODULE, "Done!");
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Module::Player::ConstWeakPtr BackendImpl::GetPlayer() const
    {
      Locker lock(PlayerMutex);
      return Module::Player::ConstWeakPtr(Player);
    }
    
    Error BackendImpl::Play()
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        
        const State prevState = CurrentState;
        if (STOPPED == prevState)
        {
          Log::Debug(THIS_MODULE, "Starting playback");
          Player->Reset();
          RenderThread = boost::thread(std::mem_fun(&BackendImpl::RenderFunc), this);
          SyncBarrier.wait();//wait until real start
          if (STARTED != CurrentState)
          {
            ThrowIfError(RenderError);
          }
          Log::Debug(THIS_MODULE, "Started");
        }
        else if (PAUSED == prevState)
        {
          Log::Debug(THIS_MODULE, "Resuming playback");
          boost::unique_lock<boost::mutex> locker(PauseMutex);
          PauseEvent.notify_one();
          //wait until really resumed
          PauseEvent.wait(locker);
          Log::Debug(THIS_MODULE, "Resumed");
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_PLAYBACK).AddSuberror(e);
      }
      catch (const boost::thread_resource_error&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
    }
    
    Error BackendImpl::Pause()
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        if (STARTED == CurrentState)
        {
          Log::Debug(THIS_MODULE, "Pausing playback");
          boost::unique_lock<boost::mutex> locker(PauseMutex);
          CurrentState = PAUSED;
          //wait until really paused
          PauseEvent.wait(locker);
          Log::Debug(THIS_MODULE, "Paused");
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_PAUSE).AddSuberror(e);
      }
    }
    
    Error BackendImpl::Stop()
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        StopPlayback();
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_STOP).AddSuberror(e);
      }
    }

    Error BackendImpl::SetPosition(uint_t frame)
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        if (STOPPED == CurrentState)
        {
          return Error();
        }
        ThrowIfError(Player->SetPosition(frame));
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_SEEK).AddSuberror(e);
      }
    }

    Error BackendImpl::GetCurrentState(Backend::State& state) const
    {
      Locker lock(PlayerMutex);
      state = CurrentState;
      return RenderError;
    }

    Backend::Event BackendImpl::WaitForEvent(Event evt, uint_t timeoutMs) const
    {
      boost::mutex localMutex;
      boost::unique_lock<boost::mutex> locker(localMutex);
      return evt > TIMEOUT && evt < LAST_EVENT && timeoutMs > 0 &&
        Events[evt].timed_wait(locker, boost::posix_time::milliseconds(timeoutMs))
        ? evt : TIMEOUT;
    }

    Error BackendImpl::SetMixer(const std::vector<MultiGain>& data)
    {
      try
      {
        const uint_t mixChannels = data.size();
        CheckChannels(mixChannels);
        
        Locker lock(PlayerMutex);
        
        Mixer::Ptr& curMixer = MixersSet[mixChannels - 1];
        if (!curMixer)
        {
          ThrowIfError(CreateMixer(mixChannels, curMixer));
        }
        ThrowIfError(curMixer->SetMatrix(data));
        //update only if current mutex updated
        if (Channels)
        {
          curMixer->SetEndpoint(FilterObject ? FilterObject : Renderer);
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Error BackendImpl::SetFilter(Converter::Ptr converter)
    {
      try
      {
        if (!converter)
        {
          throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_FILTER);
        }
        Locker lock(PlayerMutex);
        FilterObject = converter;
        FilterObject->SetEndpoint(Renderer);
        if (Channels)
        {
          MixersSet[Channels]->SetEndpoint(FilterObject);
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }

    Error BackendImpl::SetParameters(const Parameters::Map& params)
    {
      try
      {
        Locker lock(PlayerMutex);
        Parameters::Map updates;
        Parameters::DifferMaps(params, CommonParameters, updates);
        UpdateRenderParameters(updates, RenderingParameters);
        OnParametersChanged(updates);
        if (Player)
        {
          ThrowIfError(Player->SetParameters(updates));
        }
        //merge result back
        Parameters::MergeMaps(CommonParameters, params, CommonParameters, true);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Error BackendImpl::GetParameters(Parameters::Map& params) const
    {
      Locker lock(PlayerMutex);
      params = CommonParameters;
      return Error();
    }

    //internal functions
    void BackendImpl::DoStartup()
    {
      OnStartup();
      SendEvent(START);
    }
    
    void BackendImpl::DoShutdown()
    {
      OnShutdown();
      SendEvent(STOP);
    }
    
    void BackendImpl::DoPause()
    {
      OnPause();
      SendEvent(STOP);
    }
    
    void BackendImpl::DoResume()
    {
      OnResume();
      SendEvent(START);
    }
    
    void BackendImpl::DoBufferReady(std::vector<MultiSample>& buffer)
    {
      OnBufferReady(buffer);
      SendEvent(FRAME);
    }

    void BackendImpl::CheckState() const
    {
      ThrowIfError(RenderError);
      if (NOTOPENED == CurrentState)
      {
        throw Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }

    void BackendImpl::StopPlayback()
    {
      const State curState = CurrentState;
      if (STARTED == curState ||
          PAUSED == curState ||
          (STOPPED == curState && InProcess))
      {
        Log::Debug(THIS_MODULE, "Stopping playback");
        if (PAUSED == curState)
        {
          boost::unique_lock<boost::mutex> locker(PauseMutex);
          PauseEvent.notify_one();
          //wait until really resumed
          PauseEvent.wait(locker);
        }
        CurrentState = STOPPED;
        InProcess = true;//stopping now
        {
          assert(!PlayerMutex.try_lock());
          Unlocker unlock(PlayerMutex);
          SyncBarrier.wait();//wait for thread stop
          RenderThread.join();//cleanup thread
        }
        Log::Debug(THIS_MODULE, "Stopped");
        ThrowIfError(RenderError);
      }
    }

    bool BackendImpl::OnRenderFrame()
    {
      bool res = false;
      {
        Locker lock(PlayerMutex);
        if (Mixer::Ptr mixer = MixersSet[Channels - 1])
        {
          Buffer.reserve(RenderingParameters.SamplesPerFrame());
          Buffer.clear();
          Module::Player::PlaybackState state;
          ThrowIfError(Player->RenderFrame(RenderingParameters, state, *mixer));
          res = Module::Player::MODULE_PLAYING == state;
        }
      }
      DoBufferReady(Buffer);
      return res;
    }

    void BackendImpl::RenderFunc()
    {
      Log::Debug(THIS_MODULE, "Started playback thread");
      try
      {
        CurrentState = STARTED;
        InProcess = true;//starting begin
        DoStartup();//throw
        SyncBarrier.wait();
        InProcess = false;//starting finished
        for (;;)
        {
          const State curState = CurrentState;
          if (STOPPED == curState)
          {
            break;
          }
          else if (STARTED == curState)
          {
            if (!OnRenderFrame())
            {
              CurrentState = STOPPED;
              InProcess = true; //stopping begin
              break;
            }
          }
          else if (PAUSED == curState)
          {
            boost::unique_lock<boost::mutex> locker(PauseMutex);
            DoPause();
            //pausing finished
            PauseEvent.notify_one();
            //wait until unpause
            PauseEvent.wait(locker);
            DoResume();
            CurrentState = STARTED;
            //unpausing finished
            PauseEvent.notify_one();
          }
        }
        Log::Debug(THIS_MODULE, "Stopping playback thread");
        DoShutdown();//throw
        SyncBarrier.wait();
        InProcess = false; //stopping finished
      }
      catch (const Error& e)
      {
        RenderError = e;
        //if any...
        PauseEvent.notify_all();
        Log::Debug(THIS_MODULE, "Stopping playback thread by error");
        CurrentState = STOPPED;
        SendEvent(STOP);
        SyncBarrier.wait();
        InProcess = false;
      }
      Log::Debug(THIS_MODULE, "Stopped playback thread");
    }
    
    void BackendImpl::SendEvent(Event evt)
    {
      assert(evt > TIMEOUT && evt < LAST_EVENT);
      Events[evt].notify_all();
    }
  }
}
