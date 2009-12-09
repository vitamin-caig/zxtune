/*
Abstract:
  Backend helper interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "backend_impl.h"

#include "../error_codes.h"

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
    SafePlayerWrapper(Module::Player::Ptr delegate, boost::mutex& sync)
      : Delegate(delegate), Mutex(sync)
    {
    }
    
    virtual void GetPlayerInfo(PluginInformation& info) const
    {
      Locker lock(Mutex);
      return Delegate->GetPlayerInfo(info);
    }

    virtual void GetModuleInformation(Module::Information& info) const
    {
      Locker lock(Mutex);
      return Delegate->GetModuleInformation(info);
    }

    virtual Error GetModuleState(unsigned& timeState, Module::Tracking& trackState,
      Module::Analyze::ChannelsState& analyzeState) const
    {
      Locker lock(Mutex);
      return Delegate->GetModuleState(timeState, trackState, analyzeState);
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
      Sound::MultichannelReceiver& receiver)
    {
      //do not lock
      return Delegate->RenderFrame(params, state, receiver);
    }
    
    virtual Error Reset()
    {
      //do not lock
      return Delegate->Reset();
    }
    
    virtual Error SetPosition(unsigned frame)
    {
      //do not lock
      return Delegate->SetPosition(frame);
    }

    virtual Error Convert(const Module::Conversion::Parameter& param, Dump& dst) const
    {
      //do not lock
      return Delegate->Convert(param, dst);
    }
  private:
    const Module::Player::Ptr Delegate;
    boost::mutex& Mutex;
  };
  
  void GetInitialParameters(RenderParameters& params)
  {
    params.SoundFreq = 44100;
    params.ClockFreq = 1750000;
    params.FrameDurationMicrosec = 20000;//50Hz
    params.Flags = 0;
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
      : DriverParameters(), RenderingParameters()
      , SyncBarrier(TOTAL_WORKING_THREADS)
      , CurrentState(NOTOPENED), InProcess(false), RenderError()
      , Channels(0), Player(), FilterObject(), Renderer(new BufferRenderer(Buffer))
    {
      MixersSet.resize(MAX_MIXERS_COUNT);
      GetInitialParameters(RenderingParameters);
    }

    BackendImpl::~BackendImpl()
    {
      assert(STOPPED == CurrentState ||
          NOTOPENED == CurrentState);
    }

    Error BackendImpl::SetPlayer(Module::Player::Ptr player)
    {
      try
      {
        if (!player.get())
        {
          throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_PLAYER);
        }
        Module::Information modInfo;
        player->GetModuleInformation(modInfo);
        CheckChannels(modInfo.PhysicalChannels);
        
        Locker lock(PlayerMutex);
        StopPlayback();
        Channels = modInfo.PhysicalChannels;
        Player = Module::Player::Ptr(new SafePlayerWrapper(player, PlayerMutex));
        CurrentState = STOPPED;
        Mixer::Ptr& curMixer(MixersSet[Channels - 1]);
        if (!curMixer)
        {
          ThrowIfError(CreateMixer(Channels, curMixer));
        }
        curMixer->SetEndpoint(FilterObject ? FilterObject : Renderer);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    boost::weak_ptr<const Module::Player> BackendImpl::GetPlayer() const
    {
      Locker lock(PlayerMutex);
      assert(Player);
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
          OnResume();
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
        OnPause();
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
        assert(Player.get());
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

    Error BackendImpl::SetMixer(const std::vector<MultiGain>& data)
    {
      try
      {
        CheckChannels(static_cast<unsigned>(data.size()));
        
        Locker lock(PlayerMutex);
        const unsigned mixChannels = static_cast<unsigned>(data.size());
        
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

    class are_strict_equals
        : public boost::static_visitor<bool>
    {
    public:
        template <typename T, typename U>
        bool operator()( const T &, const U & ) const
        {
            return false; // cannot compare different types
        }

        template <typename T>
        bool operator()( const T & lhs, const T & rhs ) const
        {
            return lhs == rhs;
        }
    };

    bool CompareParameter(const ParametersMap::value_type& lh, const ParametersMap::value_type& rh)
    {
      return lh.first < rh.first || !boost::apply_visitor(are_strict_equals(), lh.second, rh.second);
    }

    Error BackendImpl::SetDriverParameters(const ParametersMap& params)
    {
      try
      {
        Locker lock(PlayerMutex);
        ParametersMap updates;
        std::set_difference(params.begin(), params.end(),
          DriverParameters.begin(), DriverParameters.end(), std::inserter(updates, updates.end()),
          CompareParameter);
        OnParametersChanged(updates);
        DriverParameters = params;
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_SETUP_ERROR, TEXT_SOUND_ERROR_BACKEND_SETUP_BACKEND).AddSuberror(e);
      }
    }
    
    Error BackendImpl::GetDriverParameters(ParametersMap& params) const
    {
      Locker lock(PlayerMutex);
      params = DriverParameters;
      return Error();
    }

    Error BackendImpl::SetRenderParameters(const RenderParameters& params)
    {
      Locker lock(PlayerMutex);
      RenderingParameters = params;
      return Error();
    }
    
    Error BackendImpl::GetRenderParameters(RenderParameters& params) const
    {
      Locker lock(PlayerMutex);
      params = RenderingParameters;
      return Error();
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
          OnResume();
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
        OnStartup();//throw
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
            const bool cont = SafeRenderFrame();
            OnBufferReady(Buffer);
            if (!cont)//throw
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
        OnShutdown();//throw
        SyncBarrier.wait();
        InProcess = false; //stopping finished
      }
      catch (const Error& e)
      {
        RenderError = e;
        CurrentState = STOPPED;
        SyncBarrier.wait();
        InProcess = false;
      }
    }
  }
}
