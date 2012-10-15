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
#include <debug_log.h>
#include <error_tools.h>
//library includes
#include <async/worker.h>
#include <l10n/api.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

#define FILE_TAG B3D60DB5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::Base");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  class SafeRendererWrapper : public Module::Renderer
  {
  public:
    explicit SafeRendererWrapper(Module::Renderer::Ptr player)
      : Delegate(player)
    {
      if (!Delegate.get())
      {
        throw Error(THIS_LINE, translate("Invalid module specified for backend."));
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

    virtual bool RenderFrame()
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->RenderFrame();
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
    explicit BufferRenderer(Chunk& buf) : Buffer(buf)
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
    Chunk& Buffer;
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
    Renderer(Module::Renderer::Ptr renderer, Mixer::Ptr mixer)
      : Source(renderer)
      , State(Source->GetTrackState())
      , Mix(mixer)
    {
      const Receiver::Ptr target(new BufferRenderer(Buffer));
      Mix->SetTarget(target);
    }
  public:
    typedef boost::shared_ptr<Renderer> Ptr;

    static Ptr Create(Module::Renderer::Ptr renderer, Mixer::Ptr mixer)
    {
      return Renderer::Ptr(new Renderer(renderer, mixer));
    }

    bool ApplyFrame()
    {
      Buffer.clear();
      Buffer.reserve(1000);//seems to be enough in most cases
      const bool res = Source->RenderFrame();
      if (!res)
      {
        Mix->Flush();
      }
      return res;
    }

    Chunk& GetBuffer()
    {
      return Buffer;
    }

    const Module::TrackState& GetState() const
    {
      return *State;
    }
  private:
    const Module::Renderer::Ptr Source;
    const Module::TrackState::Ptr State;
    const Mixer::Ptr Mix;
    Chunk Buffer;
  };

  class AsyncWrapper : public Async::Worker
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
        Dbg("Initializing");
        Delegate->OnStartup(*Holder);
        Playing = true;
        Signaller.Notify(Backend::MODULE_START);
        Dbg("Initialized");
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, translate("Failed to initialize playback.")).AddSuberror(e);
      }
    }

    virtual Error Finalize()
    {
      try
      {
        Dbg("Finalizing");
        Playing = false;
        Signaller.Notify(Backend::MODULE_STOP);
        Delegate->OnShutdown();
        Dbg("Finalized");
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, translate("Failed to finalize playback.")).AddSuberror(e);
      }
    }

    virtual Error Suspend()
    {
      try
      {
        Dbg("Suspending");
        Delegate->OnPause();
        Signaller.Notify(Backend::MODULE_PAUSE);
        Dbg("Suspended");
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, translate("Failed to pause playback.")).AddSuberror(e);
      }
    }

    virtual Error Resume() 
    {
      try
      {
        Dbg("Resuming");
        Delegate->OnResume();
        Signaller.Notify(Backend::MODULE_RESUME);
        Dbg("Resumed");
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, translate("Failed to resume playback.")).AddSuberror(e);
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
      , Renderer(new SafeRendererWrapper(Holder->CreateRenderer(params->GetParameters(), Mix)))
      , Job(Async::CreateJob(Async::Worker::Ptr(new AsyncWrapper(Holder, Worker, *Signaller, Renderer::Create(Renderer, Mix)))))
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

    virtual void Play()
    {
      ThrowIfError(Job->Start());
    }

    virtual void Pause()
    {
      ThrowIfError(Job->Pause());
    }

    virtual void Stop()
    {
      try
      {
        ThrowIfError(Job->Stop());
        Renderer->Reset();
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to stop playback.")).AddSuberror(e);
      }
    }

    virtual void SetPosition(uint_t frame)
    {
      try
      {
        Renderer->SetPosition(frame);
        Signaller->Notify(Backend::MODULE_SEEK);
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to set playback position.")).AddSuberror(e);
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
