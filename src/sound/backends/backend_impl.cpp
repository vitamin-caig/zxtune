/*
Abstract:
  Backend helper interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "backend_impl.h"

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
    SafePlayerWrapper(Module::Player::Ptr player, boost::mutex& sync)
      : Delegate(player), Mutex(sync)
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
    
    virtual Error SetParameters(const ParametersMap& params)
    {
      //do not lock
      return Delegate->SetParameters(params);
    }
  private:
    const Module::Player::Ptr Delegate;
    boost::mutex& Mutex;
  };
  
  void CopyInitialParameters(const RenderParameters& renderParams, ParametersMap& commonParams)
  {
    commonParams[Parameters::Sound::FREQUENCY] = renderParams.SoundFreq;
    assert(FindParameter<int64_t>(commonParams, Parameters::Sound::FREQUENCY));
    commonParams[Parameters::Sound::CLOCKRATE] = renderParams.ClockFreq;
    assert(FindParameter<int64_t>(commonParams, Parameters::Sound::CLOCKRATE));
    commonParams[Parameters::Sound::FRAMEDURATION] = renderParams.FrameDurationMicrosec;
    assert(FindParameter<int64_t>(commonParams, Parameters::Sound::FRAMEDURATION));
  }
  
  void UpdateRenderParameters(const ParametersMap& updates, RenderParameters& renderParams)
  {
    if (const int64_t* freq = FindParameter<int64_t>(updates, Parameters::Sound::FREQUENCY))
    {
      renderParams.SoundFreq = static_cast<unsigned>(*freq);
    }
    if (const int64_t* clock = FindParameter<int64_t>(updates, Parameters::Sound::CLOCKRATE))
    {
      renderParams.ClockFreq = static_cast<uint64_t>(*clock);
    }
    if (const int64_t* frame = FindParameter<int64_t>(updates, Parameters::Sound::FRAMEDURATION))
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
      , Channels(0), Holder(), Player(), FilterObject(), Renderer(new BufferRenderer(Buffer))
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
        
        boost::shared_ptr<Module::Player> tmpPlayer = boost::shared_ptr<Module::Player>(new SafePlayerWrapper(holder->CreatePlayer(), PlayerMutex));
        Locker lock(PlayerMutex);
        StopPlayback();
        Holder.swap(holder);
        Player.swap(tmpPlayer);

        Channels = modInfo.PhysicalChannels;
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

    Error BackendImpl::SetParameters(const ParametersMap& params)
    {
      try
      {
        Locker lock(PlayerMutex);
        ParametersMap updates;
        std::set_difference(params.begin(), params.end(),
          CommonParameters.begin(), CommonParameters.end(), std::inserter(updates, updates.end()),
          CompareParameter);
        UpdateRenderParameters(updates, RenderingParameters);
        OnParametersChanged(updates);
        Player->SetParameters(updates);
        //merge result back
        {
          ParametersMap merged;
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
    
    Error BackendImpl::GetParameters(ParametersMap& params) const
    {
      Locker lock(PlayerMutex);
      params = CommonParameters;
      return Error();
    }

    //internal functions
    void BackendImpl::CheckState() const
    {
      ThrowIfError(RenderError);
      if (NOTOPENED == CurrentState)
      {
        throw Error(THIS_LINE, BACKEND_CONTROL_ERROR, TEXT_SOUND_ERROR_BACKEND_INVALID_STATE);
      }
      assert(Holder && Player);
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
            const bool stopping = !SafeRenderFrame();
            OnBufferReady(Buffer);
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
