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

    bool RenderFrame(BackendCallback& callback, BackendWorker& worker)
    {
      callback.OnFrame(*State);
      Buffer.clear();
      Buffer.reserve(1000);//seems to be enough in most cases
      const bool res = Source->RenderFrame();
      if (!res)
      {
        Mix->Flush();
      }
      worker.BufferReady(Buffer);
      return res;
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
    AsyncWrapper(Module::Holder::Ptr holder, BackendWorker::Ptr worker, BackendCallback::Ptr callback, Renderer::Ptr render)
      : Holder(holder)
      , Delegate(worker)
      , Callback(callback)
      , Render(render)
      , Playing(false)
    {
    }

    virtual Error Initialize()
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
        Delegate->Shutdown();
        Callback->OnStop();
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
        Callback->OnPause();
        Delegate->Pause();
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
        Callback->OnResume();
        Delegate->Resume();
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
        RenderFrame();
        if (IsFinished())
        {
          Callback->OnFinish();
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

  class BackendInternal : public Backend
  {
  public:
    BackendInternal(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker)
      : Worker(worker)
      , Mix(CreateMixer(*params))
      , Holder(params->GetModule())
      , Renderer(new SafeRendererWrapper(Holder->CreateRenderer(params->GetParameters(), Mix)))
      , Job(Async::CreateJob(Async::Worker::Ptr(new AsyncWrapper(Holder, Worker, CreateCallback(params, Worker), Renderer::Create(Renderer, Mix)))))
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

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return Worker->GetVolumeControl();
    }
  private:
    const BackendWorker::Ptr Worker;
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

    BackendCallback::Ptr CreateCompositeCallback(BackendCallback::Ptr first, BackendCallback::Ptr second)
    {
      return boost::make_shared<CompositeBackendCallback>(first, second);
    }
  }
}
