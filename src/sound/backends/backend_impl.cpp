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
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::Base");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");

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
      Buffer.reserve(1000);//seems to be enough in most cases
    }

    virtual void ApplyData(const OutputSample& samp)
    {
      Buffer.push_back(samp);
    }

    virtual void Flush()
    {
    }
  private:
    Chunk& Buffer;
  };

  typedef boost::shared_ptr<Chunk> ChunkPtr;

  class Renderer
  {
  public:
    typedef boost::shared_ptr<Renderer> Ptr;

    Renderer(Module::Renderer::Ptr renderer, ChunkPtr buffer)
      : Source(renderer)
      , State(Source->GetTrackState())
      , Buffer(buffer)
    {
    }

    bool RenderFrame(BackendCallback& callback, BackendWorker& worker)
    {
      callback.OnFrame(*State);
      Buffer->clear();
      const bool res = Source->RenderFrame();
      worker.BufferReady(*Buffer);
      return res;
    }
  private:
    const Module::Renderer::Ptr Source;
    const Module::TrackState::Ptr State;
    const ChunkPtr Buffer;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(Module::Holder::Ptr holder, BackendWorker::Ptr worker, BackendCallback::Ptr callback, Renderer::Ptr render)
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
      Playing = Render->RenderFrame(*Callback, *Delegate);
    }
  private:
    const Module::Holder::Ptr Holder;
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

    virtual void OnStart(Module::Holder::Ptr module)
    {
      First->OnStart(module);
      Second->OnStart(module);
    }

    virtual void OnFrame(const Module::TrackState& state)
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
    virtual void OnStart(Module::Holder::Ptr /*module*/)
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

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
    {
      worker->Test();
      const Module::Holder::Ptr holder = params->GetModule();
      const ChunkPtr buffer = boost::make_shared<Chunk>();
      const Receiver::Ptr target = boost::make_shared<BufferRenderer>(boost::ref(*buffer));
      const Module::Renderer::Ptr moduleRenderer = boost::make_shared<SafeRendererWrapper>(holder->CreateRenderer(params->GetParameters(), target));
      const Renderer::Ptr renderer = boost::make_shared<Renderer>(moduleRenderer, buffer);
      const Async::Worker::Ptr asyncWorker = boost::make_shared<AsyncWrapper>(holder, worker, CreateCallback(params, worker), renderer);
      const Async::Job::Ptr job = Async::CreateJob(asyncWorker);
      return boost::make_shared<BackendInternal>(worker, moduleRenderer, job);
    }

    BackendCallback::Ptr CreateCompositeCallback(BackendCallback::Ptr first, BackendCallback::Ptr second)
    {
      return boost::make_shared<CompositeBackendCallback>(first, second);
    }
  }
}
