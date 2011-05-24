/*
Abstract:
  Playback support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_supp.h"
#include "playlist/supp/data.h"
#include "ui/utils.h"
#include "ui/tools/errordialog.h"
//common includes
#include <error.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
//boost inlcudes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QMutex>

namespace
{
  bool IsNullBackend(const ZXTune::Sound::BackendCreator& creator)
  {
    static const Char NULL_BACKEND_ID[] = {'n', 'u', 'l', 'l', 0};
    return creator.Id() == NULL_BACKEND_ID;
  }

  void ShowErrors(const std::list<Error>& errors)
  {
    const QString title(PlaybackSupport::tr("<b>Error while initializing sound subsystem</b>"));
    std::for_each(errors.begin(), errors.end(), boost::bind(&ShowErrorMessage, title, _1));
  }

  const ZXTune::Sound::MultiGain MIXER3[] =
  {
    { {1.0, 0.0} },
    { {0.5, 0.5} },
    { {0.0, 1.0} }
  };
  const ZXTune::Sound::MultiGain MIXER4[] =
  {
    { {1.0, 0.0} },
    { {0.7, 0.3} },
    { {0.3, 0.7} },
    { {0.0, 1.0} }
  };

  ZXTune::Sound::Mixer::Ptr CreateMixer(const ZXTune::Sound::MultiGain* matrix, uint_t chans)
  {
    ZXTune::Sound::Mixer::Ptr res;
    ThrowIfError(ZXTune::Sound::CreateMixer(chans, res));
    std::vector<ZXTune::Sound::MultiGain> mtx(matrix, matrix + chans);
    ThrowIfError(res->SetMatrix(mtx));
    return res;
  }

  class BackendParams : public ZXTune::Sound::CreateBackendParameters
  {
  public:
    BackendParams(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
      : Params(params)
      , Module(module)
      , Info(Module->GetModuleInformation())
      , Properties(Module->GetModuleProperties())
    {
    }

    virtual Parameters::Accessor::Ptr GetParameters() const
    {
      return Parameters::CreateMergedAccessor(Properties, Params);
    }

    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      return Module;
    }

    virtual ZXTune::Sound::Mixer::Ptr GetMixer() const
    {
      switch (Info->PhysicalChannels())
      {
      case 3:
        return CreateMixer(MIXER3, 3);
      case 4:
        return CreateMixer(MIXER4, 4);
      default:
        return ZXTune::Sound::Mixer::Ptr();
      }
    }

    virtual ZXTune::Sound::Converter::Ptr GetFilter() const
    {
      return ZXTune::Sound::Converter::Ptr();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const ZXTune::Module::Holder::Ptr Module;
    const ZXTune::Module::Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  ZXTune::Sound::Backend::Ptr CreateBackend(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
  {
    using namespace ZXTune;
    //create backend
    const Sound::CreateBackendParameters::Ptr createParams(new BackendParams(params, module));
    Sound::Backend::Ptr result;
    std::list<Error> errors;
    for (Sound::BackendCreator::Iterator::Ptr backends = Sound::EnumerateBackends();
      backends->IsValid(); backends->Next())
    {
      const Sound::BackendCreator::Ptr creator = backends->Get();
      if (IsNullBackend(*creator))
      {
        ShowErrors(errors);
        return Sound::Backend::Ptr();
      }
      if (const Error& err = creator->CreateBackend(createParams, result))
      {
        errors.push_back(err);
      }
      else
      {
        break;
      }
    }
    return result;
  }

  class PlaybackSupportImpl : public PlaybackSupport
  {
  public:
    PlaybackSupportImpl(QObject& parent, Parameters::Accessor::Ptr sndOptions)
      : PlaybackSupport(parent)
      , Params(sndOptions)
    {
    }

    virtual ~PlaybackSupportImpl()
    {
      this->wait();
    }

    virtual void SetItem(const Playlist::Item::Data& item)
    {
      Backend = CreateBackend(Params, item.GetModule());
      if (Backend)
      {
        OnSetBackend(Backend);
        this->wait();
        Backend->Play();
        this->start();
      }
    }

    virtual void Play()
    {
      //play only if any module selected or set
      if (Backend)
      {
        Backend->Play();
        this->start();
      }
    }

    virtual void Stop()
    {
      if (Backend)
      {
        Backend->Stop();
        this->wait();
      }
    }

    virtual void Pause()
    {
      if (!Backend)
      {
        return;
      }
      const ZXTune::Sound::Backend::State curState = Backend->GetCurrentState();
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
      if (Backend)
      {
        Backend->SetPosition(frame);
      }
    }

    virtual void run()
    {
      using namespace ZXTune;

      //notify about start
      OnStartModule(Backend);

      const Async::Signals::Collector::Ptr signaller = Backend->CreateSignalsCollector(
        Sound::Backend::MODULE_RESUME | Sound::Backend::MODULE_PAUSE |
        Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH);
      for (;;)
      {
        uint_t sigmask = 0;
        if (signaller->WaitForSignals(sigmask, 100/*10fps*/))
        {
          if (sigmask & (Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH))
          {
            if (sigmask & Sound::Backend::MODULE_FINISH)
            {
              OnFinishModule();
            }
            break;
          }
          else if (sigmask & Sound::Backend::MODULE_RESUME)
          {
            OnResumeModule();
          }
          else if (sigmask & Sound::Backend::MODULE_PAUSE)
          {
            OnPauseModule();
          }
          else
          {
            assert(!"Invalid signal");
          }
        }
        const Sound::Backend::State curState = Backend->GetCurrentState();
        if (Sound::Backend::STARTED != curState)
        {
          continue;
        }
        OnUpdateState();
      }
      //notify about stop
      OnStopModule();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    ZXTune::Sound::Backend::Ptr Backend;
  };
}

PlaybackSupport::PlaybackSupport(QObject& parent) : QThread(&parent)
{
}

PlaybackSupport* PlaybackSupport::Create(QObject& parent, Parameters::Accessor::Ptr sndOptions)
{
  REGISTER_METATYPE(ZXTune::Sound::Backend::Ptr);
  return new PlaybackSupportImpl(parent, sndOptions);
}
