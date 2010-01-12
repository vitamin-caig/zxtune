/*
Abstract:
  Backend helper interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "backend_impl.h"

#include <tools.h>
#include <error_tools.h>
#include <sound/error_codes.h>
#include <sound/sound_parameters.h>

#include <text/sound.h>

#define FILE_TAG B3D60DB5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  typedef boost::lock_guard<boost::mutex> Locker;

  const boost::posix_time::milliseconds PLAYTHREAD_SLEEP_PERIOD(1000);
  //playing thread and starting/stopping thread
  const std::size_t TOTAL_WORKING_THREADS = 2;
  
  const unsigned MIN_MIXERS_COUNT = 1;
  const unsigned MAX_MIXERS_COUNT = 8;
  
  inline void CheckChannels(unsigned chans)
  {
    if (!in_range<unsigned>(chans, MIN_MIXERS_COUNT, MAX_MIXERS_COUNT))
    {
      throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
        TEXT_SOUND_ERROR_BACKEND_INVALID_CHANNELS, chans, MIN_MIXERS_COUNT, MAX_MIXERS_COUNT);
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
        throw Error(THIS_LINE, BACKEND_FAILED_CREATE, TEXT_SOUND_ERROR_BACKEND_INVALID_MODULE);
      }
    }

    virtual const Module::Holder& GetModule() const
    {
      //do not sync
      return Delegate->GetModule();
    }

    virtual Error GetPlaybackState(unsigned& timeState, Module::Tracking& trackState,
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
    
    virtual Error SetPosition(unsigned frame)
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
    commonParams[Parameters::ZXTune::Sound::FREQUENCY] = renderParams.SoundFreq;
    assert(Parameters::FindByName<Parameters::IntType>(commonParams, Parameters::ZXTune::Sound::FREQUENCY));
    commonParams[Parameters::ZXTune::Sound::CLOCKRATE] = renderParams.ClockFreq;
    assert(Parameters::FindByName<Parameters::IntType>(commonParams, Parameters::ZXTune::Sound::CLOCKRATE));
    commonParams[Parameters::ZXTune::Sound::FRAMEDURATION] = renderParams.FrameDurationMicrosec;
    assert(Parameters::FindByName<Parameters::IntType>(commonParams, Parameters::ZXTune::Sound::FRAMEDURATION));
  }
  
  void UpdateRenderParameters(const Parameters::Map& updates, RenderParameters& renderParams)
  {
    if (const Parameters::IntType* freq = 
      Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY))
    {
      renderParams.SoundFreq = static_cast<unsigned>(*freq);
    }
    if (const Parameters::IntType* clock = 
      Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::CLOCKRATE))
    {
      renderParams.ClockFreq = static_cast<uint64_t>(*clock);
    }
    if (const Parameters::IntType* frame = 
      Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FRAMEDURATION))
    {
      renderParams.FrameDurationMicrosec = static_cast<unsigned>(*frame);
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
      : RenderingParameters()
      , SyncBarrier(TOTAL_WORKING_THREADS)
      , CurrentState(NOTOPENED), InProcess(false), RenderError()
      , Channels(0), Player(), FilterObject(), Renderer(new BufferRenderer(Buffer))
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
        if (!holder.get())
        {
          throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_MODULE);
        }
        Module::Information modInfo;
        holder->GetModuleInformation(modInfo);
        CheckChannels(modInfo.PhysicalChannels);
        
        Locker lock(PlayerMutex);
        {
          Module::Player::Ptr tmpPlayer(new SafePlayerWrapper(holder->CreatePlayer()));
          StopPlayback();
          Player = tmpPlayer;
        }

        Channels = modInfo.PhysicalChannels;
        CurrentState = STOPPED;
        Mixer::Ptr& curMixer(MixersSet[Channels - 1]);
        if (!curMixer)
        {
          ThrowIfError(CreateMixer(Channels, curMixer));
        }
        curMixer->SetEndpoint(FilterObject ? FilterObject : Renderer);
        SendEvent(OPEN);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Module::Player::ConstWeakPtr BackendImpl::GetPlayer() const
    {
      Locker lock(PlayerMutex);
      return boost::weak_ptr<const Module::Player>(Player);
    }
    
    Error BackendImpl::Play()
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        
        const State prevState(CurrentState);
        if (STOPPED == prevState)
        {
          Player->Reset();
          RenderThread = boost::thread(std::mem_fun(&BackendImpl::RenderFunc), this);
          SyncBarrier.wait();//wait until real start
          if (STARTED != CurrentState)
          {
            ThrowIfError(RenderError);
          }
        }
        else if (PAUSED == prevState)
        {
          DoResume();
          CurrentState = STARTED;
        }
        
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_PLAYBACK).AddSuberror(e);
      }
    }
    
    Error BackendImpl::Pause()
    {
      try
      {
        Locker lock(PlayerMutex);
        CheckState();
        DoPause();
        CurrentState = PAUSED;
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_PAUSE).AddSuberror(e);
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
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_STOP).AddSuberror(e);
      }
    }

    Error BackendImpl::SetPosition(unsigned frame)
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
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_SEEK).AddSuberror(e);
      }
    }

    Error BackendImpl::GetCurrentState(Backend::State& state) const
    {
      Locker lock(PlayerMutex);
      state = CurrentState;
      return RenderError;
    }

    Backend::Event BackendImpl::WaitForEvent(Event evt, unsigned timeoutMs) const
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
        const unsigned mixChannels = static_cast<unsigned>(data.size());
        CheckChannels(static_cast<unsigned>(mixChannels));
        
        Locker lock(PlayerMutex);
        
        Mixer::Ptr& curMixer(MixersSet[mixChannels - 1]);
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
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Error BackendImpl::SetFilter(Converter::Ptr converter)
    {
      try
      {
        if (!converter)
        {
          throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_FILTER);
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
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }

    inline bool CompareParameter(const Parameters::Map::value_type& lh, const Parameters::Map::value_type& rh)
    {
      return lh.first < rh.first || !(lh.second == rh.second);
    }

    Error BackendImpl::SetParameters(const Parameters::Map& params)
    {
      try
      {
        Locker lock(PlayerMutex);
        Parameters::Map updates;
        std::set_difference(params.begin(), params.end(),
          CommonParameters.begin(), CommonParameters.end(), std::inserter(updates, updates.end()),
          CompareParameter);
        UpdateRenderParameters(updates, RenderingParameters);
        OnParametersChanged(updates);
        if (Player)
        {
          ThrowIfError(Player->SetParameters(updates));
        }
        //merge result back
        {
          Parameters::Map merged;
          std::set_union(updates.begin(), updates.end(),
            CommonParameters.begin(), CommonParameters.end(), std::inserter(merged, merged.end()),
            CompareParameter);
          CommonParameters.swap(merged);
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
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
        throw Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }

    void BackendImpl::StopPlayback()
    {
      const State curState(CurrentState);
      if (STARTED == curState ||
          PAUSED == curState ||
          (STOPPED == curState && InProcess))
      {
        if (PAUSED == curState)
        {
          DoResume();
        }
        CurrentState = STOPPED;
        InProcess = true;//stopping now
        {
          assert(!PlayerMutex.try_lock());
          Unlocker unlock(PlayerMutex);
          SyncBarrier.wait();//wait for thread stop
          RenderThread.join();//cleanup thread
        }
      }
    }

    bool BackendImpl::SafeRenderFrame()
    {
      Locker lock(PlayerMutex);
      if (Mixer::Ptr mixer = MixersSet[Channels - 1])
      {
        Buffer.reserve(RenderingParameters.SamplesPerFrame());
        Buffer.clear();
        Module::Player::PlaybackState state;
        ThrowIfError(Player->RenderFrame(RenderingParameters, state, *mixer));
        return Module::Player::MODULE_PLAYING == state;
      }
      assert(!"Never get here");
      return false;
    }

    void BackendImpl::RenderFunc()
    {
      try
      {
        CurrentState = STARTED;
        InProcess = true;//starting begin
        DoStartup();//throw
        SyncBarrier.wait();
        InProcess = false;//starting finished
        for (;;)
        {
          const State curState(CurrentState);
          if (STOPPED == curState)
          {
            break;
          }
          else if (STARTED == curState)
          {
            const bool stopping = !SafeRenderFrame();
            DoBufferReady(Buffer);
            if (stopping)
            {
              CurrentState = STOPPED;
              InProcess = true; //stopping begin
              break;
            }
          }
          else if (PAUSED == curState)
          {
            boost::this_thread::sleep(PLAYTHREAD_SLEEP_PERIOD);
          }
        }
        DoShutdown();//throw
        SyncBarrier.wait();
        InProcess = false; //stopping finished
      }
      catch (const Error& e)
      {
        RenderError = e;
        CurrentState = STOPPED;
        SendEvent(STOP);
        SyncBarrier.wait();
        InProcess = false;
      }
    }
    
    void BackendImpl::SendEvent(Event evt)
    {
      assert(evt > TIMEOUT && evt < LAST_EVENT);
      Events[evt].notify_all();
    }
  }
}
