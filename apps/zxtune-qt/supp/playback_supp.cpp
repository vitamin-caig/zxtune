/**
* 
* @file
*
* @brief Playback support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "playback_supp.h"
#include "playlist/supp/data.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
#include <error.h>
#include <pointers.h>
//library includes
#include <parameters/merged_accessor.h>
#include <sound/service.h>
//qt includes
#include <QtCore/QTimer>

namespace
{
  class StubControl : public Sound::PlaybackControl
  {
  public:
    virtual void Play()
    {
    }

    virtual void Pause()
    {
    }

    virtual void Stop()
    {
    }

    virtual void SetPosition(uint_t /*frame*/)
    {
    }
    
    virtual State GetCurrentState() const
    {
      return STOPPED;
    }

    static Ptr Instance()
    {
      static StubControl instance;
      return MakeSingletonPointer(instance);
    }
  };

  class PlaybackSupportImpl : public PlaybackSupport
                            , private Sound::BackendCallback
  {
  public:
    PlaybackSupportImpl(QObject& parent, Parameters::Accessor::Ptr sndOptions)
      : PlaybackSupport(parent)
      , Service(Sound::CreateSystemService(sndOptions))
      , Control(StubControl::Instance())
    {
      const unsigned UI_UPDATE_FPS = 5;
      Timer.setInterval(1000 / UI_UPDATE_FPS);
      Require(Timer.connect(this, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(start())));
      Require(Timer.connect(this, SIGNAL(OnStopModule()), SLOT(stop())));
      Require(connect(&Timer, SIGNAL(timeout()), SIGNAL(OnUpdateState())));
    }

    virtual void SetDefaultItem(Playlist::Item::Data::Ptr item)
    {
      if (Backend)
      {
        return;
      }
      LoadItem(item);
    }

    virtual void SetItem(Playlist::Item::Data::Ptr item)
    {
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
    }

    virtual void ResetItem()
    {
      Stop();
      Control = StubControl::Instance();
      Backend.reset();
    }

    virtual void Play()
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

    virtual void Stop()
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

    virtual void Pause()
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

    virtual void Seek(int frame)
    {
      try
      {
        Control->SetPosition(frame);
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    //BackendCallback
    virtual void OnStart()
    {
      emit OnStartModule(Backend, Item);
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnStop()
    {
      emit OnStopModule();
    }

    virtual void OnPause()
    {
      emit OnPauseModule();
    }

    virtual void OnResume()
    {
      emit OnResumeModule();
    }

    virtual void OnFinish()
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
          Item = item;
          return true;
        }
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
      return false;
    }

    Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module)
    {
      //create backend
      const Sound::BackendCallback::Ptr cb(static_cast<Sound::BackendCallback*>(this), NullDeleter<Sound::BackendCallback>());
      std::list<Error> errors;
      const Strings::Array systemBackends = Service->GetAvailableBackends();
      for (Strings::Array::const_iterator it = systemBackends.begin(), lim = systemBackends.end(); it != lim; ++it)
      {
        try
        {
          return Service->CreateBackend(*it, module, cb);
        }
        catch (const Error& err)
        {
          errors.push_back(err);
        }
      }
      ReportErrors(errors);
      return Sound::Backend::Ptr();
    }

    void ReportErrors(const std::list<Error>& errors)
    {
      for (std::list<Error>::const_iterator it = errors.begin(), lim = errors.end(); it != lim; ++it)
      {
        emit ErrorOccurred(*it);
      }
    }
  private:
    const Sound::Service::Ptr Service;
    QTimer Timer;
    Playlist::Item::Data::Ptr Item;
    Sound::Backend::Ptr Backend;
    Sound::PlaybackControl::Ptr Control;
  };
}

PlaybackSupport::PlaybackSupport(QObject& parent) : QObject(&parent)
{
}

PlaybackSupport* PlaybackSupport::Create(QObject& parent, Parameters::Accessor::Ptr sndOptions)
{
  REGISTER_METATYPE(Sound::Backend::Ptr);
  REGISTER_METATYPE(Error);
  return new PlaybackSupportImpl(parent, sndOptions);
}
