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

    virtual Error GetPlaybackState(Module::State& state,
      Module::Analyze::ChannelsState& analyzeState) const
    {
      Locker lock(Mutex);
      state = PrevState;
      analyzeState = PrevAnalyze;
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
      Sound::MultichannelReceiver& receiver)
    {
      Locker lock(Mutex);
      if (const Error& e = Delegate->GetPlaybackState(PrevState, PrevAnalyze))
      {
        return e;
      }
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
    
    virtual Error SetParameters(const Parameters::Accessor& params)
    {
      Locker lock(Mutex);
      return Delegate->SetParameters(params);
    }
  private:
    const Module::Player::Ptr Delegate;
    mutable boost::mutex Mutex;
    Module::State PrevState;
    Module::Analyze::ChannelsState PrevAnalyze;
  };
  
  void CopyInitialParameters(const RenderParameters& renderParams, Parameters::Modifier& commonParams)
  {
    commonParams.SetIntValue(Parameters::ZXTune::Sound::FREQUENCY, renderParams.SoundFreq);
    commonParams.SetIntValue(Parameters::ZXTune::Sound::CLOCKRATE, renderParams.ClockFreq);
    commonParams.SetIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, renderParams.FrameDurationMicrosec);
    commonParams.SetIntValue(Parameters::ZXTune::Sound::LOOPMODE, renderParams.Looping);
  }
  
  void UpdateRenderParameters(const Parameters::Accessor& params, RenderParameters& renderParams)
  {
    Parameters::IntType intVal = 0;
    if (params.FindIntValue(Parameters::ZXTune::Sound::FREQUENCY, intVal))
    {
      renderParams.SoundFreq = static_cast<uint_t>(intVal);
    }
    if (params.FindIntValue(Parameters::ZXTune::Sound::CLOCKRATE, intVal))
    {
      renderParams.ClockFreq = static_cast<uint64_t>(intVal);
    }
    if (params.FindIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, intVal))
    {
      renderParams.FrameDurationMicrosec = static_cast<uint_t>(intVal);
    }
    if (params.FindIntValue(Parameters::ZXTune::Sound::LOOPMODE, intVal))
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
    
    virtual void ApplyData(const MultiSample& samp)
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
      : CommonParameters(Parameters::Container::Create())
      , Signaller(SignalsDispatcher::Create())
      , SyncBarrier(TOTAL_WORKING_THREADS)
      , CurrentState(Backend::NOTOPENED), InProcess(false)
      , Channels(0), Renderer(new BufferRenderer(Buffer))
    {
      MixersSet.resize(MAX_MIXERS_COUNT);
      CopyInitialParameters(RenderingParameters, *CommonParameters);
    }

    BackendImpl::~BackendImpl()
    {
      assert(Backend::STOPPED == CurrentState ||
          Backend::NOTOPENED == CurrentState ||
          Backend::FAILED == CurrentState);
    }

    Error BackendImpl::SetModule(Module::Holder::Ptr holder)
    {
      try
      {
        if (!holder)
        {
          Log::Debug(THIS_MODULE, "Reseting the holder");
          Locker lock(PlayerMutex);
          StopPlayback();
          Holder.reset();
          Player.reset();
          CurrentState = Backend::NOTOPENED;
        }
        else
        {
          Log::Debug(THIS_MODULE, "Opening the holder");
          const uint_t physicalChannels = holder->GetModuleInformation()->PhysicalChannels();
          CheckChannels(physicalChannels);

          Locker lock(PlayerMutex);
          {
            Log::Debug(THIS_MODULE, "Creating the player");
            Module::Player::Ptr tmpPlayer(new SafePlayerWrapper(holder->CreatePlayer()));
            ThrowIfError(tmpPlayer->SetParameters(*CommonParameters));
            StopPlayback();
            Holder = holder;
            Player = tmpPlayer;
          }
          Channels = physicalChannels;
          Mixer::Ptr& curMixer = MixersSet[Channels - 1];
          if (!curMixer)
          {
            ThrowIfError(CreateMixer(Channels, curMixer));
          }
          curMixer->SetTarget(FilterObject ? FilterObject : Renderer);
          CurrentState = Backend::STOPPED;
          SendSignal(Backend::MODULE_OPEN);
        }
        RenderError = Error();
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
        
        const Backend::State prevState = CurrentState;
        if (Backend::STOPPED == prevState)
        {
          Log::Debug(THIS_MODULE, "Starting playback");
          Player->Reset();
          RenderThread = boost::thread(std::mem_fun(&BackendImpl::RenderFunc), this);
          SyncBarrier.wait();//wait until real start
          if (Backend::STARTED != CurrentState)
          {
            ThrowIfError(RenderError);
          }
          Log::Debug(THIS_MODULE, "Started");
        }
        else if (Backend::PAUSED == prevState)
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
        if (Backend::STARTED == CurrentState)
        {
          Log::Debug(THIS_MODULE, "Pausing playback");
          boost::unique_lock<boost::mutex> locker(PauseMutex);
          CurrentState = Backend::PAUSED;
          //wait until really Backend::PAUSED
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
        if (Backend::STOPPED == CurrentState)
        {
          return Error();
        }
        ThrowIfError(Player->SetPosition(frame));
        SendSignal(Backend::MODULE_SEEK);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_SEEK).AddSuberror(e);
      }
    }

    Backend::State BackendImpl::GetCurrentState(Error* error) const
    {
      Locker lock(PlayerMutex);
      if (error)
      {
        *error = RenderError;
      }
      return CurrentState;
    }

    SignalsCollector::Ptr BackendImpl::CreateSignalsCollector(uint_t signalsMask) const
    {
      return Signaller->CreateCollector(signalsMask);
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
          curMixer->SetTarget(FilterObject ? FilterObject : Renderer);
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
        FilterObject->SetTarget(Renderer);
        if (Channels)
        {
          MixersSet[Channels]->SetTarget(FilterObject);
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }

    Error BackendImpl::SetParameters(const Parameters::Accessor& params)
    {
      try
      {
        Locker lock(PlayerMutex);
        Parameters::Container::Ptr newContainer = Parameters::Container::Create();
        params.Process(*newContainer);
        CommonParameters->Process(*newContainer);
        UpdateRenderParameters(*newContainer, RenderingParameters);
        OnParametersChanged(*newContainer);
        if (Player)
        {
          ThrowIfError(Player->SetParameters(*newContainer));
        }
        //merge result back
        CommonParameters = newContainer;
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    //internal functions
    void BackendImpl::DoStartup()
    {
      OnStartup();
      SendSignal(Backend::MODULE_START);
    }
    
    void BackendImpl::DoShutdown()
    {
      OnShutdown();
      SendSignal(Backend::MODULE_STOP);
    }
    
    void BackendImpl::DoPause()
    {
      OnPause();
      SendSignal(Backend::MODULE_PAUSE);
    }
    
    void BackendImpl::DoResume()
    {
      OnResume();
      SendSignal(Backend::MODULE_RESUME);
    }
    
    void BackendImpl::DoBufferReady(std::vector<MultiSample>& buffer)
    {
      OnBufferReady(buffer);
    }

    void BackendImpl::CheckState() const
    {
      ThrowIfError(RenderError);
      if (Backend::NOTOPENED == CurrentState)
      {
        throw Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }

    void BackendImpl::StopPlayback()
    {
      const Backend::State curState = CurrentState;
      if (Backend::STARTED == curState ||
          Backend::PAUSED == curState ||
          (Backend::STOPPED == curState && InProcess))
      {
        Log::Debug(THIS_MODULE, "Stopping playback");
        if (Backend::PAUSED == curState)
        {
          boost::unique_lock<boost::mutex> locker(PauseMutex);
          PauseEvent.notify_one();
          //wait until really resumed
          PauseEvent.wait(locker);
        }
        CurrentState = Backend::STOPPED;
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
        CurrentState = Backend::STARTED;
        InProcess = true;//starting begin
        DoStartup();//throw
        SyncBarrier.wait();
        InProcess = false;//starting finished
        for (;;)
        {
          const Backend::State curState = CurrentState;
          if (Backend::STOPPED == curState)
          {
            break;
          }
          else if (Backend::STARTED == curState)
          {
            if (!OnRenderFrame())
            {
              CurrentState = Backend::STOPPED;
              InProcess = true; //stopping begin
              SendSignal(Backend::MODULE_FINISH);
              break;
            }
          }
          else if (Backend::PAUSED == curState)
          {
            boost::unique_lock<boost::mutex> locker(PauseMutex);
            DoPause();
            //pausing finished
            PauseEvent.notify_one();
            //wait until unpause
            PauseEvent.wait(locker);
            DoResume();
            CurrentState = Backend::STARTED;
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
        CurrentState = Backend::FAILED;
        SendSignal(Backend::MODULE_STOP);
        SyncBarrier.wait();
        InProcess = false;
      }
      Log::Debug(THIS_MODULE, "Stopped playback thread");
    }

    void BackendImpl::SendSignal(uint_t sig)
    {
      Signaller->Notify(sig);
    }
  }
}
