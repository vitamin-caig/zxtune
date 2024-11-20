/**
 *
 * @file
 *
 * @brief Playback support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/supp/playback_supp.h"

#include "apps/zxtune-qt/playlist/supp/data.h"
#include "apps/zxtune-qt/supp/thread_utils.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "module/holder.h"
#include "sound/service.h"

#include "contract.h"
#include "error.h"
#include "lazy.h"
#include "pointers.h"

#include <QtCore/QByteArrayData>
#include <QtCore/QTimer>

#include <atomic>
#include <list>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Module
{
  class State;
}  // namespace Module

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
      , ItemCookie(nullptr)
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
      if (!ItemCookie)
      {
        ItemCookie = item.get();
        SetItemAsync(std::move(item), false);
      }
    }

    void SetItem(Playlist::Item::Data::Ptr item) override
    {
      if (item == Item)
      {
        // Clicked the same item, just replay it
        Control->SetPosition({});
        Play();
      }
      else if (ItemCookie.exchange(item.get()) != item.get())
      {
        // Change the item
        SetItemAsync(std::move(item), true);
      }
    }

    void ResetItem() override
    {
      Stop();
      Item.reset();
      ItemCookie = nullptr;
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
      SelfThread::Execute(this, &PlaybackSupportImpl::ResetItem);
      emit OnFinishModule();
    }

  private:
    void SetItemAsync(Playlist::Item::Data::Ptr item, bool play)
    {
      IOThread::Execute([this, item, play]() { LoadItem(item, play); });
    }

    void LoadItem(Playlist::Item::Data::Ptr item, bool play)
    {
      // must not be in UI thread
      Require(!MainThread::IsCurrent());
      const Module::Holder::Ptr module = item->GetModule();
      if (!module || (item.get() != ItemCookie))
      {
        return;
      }

      try
      {
        auto backend = CreateBackend(module);
        if (!backend || item.get() != ItemCookie)
        {
          return;
        }
        SelfThread::Execute(this, &PlaybackSupportImpl::ApplyItem, std::move(item), std::move(backend), play);
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    void ApplyItem(Playlist::Item::Data::Ptr item, Sound::Backend::Ptr backend, bool play)
    {
      // must be in UI thread
      Require(MainThread::IsCurrent());
      ResetItem();
      Backend = std::move(backend);
      Control = Backend->GetPlaybackControl();
      Item = std::move(item);
      if (play)
      {
        Play();
      }
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
    std::atomic<const void*> ItemCookie;
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
