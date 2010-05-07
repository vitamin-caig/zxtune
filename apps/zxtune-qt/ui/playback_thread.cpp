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
      : Quit(false)
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
          { {1.0, 0.0} },
          { {0.5, 0.5} },
          { {0.0, 1.0} }
        };
        static const MultiGain MIXER4[] =
        {
          { {1.0, 0.0} },
          { {0.7, 0.3} },
          { {0.3, 0.7} },
          { {0.0, 1.0} }
        };
        Backend->SetMixer(std::vector<MultiGain>(MIXER3, ArrayEnd(MIXER3)));
        Backend->SetMixer(std::vector<MultiGain>(MIXER4, ArrayEnd(MIXER4)));
      }
      this->start();
    }

    virtual ~PlaybackThreadImpl()
    {
      Quit = true;
      this->wait();
    }

    virtual void SetItem(const ModuleItem& item)
    {
      Backend->SetModule(item.Module);
      Backend->Play();
    }

    virtual void Play()
    {
      Backend->Play();
    }

    virtual void Stop()
    {
      Backend->Stop();
    }

    virtual void Pause()
    {
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
      using namespace ZXTune;
      SignalsCollector::Ptr signaller = Backend->CreateSignalsCollector(
        Sound::Backend::MODULE_OPEN | 
        Sound::Backend::MODULE_START | Sound::Backend::MODULE_RESUME | 
        Sound::Backend::MODULE_PAUSE | Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH);
      //global state
      Sound::Backend::State state = Sound::Backend::NOTOPENED;
      Module::Player::ConstWeakPtr player;
      Module::Information info;
      //playback state, just for optimization
      uint_t time = 0;
      Module::Tracking tracking;
      Module::Analyze::ChannelsState analyze;
      while (!Quit)
      {
        uint_t sigmask = 0;
        while (signaller->WaitForSignals(sigmask, 100/*10fps*/))
        {
          Backend->GetCurrentState(state);
          if (sigmask & Sound::Backend::MODULE_OPEN)
          {
            player = Backend->GetPlayer();
            if (Module::Player::ConstPtr realPlayer = player.lock())
            {
              //TODO: make more reliable way
              realPlayer->GetModule().GetModuleInformation(info);
            }
          }
          //TODO: list
          else if (sigmask & Sound::Backend::MODULE_START)
          {
            OnStartModule(info);
          }
          else if (sigmask & Sound::Backend::MODULE_RESUME)
          {
            OnResumeModule(info);
          }
          else if (sigmask & Sound::Backend::MODULE_PAUSE)
          {
            OnPauseModule(info);
          }
          else if (sigmask & Sound::Backend::MODULE_STOP)
          {
            OnStopModule(info);
          }
          else if (sigmask & Sound::Backend::MODULE_FINISH)
          {
            OnFinishModule(info);
          }
          else
          {
            assert(!"Invalid signal");
          }
        }
        if (Sound::Backend::STARTED != state)
        {
          continue;
        }
        if (Module::Player::ConstPtr realPlayer = player.lock())
        {
          realPlayer->GetPlaybackState(time, tracking, analyze);
          OnUpdateState(static_cast<uint>(time), tracking, analyze);
        }
      }
    }
  private:
    ZXTune::Sound::Backend::Ptr Backend;
    volatile bool Quit;
  };
}

PlaybackThread* PlaybackThread::Create(QWidget* owner)
{
  assert(owner);
  return new PlaybackThreadImpl(owner);
}
