/*
Abstract:
  Playback thread implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_thread.h"
#include "playback_thread_moc.h"
#include <apps/base/moduleitem.h>
//common includes
#include <error.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
//qt includes
#include <QtCore/QMutex>
#include <QtGui/QWidget>

namespace
{
  class PlaybackThreadImpl : public PlaybackThread
  {
  public:
    explicit PlaybackThreadImpl(QWidget* owner)
    {
      setParent(owner);

      //create backend
      {
        using namespace ZXTune::Sound;
        BackendInformationArray backends;
        EnumerateBackends(backends);
        Parameters::Map params;
        for (BackendInformationArray::const_iterator it = backends.begin(), lim = backends.end(); it != lim; ++it)
        {
          if (!CreateBackend(it->Id, params, Backend))
          {
            break;
          }
        }
        assert(Backend.get());
        static const MultiGain MIXER3[] =
        {
          {1.0, 0.0},
          {0.5, 0.5},
          {0.0, 1.0}
        };
        static const MultiGain MIXER4[] =
        {
          {1.0, 0.0},
          {0.7, 0.3},
          {0.3, 0.7},
          {0.0, 1.0}
        };
        Backend->SetMixer(std::vector<MultiGain>(MIXER3, ArrayEnd(MIXER3)));
        Backend->SetMixer(std::vector<MultiGain>(MIXER4, ArrayEnd(MIXER4)));
      }
    }

    virtual void SetItem(const ModuleItem& item)
    {
      {
        QMutexLocker lock(&Sync);
        Stop();
        item.Module->GetModuleInformation(CurrentInfo);
        Backend->SetModule(item.Module);
      }
      Play();
    }

    virtual void Play()
    {
      QMutexLocker lock(&Sync);
      if (!this->isRunning())
      {
        this->start();
      }
      else
      {
        Backend->Play();
      }
    }

    virtual void Stop()
    {
      Backend->Stop();
      this->wait();
    }

    virtual void Pause()
    {
      //TODO: handle toggle
      Backend->Pause();
    }

    virtual void run()
    {
      //sync with checked play
      {
        QMutexLocker lock(&Sync);
      }
      const ZXTune::Module::Player::ConstWeakPtr weakPlayer = Backend->GetPlayer();
      if (const ZXTune::Module::Player::ConstPtr player = weakPlayer.lock())
      {
        Backend->Play();
        OnStartModule(CurrentInfo);
        for(;;)
        {
          ZXTune::Sound::Backend::State state;
          Backend->GetCurrentState(state);
          if (ZXTune::Sound::Backend::STARTED == state)
          {
            uint_t time = 0;
            ZXTune::Module::Tracking tracking;
            ZXTune::Module::Analyze::ChannelsState state;
            player->GetPlaybackState(time, tracking, state);
            OnUpdateState(time, tracking, state);
          }
          //TODO: handle pause
          if (ZXTune::Sound::Backend::STOPPED == state ||
            ZXTune::Sound::Backend::STOP == Backend->WaitForEvent(ZXTune::Sound::Backend::STOP, 100/*10fps*/))
          {
            break;
          }
        }
        OnStopModule();
      }
    }
  private:
    ZXTune::Sound::Backend::Ptr Backend;
    ZXTune::Module::Information CurrentInfo;
    QMutex Sync;
  };
}

PlaybackThread* PlaybackThread::Create(QWidget* owner)
{
  assert(owner);
  return new PlaybackThreadImpl(owner);
}
