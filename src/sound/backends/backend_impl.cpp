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
#include <boost/ref.hpp>

#define FILE_TAG B3D60DB5

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Base");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
  class BufferRenderer : public Receiver
  {
  public:
    explicit BufferRenderer(BackendWorker& worker)
      : Worker(worker)
    {
    }

    virtual void ApplyData(const Chunk::Ptr& chunk)
    {
      Worker.FrameFinish(chunk);
    }

    virtual void Flush()
    {
    }
  private:
    BackendWorker& Worker;
  };

  class CallbackOverWorker : public BackendCallback
  {
  public:
    CallbackOverWorker(BackendCallback::Ptr delegate, BackendWorker::Ptr worker)
      : Delegate(delegate)
      , Worker(worker)
    {
    }

    virtual void OnStart()
    {
      Worker->Startup();
      Delegate->OnStart();
    }

    virtual void OnFrame(const Module::TrackState& state)
    {
      Worker->FrameStart(state);
      Delegate->OnFrame(state);
    }

    virtual void OnStop()
    {
      Worker->Shutdown();
      Delegate->OnStop();
    }

    virtual void OnPause()
    {
      Worker->Pause();
      Delegate->OnPause();
    }

    virtual void OnResume()
    {
      Worker->Resume();
      Delegate->OnResume();
    }

    virtual void OnFinish()
    {
      Delegate->OnFinish();
    }
  private:
    const BackendCallback::Ptr Delegate;
    const BackendWorker::Ptr Worker;
  };

  class RendererWrapper : public Module::Renderer
  {
  public:
    RendererWrapper(Module::Renderer::Ptr delegate, BackendCallback::Ptr callback)
      : Delegate(delegate)
      , Callback(callback)
      , State(Delegate->GetTrackState())
      , SeekRequest(NO_SEEK)
    {
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Delegate->GetAnalyzer();
    }

    virtual bool RenderFrame()
    {
      if (SeekRequest != NO_SEEK)
      {
        Delegate->SetPosition(SeekRequest);
        SeekRequest = NO_SEEK;
      }
      Callback->OnFrame(*State);
      return Delegate->RenderFrame();
    }

    virtual void Reset()
    {
      SeekRequest = NO_SEEK;
      Delegate->Reset();
    }

    virtual void SetPosition(uint_t frame)
    {
      SeekRequest = frame;
    }
  private:
    static const uint_t NO_SEEK = ~uint_t(0);
    const Module::Renderer::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Module::TrackState::Ptr State;
    //TODO: use atomic variable
    volatile uint_t SeekRequest;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(BackendCallback::Ptr callback, Module::Renderer::Ptr render)
      : Callback(callback)
      , Render(render)
      , Playing(false)
    {
    }

    virtual void Initialize()
    {
      try
      {
        Dbg("Initializing");
        Render->Reset();
        Callback->OnStart();
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
      Playing = Render->RenderFrame();
    }
  private:
    const BackendWorker::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Module::Renderer::Ptr Render;
    volatile bool Playing;
  };

  class StubBackendCallback : public BackendCallback
  {
  public:
    virtual void OnStart()
    {
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
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

  BackendCallback::Ptr CreateCallback(BackendCallback::Ptr callback, BackendWorker::Ptr worker)
  {
    static StubBackendCallback STUB;
    const BackendCallback::Ptr cb = callback ? callback : BackendCallback::Ptr(&STUB, NullDeleter<BackendCallback>());
    return boost::make_shared<CallbackOverWorker>(cb, worker);
  }

  class ControlInternal : public PlaybackControl
  {
  public:
    ControlInternal(Async::Job::Ptr job, Module::Renderer::Ptr renderer)
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
    const Module::Renderer::Ptr Renderer;
  };

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(BackendWorker::Ptr worker, Module::Renderer::Ptr renderer, Async::Job::Ptr job)
      : Worker(worker)
      , Renderer(renderer)
      , Control(boost::make_shared<ControlInternal>(job, renderer))
    {
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return Renderer->GetTrackState();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
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
    const Module::Renderer::Ptr Renderer;
    const PlaybackControl::Ptr Control;
  };
}

namespace Sound
{
  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder, BackendCallback::Ptr origCallback, BackendWorker::Ptr worker)
  {
    const Receiver::Ptr target = boost::make_shared<BufferRenderer>(boost::ref(*worker));
    const Module::Renderer::Ptr origRenderer = holder->CreateRenderer(params, target);
    const BackendCallback::Ptr callback = CreateCallback(origCallback, worker);
    const Module::Renderer::Ptr renderer = boost::make_shared<RendererWrapper>(origRenderer, callback);
    const Async::Worker::Ptr asyncWorker = boost::make_shared<AsyncWrapper>(callback, renderer);
    const Async::Job::Ptr job = Async::CreateJob(asyncWorker);
    return boost::make_shared<BackendInternal>(worker, renderer, job);
  }
}
