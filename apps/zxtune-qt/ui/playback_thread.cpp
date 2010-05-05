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
      : Stopped(false)
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

    virtual ~PlaybackThreadImpl()
    {
      Stop();
    }

    virtual void SetItem(const ModuleItem& item)
    {
      {
        QMutexLocker lock(&Sync);
        Stopped = true;
        Backend->Stop();
        this->wait();
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
        //stopped
        this->start();
      }
      else
      {
        //played or paused
        Backend->Play();
      }
    }

    virtual void Stop()
    {
      {
        QMutexLocker lock(&Sync);
        Stopped = true;
        Backend->Stop();
      }
      this->wait();
    }

    virtual void Pause()
    {
      QMutexLocker lock(&Sync);
      //toggle play/pause
      ZXTune::Sound::Backend::State curState = ZXTune::Sound::Backend::STARTED;
      Backend->GetCurrentState(curState);
      if (ZXTune::Sound::Backend::STARTED == curState)
      {
        Backend->Pause();
      }
      else if (ZXTune::Sound::Backend::PAUSED == curState)
      {
        Backend->Play();
      }
    }

    virtual void Seek(int frame)
    {
      Backend->SetPosition(frame);
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
          ZXTune::Sound::Backend::State curState = ZXTune::Sound::Backend::STARTED;
          Backend->GetCurrentState(curState);
          bool donePause = false;
          if (ZXTune::Sound::Backend::STOPPED == curState)
          {
            //stop
            break;
          }
          else if (ZXTune::Sound::Backend::PAUSED == curState && !donePause)
          {
            //pause
            OnPauseModule(CurrentInfo);
            donePause = true;
          }
          else if (ZXTune::Sound::Backend::STARTED == curState)
          {
            if (donePause)
            {
              //resume playback
              OnResumeModule(CurrentInfo);
              donePause = false;
            }
            //playing
            uint_t time = 0;
            ZXTune::Module::Tracking tracking;
            ZXTune::Module::Analyze::ChannelsState state;
            player->GetPlaybackState(time, tracking, state);
            OnUpdateState(static_cast<uint>(time), tracking, state);
          }
          this->msleep(100);//10fps
        }
        if (!Stopped)
        {
          OnFinishModule(CurrentInfo);
        }
        OnStopModule(CurrentInfo);
        Stopped = false;
      }
    }
  private:
    ZXTune::Sound::Backend::Ptr Backend;
    ZXTune::Module::Information CurrentInfo;
    QMutex Sync;
    //TODO: make right
    volatile bool Stopped;
  };
}

PlaybackThread* PlaybackThread::Create(QWidget* owner)
{
  assert(owner);
  return new PlaybackThreadImpl(owner);
}
