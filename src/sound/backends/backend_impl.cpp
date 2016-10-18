/**
*
* @file
*
* @brief  Backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "backend_impl.h"
//common includes
#include <error_tools.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <async/worker.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <atomic>

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

    void ApplyData(const Chunk::Ptr& chunk) override
    {
      Worker.FrameFinish(chunk);
    }

    void Flush() override
    {
    }
  private:
    BackendWorker& Worker;
  };

  class CallbackOverWorker : public BackendCallback
  {
  public:
    CallbackOverWorker(BackendCallback::Ptr delegate, BackendWorker::Ptr worker)
      : Delegate(std::move(delegate))
      , Worker(std::move(worker))
    {
    }

    void OnStart() override
    {
      Worker->Startup();
      Delegate->OnStart();
    }

    void OnFrame(const Module::TrackState& state) override
    {
      Worker->FrameStart(state);
      Delegate->OnFrame(state);
    }

    void OnStop() override
    {
      Worker->Shutdown();
      Delegate->OnStop();
    }

    void OnPause() override
    {
      Worker->Pause();
      Delegate->OnPause();
    }

    void OnResume() override
    {
      Worker->Resume();
      Delegate->OnResume();
    }

    void OnFinish() override
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
      : Delegate(std::move(delegate))
      , Callback(std::move(callback))
      , State(Delegate->GetTrackState())
      , SeekRequest(NO_SEEK)
    {
    }

    Module::TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Delegate->GetAnalyzer();
    }

    bool RenderFrame() override
    {
      const uint_t request = SeekRequest.exchange(NO_SEEK);
      if (request != NO_SEEK)
      {
        Delegate->SetPosition(request);
      }
      Callback->OnFrame(*State);
      return Delegate->RenderFrame();
    }

    void Reset() override
    {
      SeekRequest = NO_SEEK;
      Delegate->Reset();
    }

    void SetPosition(uint_t frame) override
    {
      SeekRequest = frame;
    }
  private:
    static const uint_t NO_SEEK = ~uint_t(0);
    const Module::Renderer::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Module::TrackState::Ptr State;
    std::atomic<uint_t> SeekRequest;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(BackendCallback::Ptr callback, Module::Renderer::Ptr render)
      : Callback(std::move(callback))
      , Render(std::move(render))
      , Playing(false)
    {
    }

    void Initialize() override
    {
      try
      {
        Dbg("Initializing");
        Render->Reset();
        Callback->OnStart();
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to initialize playback.")).AddSuberror(e);
      }
      try
      {
        //initial frame rendering
        RenderFrame();
        Dbg("Initialized");
      }
      catch (const Error& e)
      {
        Dbg("Deinitialize backend worker due to error while rendering initial frame");
        Callback->OnStop();
        throw Error(THIS_LINE, translate("Failed to initialize playback.")).AddSuberror(e);
      }
    }

    void Finalize() override
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

    void Suspend() override
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

    void Resume() override 
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

    void ExecuteCycle() override
    {
      RenderFrame();
      if (IsFinished())
      {
        Callback->OnFinish();
      }
    }

    bool IsFinished() const override
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
    std::atomic<bool> Playing;
  };

  class StubBackendCallback : public BackendCallback
  {
  public:
    void OnStart() override
    {
    }

    void OnFrame(const Module::TrackState& /*state*/) override
    {
    }

    void OnStop() override
    {
    }

    void OnPause() override
    {
    }

    void OnResume() override
    {
    }

    void OnFinish() override
    {
    }
  };

  BackendCallback::Ptr CreateCallback(BackendCallback::Ptr callback, BackendWorker::Ptr worker)
  {
    static StubBackendCallback STUB;
    const BackendCallback::Ptr cb = callback ? callback : MakeSingletonPointer(STUB);
    return MakePtr<CallbackOverWorker>(cb, worker);
  }

  class ControlInternal : public PlaybackControl
  {
  public:
    ControlInternal(Async::Job::Ptr job, Module::Renderer::Ptr renderer)
      : Job(std::move(job))
      , Renderer(std::move(renderer))
    {
    }

    void Play() override
    {
      Job->Start();
    }

    void Pause() override
    {
      Job->Pause();
    }

    void Stop() override
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

    void SetPosition(uint_t frame) override
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

    State GetCurrentState() const override
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
      : Worker(std::move(worker))
      , Renderer(renderer)
      , Control(MakePtr<ControlInternal>(job, renderer))
    {
    }

    Module::TrackState::Ptr GetTrackState() const override
    {
      return Renderer->GetTrackState();
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Renderer->GetAnalyzer();
    }

    PlaybackControl::Ptr GetPlaybackControl() const override
    {
      return Control;
    }

    VolumeControl::Ptr GetVolumeControl() const override
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
    const Receiver::Ptr target = MakePtr<BufferRenderer>(*worker);
    const Module::Renderer::Ptr origRenderer = holder->CreateRenderer(params, target);
    const BackendCallback::Ptr callback = CreateCallback(origCallback, worker);
    const Module::Renderer::Ptr renderer = MakePtr<RendererWrapper>(origRenderer, callback);
    const Async::Worker::Ptr asyncWorker = MakePtr<AsyncWrapper>(callback, renderer);
    const Async::Job::Ptr job = Async::CreateJob(asyncWorker);
    return MakePtr<BackendInternal>(worker, renderer, job);
  }
}
