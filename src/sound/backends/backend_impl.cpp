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
//library includes
#include <async/worker.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

#define FILE_TAG B3D60DB5

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Base");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
  class SafeRendererWrapper : public ZXTune::Module::Renderer
  {
  public:
    explicit SafeRendererWrapper(ZXTune::Module::Renderer::Ptr player)
      : Delegate(player)
    {
      if (!Delegate.get())
      {
        throw Error(THIS_LINE, translate("Invalid module specified for backend."));
      }
    }

    virtual ZXTune::Module::TrackState::Ptr GetTrackState() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Delegate->GetTrackState();
    }

    virtual ZXTune::Module::Analyzer::Ptr GetAnalyzer() const
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
    const ZXTune::Module::Renderer::Ptr Delegate;
    mutable boost::mutex Mutex;
  };

  class BufferRenderer : public Receiver
  {
  public:
    explicit BufferRenderer(BackendWorker& worker)
      : Worker(worker)
    {
    }

    virtual void ApplyData(const Chunk::Ptr& chunk)
    {
      Worker.BufferReady(chunk);
    }

    virtual void Flush()
    {
    }
  private:
    BackendWorker& Worker;
  };

  class Renderer
  {
  public:
    typedef boost::shared_ptr<Renderer> Ptr;

    Renderer(ZXTune::Module::Renderer::Ptr renderer)
      : Source(renderer)
      , State(Source->GetTrackState())
    {
    }

    bool RenderFrame(BackendCallback& callback)
    {
      callback.OnFrame(*State);
      return Source->RenderFrame();
    }
  private:
    const ZXTune::Module::Renderer::Ptr Source;
    const ZXTune::Module::TrackState::Ptr State;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(ZXTune::Module::Holder::Ptr holder, BackendWorker::Ptr worker, BackendCallback::Ptr callback, Renderer::Ptr render)
      : Holder(holder)
      , Delegate(worker)
      , Callback(callback)
      , Render(render)
      , Playing(false)
    {
    }

    virtual void Initialize()
    {
      try
      {
        Dbg("Initializing");
        Callback->OnStart(Holder);
        Delegate->Startup();
        Playing = true;
        //initial frame rendering
        RenderFrame();
        Dbg("Initialized");
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to initialize playback.")).AddSuberror(e);
      }
    }

    virtual void Finalize()
    {
      try
      {
        Dbg("Finalizing");
        Playing = false;
        Delegate->Shutdown();
        Callback->OnStop();
        Dbg("Finalized");
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to finalize playback.")).AddSuberror(e);
      }
    }

    virtual void Suspend()
    {
      try
      {
        Dbg("Suspending");
        Callback->OnPause();
        Delegate->Pause();
        Dbg("Suspended");
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to pause playback.")).AddSuberror(e);
      }
    }

    virtual void Resume() 
    {
      try
      {
        Dbg("Resuming");
        Callback->OnResume();
        Delegate->Resume();
        Dbg("Resumed");
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to resume playback.")).AddSuberror(e);
      }
    }

    virtual void ExecuteCycle()
    {
      RenderFrame();
      if (IsFinished())
      {
        Callback->OnFinish();
      }
    }

    virtual bool IsFinished() const
    {
      return !Playing;
    }
  private:
    void RenderFrame()
    {
      Playing = Render->RenderFrame(*Callback);
    }
  private:
    const ZXTune::Module::Holder::Ptr Holder;
    const BackendWorker::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Renderer::Ptr Render;
    bool Playing;
  };

  class CompositeBackendCallback : public BackendCallback
  {
  public:
    CompositeBackendCallback(BackendCallback::Ptr first, BackendCallback::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual void OnStart(ZXTune::Module::Holder::Ptr module)
    {
      First->OnStart(module);
      Second->OnStart(module);
    }

    virtual void OnFrame(const ZXTune::Module::TrackState& state)
    {
      First->OnFrame(state);
      Second->OnFrame(state);
    }

    virtual void OnStop()
    {
      First->OnStop();
      Second->OnStop();
    }

    virtual void OnPause()
    {
      First->OnPause();
      Second->OnPause();
    }

    virtual void OnResume()
    {
      First->OnResume();
      Second->OnResume();
    }

    virtual void OnFinish()
    {
      First->OnFinish();
      Second->OnFinish();
    }
  private:
    const BackendCallback::Ptr First;
    const BackendCallback::Ptr Second;
  };

  class StubBackendCallback : public BackendCallback
  {
  public:
    virtual void OnStart(ZXTune::Module::Holder::Ptr /*module*/)
    {
    }

    virtual void OnFrame(const ZXTune::Module::TrackState& /*state*/)
    {
    }

    virtual void OnStop()
    {
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnFinish()
    {
    }
  };

  BackendCallback::Ptr CreateCallback(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
  {
    const BackendCallback::Ptr user = params->GetCallback();
    const BackendCallback::Ptr work = boost::dynamic_pointer_cast<BackendCallback>(worker);
    if (user && work)
    {
      return boost::make_shared<CompositeBackendCallback>(user, work);
    }
    else if (user)
    {
      return user;
    }
    else if (work)
    {
      return work;
    }
    else
    {
      static StubBackendCallback STUB;
      return BackendCallback::Ptr(&STUB, NullDeleter<BackendCallback>());
    }
  }

  class ControlInternal : public PlaybackControl
  {
  public:
    ControlInternal(Async::Job::Ptr job, ZXTune::Module::Renderer::Ptr renderer)
      : Job(job)
      , Renderer(renderer)
    {
    }

    virtual void Play()
    {
      Job->Start();
    }

    virtual void Pause()
    {
      Job->Pause();
    }

    virtual void Stop()
    {
      try
      {
        Job->Stop();
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
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to set playback position.")).AddSuberror(e);
      }
    }

    virtual State GetCurrentState() const
    {
      return Job->IsActive()
        ? (Job->IsPaused() ? PAUSED : STARTED)
        : STOPPED;
    }
  private:
    const Async::Job::Ptr Job;
    const ZXTune::Module::Renderer::Ptr Renderer;
  };

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(BackendWorker::Ptr worker, ZXTune::Module::Renderer::Ptr renderer, Async::Job::Ptr job)
      : Worker(worker)
      , Renderer(renderer)
      , Control(boost::make_shared<ControlInternal>(job, renderer))
    {
    }

    virtual ZXTune::Module::TrackState::Ptr GetTrackState() const
    {
      return Renderer->GetTrackState();
    }

    virtual ZXTune::Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Renderer->GetAnalyzer();
    }

    virtual PlaybackControl::Ptr GetPlaybackControl() const
    {
      return Control;
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return Worker->GetVolumeControl();
    }
  private:
    const BackendWorker::Ptr Worker;
    const ZXTune::Module::Renderer::Ptr Renderer;
    const PlaybackControl::Ptr Control;
  };
}

namespace Sound
{
  Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
  {
    worker->Test();
    const ZXTune::Module::Holder::Ptr holder = params->GetModule();
    const Receiver::Ptr target = boost::make_shared<BufferRenderer>(boost::ref(*worker));
    const ZXTune::Module::Renderer::Ptr moduleRenderer = boost::make_shared<SafeRendererWrapper>(holder->CreateRenderer(params->GetParameters(), target));
    const Renderer::Ptr renderer = boost::make_shared<Renderer>(moduleRenderer);
    const Async::Worker::Ptr asyncWorker = boost::make_shared<AsyncWrapper>(holder, worker, CreateCallback(params, worker), renderer);
    const Async::Job::Ptr job = Async::CreateJob(asyncWorker);
    return boost::make_shared<BackendInternal>(worker, moduleRenderer, job);
  }

  BackendCallback::Ptr CreateCompositeCallback(BackendCallback::Ptr first, BackendCallback::Ptr second)
  {
    return boost::make_shared<CompositeBackendCallback>(first, second);
  }
}
