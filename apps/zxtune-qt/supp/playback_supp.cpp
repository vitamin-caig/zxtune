/**
 *
 * @file
 *
 * @brief Playback support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "playback_supp.h"
#include "playlist/supp/data.h"
#include "supp/thread_utils.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <error.h>
#include <lazy.h>
#include <pointers.h>
// library includes
#include <parameters/merged_accessor.h>
#include <sound/service.h>
// std includes
#include <utility>
// qt includes
#include <QtCore/QTimer>

namespace
{
  class StubControl : public Sound::PlaybackControl
  {
  public:
    void Play() override {}

    void Pause() override {}

    void Stop() override {}

    void SetPosition(Time::AtMillisecond /*request*/) override {}

    State GetCurrentState() const override
    {
      return STOPPED;
    }

    static Ptr Instance()
    {
      static StubControl instance;
      return MakeSingletonPointer(instance);
    }
  };

  class PlaybackSupportImpl
    : public PlaybackSupport
    , private Sound::BackendCallback
  {
  public:
    PlaybackSupportImpl(QObject& parent, Parameters::Accessor::Ptr sndOptions)
      : PlaybackSupport(parent)
      , Service(&Sound::CreateSystemService, std::move(sndOptions))
      , Control(StubControl::Instance())
    {
      const unsigned UI_UPDATE_FPS = 5;
      Timer.setInterval(1000 / UI_UPDATE_FPS);
      Require(connect(this, &PlaybackSupport::OnStartModule, &Timer,
                      [timer = &Timer](Sound::Backend::Ptr, Playlist::Item::Data::Ptr) { timer->start(); }));
      Require(connect(this, &PlaybackSupport::OnStopModule, &Timer, &QTimer::stop));
      Require(connect(&Timer, &QTimer::timeout, this, &PlaybackSupport::OnUpdateState));
    }

    void SetDefaultItem(Playlist::Item::Data::Ptr item) override
    {
      IOThread::Execute([this, item]() {
        if (Backend)
        {
          return;
        }
        LoadItem(item);
      });
    }

    void SetItem(Playlist::Item::Data::Ptr item) override
    {
      IOThread::Execute([this, item]() {
        try
        {
          if (LoadItem(item))
          {
            Control->Play();
          }
        }
        catch (const Error& e)
        {
          emit ErrorOccurred(e);
        }
      });
    }

    void ResetItem() override
    {
      Stop();
      Item.reset();
      Control = StubControl::Instance();
      Backend.reset();
    }

    void Play() override
    {
      try
      {
        Control->Play();
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    void Stop() override
    {
      try
      {
        Control->Stop();
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    void Pause() override
    {
      try
      {
        const Sound::PlaybackControl::State curState = Control->GetCurrentState();
        if (Sound::PlaybackControl::STARTED == curState)
        {
          Control->Pause();
        }
        else if (Sound::PlaybackControl::PAUSED == curState)
        {
          Control->Play();
        }
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    void Seek(Time::AtMillisecond request) override
    {
      try
      {
        Control->SetPosition(request);
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    // BackendCallback
    void OnStart() override
    {
      emit OnStartModule(Backend, Item);
    }

    void OnFrame(const Module::State& /*state*/) override {}

    void OnStop() override
    {
      emit OnStopModule();
    }

    void OnPause() override
    {
      emit OnPauseModule();
    }

    void OnResume() override
    {
      emit OnResumeModule();
    }

    void OnFinish() override
    {
      emit OnFinishModule();
    }

  private:
    bool LoadItem(Playlist::Item::Data::Ptr item)
    {
      const Module::Holder::Ptr module = item->GetModule();
      if (!module)
      {
        return false;
      }
      try
      {
        ResetItem();
        Backend = CreateBackend(module);
        if (Backend)
        {
          Control = Backend->GetPlaybackControl();
          Item = std::move(item);
          return true;
        }
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
      return false;
    }

    Sound::Backend::Ptr CreateBackend(const Module::Holder::Ptr& module)
    {
      // create backend
      const Sound::BackendCallback::Ptr cb(static_cast<Sound::BackendCallback*>(this),
                                           NullDeleter<Sound::BackendCallback>());
      std::list<Error> errors;
      const auto systemBackends = Service->GetAvailableBackends();
      for (const auto& id : systemBackends)
      {
        try
        {
          return Service->CreateBackend(id, module, cb);
        }
        catch (const Error& err)
        {
          errors.push_back(err);
        }
      }
      ReportErrors(errors);
      return {};
    }

    void ReportErrors(const std::list<Error>& errors)
    {
      for (const auto& err : errors)
      {
        emit ErrorOccurred(err);
      }
    }

  private:
    const Lazy<Sound::Service::Ptr> Service;
    QTimer Timer;
    Playlist::Item::Data::Ptr Item;
    Sound::Backend::Ptr Backend;
    Sound::PlaybackControl::Ptr Control;
  };
}  // namespace

PlaybackSupport::PlaybackSupport(QObject& parent)
  : QObject(&parent)
{}

PlaybackSupport* PlaybackSupport::Create(QObject& parent, Parameters::Accessor::Ptr sndOptions)
{
  REGISTER_METATYPE(Sound::Backend::Ptr);
  REGISTER_METATYPE(Error);
  return new PlaybackSupportImpl(parent, std::move(sndOptions));
}
