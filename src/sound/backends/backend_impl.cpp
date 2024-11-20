/**
 *
 * @file
 *
 * @brief  Backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/backend_impl.h"

#include "module/players/pipeline.h"
#include "sound/backends/l10n.h"
#include "sound/impl/fft_analyzer.h"

#include "async/job.h"
#include "async/worker.h"
#include "debug/log.h"
#include "l10n/api.h"
#include "module/renderer.h"
#include "module/state.h"
#include "sound/analyzer.h"
#include "sound/chunk.h"
#include "time/instant.h"

#include "error.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include <atomic>
#include <exception>
#include <utility>

namespace Sound::BackendBase
{
  const Debug::Stream Dbg("Sound::Backend::Base");

  class CallbackOverWorker : public BackendCallback
  {
  public:
    CallbackOverWorker(BackendCallback::Ptr delegate, BackendWorker::Ptr worker)
      : Delegate(std::move(delegate))
      , Worker(std::move(worker))
    {}

    void OnStart() override
    {
      Worker->Startup();
      Delegate->OnStart();
    }

    void OnFrame(const Module::State& state) override
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
    using Ptr = std::shared_ptr<RendererWrapper>;

    RendererWrapper(Module::Renderer::Ptr delegate, BackendCallback::Ptr callback)
      : Delegate(std::move(delegate))
      , Callback(std::move(callback))
      , State(Delegate->GetState())
      , SeekRequest(NO_SEEK)
      , Analyzer(FFTAnalyzer::Create())
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Analyzer::Ptr GetFftAnalyzer() const
    {
      return Analyzer;
    }

    Sound::Chunk Render() override
    {
      const auto request = SeekRequest.exchange(NO_SEEK);
      if (request != NO_SEEK)
      {
        Delegate->SetPosition(Time::AtMillisecond(request));
      }
      Callback->OnFrame(*State);
      auto result = Delegate->Render();
      Analyzer->FeedSound(result.data(), result.size());
      return result;
    }

    void Reset() override
    {
      SeekRequest = NO_SEEK;
      Delegate->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      SeekRequest = request.Get();
    }

  private:
    static const uint_t NO_SEEK = ~uint_t(0);
    const Module::Renderer::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Module::State::Ptr State;
    std::atomic<uint_t> SeekRequest;
    const FFTAnalyzer::Ptr Analyzer;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(BackendCallback::Ptr callback, Module::Renderer::Ptr render, BackendWorker::Ptr worker)
      : Callback(std::move(callback))
      , Renderer(std::move(render))
      , Worker(std::move(worker))
      , Playing(false)
    {}

    void Initialize() override
    {
      try
      {
        Dbg("Initializing");
        Callback->OnStart();
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to initialize playback.")).AddSuberror(e);
      }
      try
      {
        // initial frame rendering
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
      try
      {
        auto data = Renderer->Render();
        if (!data.empty())
        {
          Playing = true;
          Worker->FrameFinish(std::move(data));
        }
        else
        {
          Playing = false;
        }
      }
      catch (const std::exception& e)
      {
        Playing = false;
        throw Error(THIS_LINE, e.what());
      }
      catch (const Error& e)
      {
        Playing = false;
        throw;
      }
    }

  private:
    const BackendWorker::Ptr Delegate;
    const BackendCallback::Ptr Callback;
    const Module::Renderer::Ptr Renderer;
    const BackendWorker::Ptr Worker;
    std::atomic<bool> Playing;
  };

  class StubBackendCallback : public BackendCallback
  {
  public:
    void OnStart() override {}

    void OnFrame(const Module::State& /*state*/) override {}

    void OnStop() override {}

    void OnPause() override {}

    void OnResume() override {}

    void OnFinish() override {}
  };

  BackendCallback::Ptr CreateCallback(BackendCallback::Ptr callback, BackendWorker::Ptr worker)
  {
    static StubBackendCallback STUB;
    auto&& cb = callback ? std::move(callback) : MakeSingletonPointer(STUB);
    return MakePtr<CallbackOverWorker>(std::move(cb), std::move(worker));
  }

  class ControlInternal : public PlaybackControl
  {
  public:
    ControlInternal(Async::Job::Ptr job, Module::Renderer::Ptr renderer)
      : Job(std::move(job))
      , Renderer(std::move(renderer))
    {}

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

    void SetPosition(Time::AtMillisecond request) override
    {
      try
      {
        Renderer->SetPosition(request);
      }
      catch (const Error& e)
      {
        throw Error(THIS_LINE, translate("Failed to set playback position.")).AddSuberror(e);
      }
    }

    State GetCurrentState() const override
    {
      return Job->IsActive() ? (Job->IsPaused() ? PAUSED : STARTED) : STOPPED;
    }

  private:
    const Async::Job::Ptr Job;
    const Module::Renderer::Ptr Renderer;
  };

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(BackendWorker::Ptr worker, RendererWrapper::Ptr renderer, Async::Job::Ptr job)
      : Worker(std::move(worker))
      , Renderer(std::move(renderer))
      , Control(MakePtr<ControlInternal>(std::move(job), Renderer))
    {}

    Module::State::Ptr GetState() const override
    {
      return Renderer->GetState();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Renderer->GetFftAnalyzer();
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
    const RendererWrapper::Ptr Renderer;
    const PlaybackControl::Ptr Control;
  };
}  // namespace Sound::BackendBase

namespace Sound
{
  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr globalParams, const Module::Holder::Ptr& holder,
                             BackendCallback::Ptr origCallback, BackendWorker::Ptr worker)
  {
    auto origRenderer = Module::CreatePipelinedRenderer(*holder, std::move(globalParams));
    auto callback = BackendBase::CreateCallback(std::move(origCallback), worker);
    auto renderer = MakePtr<BackendBase::RendererWrapper>(std::move(origRenderer), callback);
    auto asyncWorker = MakePtr<BackendBase::AsyncWrapper>(std::move(callback), renderer, worker);
    auto job = Async::CreateJob(std::move(asyncWorker));
    return MakePtr<BackendBase::BackendInternal>(std::move(worker), std::move(renderer), std::move(job));
  }
}  // namespace Sound
