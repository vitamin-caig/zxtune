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
#include <async/job.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
//text includes
#include <sound/text/sound.h>

#define FILE_TAG B3D60DB5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("BackendBase");

  class SafeRendererWrapper : public Module::Renderer
  {
  public:
    explicit SafeRendererWrapper(Module::Renderer::Ptr player)
      : Delegate(player)
    {
      if (!Delegate.get())
      {
        throw Error(THIS_LINE, BACKEND_FAILED_CREATE, Text::SOUND_ERROR_BACKEND_INVALID_MODULE);
      }
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->GetTrackState();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->GetAnalyzer();
    }

    virtual bool RenderFrame(const Sound::RenderParameters& params)
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->RenderFrame(params);
    }

    virtual void Reset()
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->Reset();
    }

    virtual void SetPosition(uint_t frame)
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->SetPosition(frame);
    }
  private:
    const Module::Renderer::Ptr Delegate;
    mutable boost::mutex Mutex;
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

  class MixerWithFilter : public Mixer
  {
  public:
    MixerWithFilter(Mixer::Ptr mixer, Converter::Ptr filter)
      : Delegate(mixer)
      , Filter(filter)
    {
      Delegate->SetTarget(Filter);
    }
    
    virtual void ApplyData(const Mixer::InDataType& data)
    {
      Delegate->ApplyData(data);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(DataReceiver<Mixer::OutDataType>::Ptr target)
    {
      Filter->SetTarget(target);
    }

    virtual Error SetMatrix(const std::vector<MultiGain>& data)
    {
      return Delegate->SetMatrix(data);
    }
  private:
    const Mixer::Ptr Delegate;
    const Converter::Ptr Filter;
  };

  Mixer::Ptr CreateMixer(const CreateBackendParameters& params)
  {
    const Mixer::Ptr mixer = params.GetMixer();
    const Converter::Ptr filter = params.GetFilter();
    return filter ? boost::make_shared<MixerWithFilter>(mixer, filter) : mixer;
  }

  class Renderer
  {
    Renderer(RenderParameters::Ptr renderParams, Module::Renderer::Ptr renderer, Mixer::Ptr mixer)
      : RenderingParameters(renderParams)
      , Source(renderer)
      , State(Source->GetTrackState())
      , Mix(mixer)
    {
      const Receiver::Ptr target(new BufferRenderer(Buffer));
      Mix->SetTarget(target);
    }
  public:
    typedef boost::shared_ptr<Renderer> Ptr;

    static Ptr Create(const CreateBackendParameters& params, Module::Renderer::Ptr renderer, Mixer::Ptr mixer)
    {
      const RenderParameters::Ptr renderParams = RenderParameters::Create(params.GetParameters());
      return Renderer::Ptr(new Renderer(renderParams, renderer, mixer));
    }

    bool ApplyFrame()
    {
      Buffer.reserve(RenderingParameters->SamplesPerFrame());
      Buffer.clear();
      const bool res = Source->RenderFrame(*RenderingParameters);
      if (!res)
      {
        Mix->Flush();
      }
      return res;
    }

    std::vector<MultiSample>& GetBuffer()
    {
      return Buffer;
    }

    const Module::TrackState& GetState() const
    {
      return *State;
    }
  private:
    const RenderParameters::Ptr RenderingParameters;
    const Module::Renderer::Ptr Source;
    const Module::TrackState::Ptr State;
    const Mixer::Ptr Mix;
    std::vector<MultiSample> Buffer;
  };

  class AsyncWrapper : public Async::Job::Worker
  {
  public:
    AsyncWrapper(Module::Holder::Ptr holder, BackendWorker::Ptr worker, Async::Signals::Dispatcher& signaller, Renderer::Ptr render)
      : Holder(holder)
      , Delegate(worker)
      , Signaller(signaller)
      , Render(render)
      , Playing(false)
    {
    }

    virtual Error Initialize()
    {
      try
      {
        Delegate->OnStartup(*Holder);
        Playing = true;
        Signaller.Notify(Backend::MODULE_START);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_PLAYBACK).AddSuberror(e);
      }
    }

    virtual Error Finalize()
    {
      try
      {
        Playing = false;
        Signaller.Notify(Backend::MODULE_STOP);
        Delegate->OnShutdown();
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_STOP).AddSuberror(e);
      }
    }

    virtual Error Suspend()
    {
      try
      {
        Delegate->OnPause();
        Signaller.Notify(Backend::MODULE_PAUSE);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_PAUSE).AddSuberror(e);
      }
    }

    virtual Error Resume() 
    {
      try
      {
        Delegate->OnResume();
        Signaller.Notify(Backend::MODULE_RESUME);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_PLAYBACK).AddSuberror(e);
      }
    }

    virtual Error ExecuteCycle()
    {
      try
      {
        Delegate->OnFrame(Render->GetState());
        Playing = Render->ApplyFrame();
        Delegate->OnBufferReady(Render->GetBuffer());
        if (IsFinished())
        {
          Signaller.Notify(Backend::MODULE_FINISH);
        }
        return Error();
      }
      catch (const Error& err)
      {
        return err;
      }
    }

    virtual bool IsFinished() const
    {
      return !Playing;
    }
  private:
    const Module::Holder::Ptr Holder;
    const BackendWorker::Ptr Delegate;
    Async::Signals::Dispatcher& Signaller;
    const Renderer::Ptr Render;
    bool Playing;
  };

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
      : Worker(worker)
      , Signaller(Async::Signals::Dispatcher::Create())
      , Mix(CreateMixer(*params))
      , Holder(params->GetModule())
      , Renderer(new SafeRendererWrapper(Holder->CreateRenderer(Holder->GetModuleProperties(), Mix)))
      , Job(Async::Job::Create(Async::Job::Worker::Ptr(new AsyncWrapper(Holder, Worker, *Signaller, Renderer::Create(*params, Renderer, Mix)))))
    {
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      return Holder->GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Holder->GetModuleProperties();
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return Renderer->GetTrackState();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Renderer->GetAnalyzer();
    }

    virtual Error Play()
    {
      return Job->Start();
    }

    virtual Error Pause()
    {
      return Job->Pause();
    }

    virtual Error Stop()
    {
      return Job->Stop();
    }

    virtual Error SetPosition(uint_t frame)
    {
      try
      {
        Renderer->SetPosition(frame);
        Signaller->Notify(Backend::MODULE_SEEK);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, BACKEND_CONTROL_ERROR, Text::SOUND_ERROR_BACKEND_SEEK).AddSuberror(e);
      }
    }

    virtual State GetCurrentState() const
    {
      return Job->IsActive()
        ? (Job->IsPaused() ? Backend::PAUSED : Backend::STARTED)
        : Backend::STOPPED;
    }

    virtual Async::Signals::Collector::Ptr CreateSignalsCollector(uint_t signalsMask) const
    {
      return Signaller->CreateCollector(signalsMask);
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return Worker->GetVolumeControl();
    }
  private:
    const BackendWorker::Ptr Worker;
    const Async::Signals::Dispatcher::Ptr Signaller;
    const Mixer::Ptr Mix;
    const Module::Holder::Ptr Holder;
    const Module::Renderer::Ptr Renderer;
    const Async::Job::Ptr Job;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
    {
      worker->Test();
      return boost::make_shared<BackendInternal>(params, worker);
    }
  }
}
