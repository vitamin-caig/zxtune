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

#include "async/worker.h"
#include "debug/log.h"
#include "sound/parameters.h"
#include "sound/render_params.h"

#include "error_tools.h"
#include "make_ptr.h"
#include "pointers.h"

#include <atomic>
#include <mutex>
#include <utility>

namespace Sound::BackendBase
{
  const Debug::Stream Dbg("Sound::Backend::Base");

  class PlaybackState
  {
  public:
    using Ptr = std::shared_ptr<PlaybackState>;

    void ChunkCreated(const Sound::Chunk& chunk)
    {
      const std::scoped_lock lock(Guard);
      if (FftAnalyzer)
      {
        Pending.assign(chunk.begin(), chunk.end());
      }
      else
      {
        Pending.clear();
      }
    }

    void ChunkCommitted(const Module::State& state)
    {
      const std::scoped_lock lock(Guard);
      State = state;
      if (FftAnalyzer && !Pending.empty())
      {
        FftAnalyzer->FeedSound(Pending.data(), Pending.size());
        Pending.clear();
      }
    }

    Sound::Analyzer::Ptr GetAnalyzer() const
    {
      const std::scoped_lock lock(Guard);
      if (!FftAnalyzer)
      {
        FftAnalyzer = FFTAnalyzer::Create();
      }
      return FftAnalyzer;
    }

    Module::State GetState() const
    {
      const std::scoped_lock lock(Guard);
      return State;
    }

  private:
    mutable std::mutex Guard;
    mutable FFTAnalyzer::Ptr FftAnalyzer;
    Module::State State;
    Sound::Chunk Pending;
  };

  class RendererWrapper
  {
  public:
    using Ptr = std::shared_ptr<RendererWrapper>;
    using Result = std::pair<Module::State, Sound::Chunk>;

    explicit RendererWrapper(Module::Renderer::Ptr delegate)
      : Delegate(std::move(delegate))
      , SeekRequest(NO_SEEK)
    {}

    Result Render()
    {
      const auto request = SeekRequest.exchange(NO_SEEK);
      if (request != NO_SEEK)
      {
        Delegate->SetPosition(Time::AtMillisecond(request));
      }
      const auto state = Delegate->GetState();
      auto data = Delegate->Render();
      return {state, std::move(data)};
    }

    void SetPosition(Time::AtMillisecond request)
    {
      SeekRequest = request.Get();
    }

  private:
    static const uint_t NO_SEEK = ~uint_t(0);
    const Module::Renderer::Ptr Delegate;
    std::atomic<uint_t> SeekRequest;
  };

  class AsyncWrapper : public Async::Worker
  {
  public:
    AsyncWrapper(BackendCallback::Ptr callback, RendererWrapper::Ptr render, BackendWorker::Ptr worker,
                 PlaybackState::Ptr playback)
      : Callback(std::move(callback))
      , Renderer(std::move(render))
      , Worker(std::move(worker))
      , State(std::move(playback))
      , Playing(false)
    {}

    void Initialize() override
    {
      try
      {
        Dbg("Initializing");
        Worker->Startup();
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
        Worker->Shutdown();
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
        Worker->Shutdown();
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
        Worker->Pause();
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
        Worker->Resume();
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
        auto [state, data] = Renderer->Render();
        if ((Playing = !data.empty()))
        {
          State->ChunkCreated(data);
          Worker->PlayFrame(state, std::move(data));
          Callback->OnFrame(state);
          State->ChunkCommitted(state);
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
    const BackendCallback::Ptr Callback;
    const RendererWrapper::Ptr Renderer;
    const BackendWorker::Ptr Worker;
    const PlaybackState::Ptr State;
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

  BackendCallback::Ptr CreateCallback(BackendCallback::Ptr callback)
  {
    static StubBackendCallback STUB;
    return callback ? std::move(callback) : MakeSingletonPointer(STUB);
  }

  class ControlInternal : public PlaybackControl
  {
  public:
    using Ptr = std::shared_ptr<ControlInternal>;

    ControlInternal(Async::Job::Ptr job, RendererWrapper::Ptr renderer, Sound::BackendBase::PlaybackState::Ptr playback)
      : Job(std::move(job))
      , Renderer(std::move(renderer))
      , State(std::move(playback))
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

    Analyzer::Ptr GetAnalyzer() const
    {
      return State->GetAnalyzer();
    }

    Module::State GetModuleState() const override
    {
      return State->GetState();
    }

    PlaybackControl::PlaybackState GetPlaybackState() const override
    {
      return Job->IsActive() ? (Job->IsPaused() ? PAUSED : STARTED) : STOPPED;
    }

  private:
    const Async::Job::Ptr Job;
    const RendererWrapper::Ptr Renderer;
    const Sound::BackendBase::PlaybackState::Ptr State;
  };

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(BackendWorker::Ptr worker, PlaybackState::Ptr playback, RendererWrapper::Ptr renderer,
                    Async::Job::Ptr job)
      : Worker(std::move(worker))
      , Control(MakePtr<ControlInternal>(std::move(job), std::move(renderer), std::move(playback)))
    {}

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Control->GetAnalyzer();
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
    const ControlInternal::Ptr Control;
  };
}  // namespace Sound::BackendBase

namespace Sound
{
  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr globalParams, const Module::Holder::Ptr& holder,
                             BackendCallback::Ptr origCallback, BackendWorker::Ptr worker)
  {
    auto origRenderer = Module::CreatePipelinedRenderer(*holder, std::move(globalParams));
    auto callback = BackendBase::CreateCallback(std::move(origCallback));
    auto renderer = MakePtr<BackendBase::RendererWrapper>(std::move(origRenderer));
    auto playback = MakePtr<BackendBase::PlaybackState>();
    auto asyncWorker = MakePtr<BackendBase::AsyncWrapper>(std::move(callback), renderer, worker, playback);
    auto job = Async::CreateJob(std::move(asyncWorker));
    return MakePtr<BackendBase::BackendInternal>(std::move(worker), std::move(playback), std::move(renderer),
                                                 std::move(job));
  }
}  // namespace Sound
