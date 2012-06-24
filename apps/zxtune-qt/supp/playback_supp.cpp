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
#include "mixer.h"
#include "playlist/supp/data.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
//boost inlcudes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QMutex>

namespace
{
  class BackendParams : public ZXTune::Sound::CreateBackendParameters
  {
  public:
    BackendParams(PlaybackSupport* supp, Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
      : Supp(supp)
      , Params(params)
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
      return Supp
        ? CreateMixer(*Supp, Params, Info->PhysicalChannels())
        : CreateMixer(Params, Info->PhysicalChannels());
    }

    virtual ZXTune::Sound::Converter::Ptr GetFilter() const
    {
      return ZXTune::Sound::Converter::Ptr();
    }
  private:
    PlaybackSupport* const Supp;
    const Parameters::Accessor::Ptr Params;
    const ZXTune::Module::Holder::Ptr Module;
    const ZXTune::Module::Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

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

    virtual void SetItem(Playlist::Item::Data::Ptr item)
    {
      const ZXTune::Module::Holder::Ptr module = item->GetModule();
      if (!module)
      {
        return;
      }
      try
      {
        Stop();
        Backend.reset();
        Backend = CreateBackend(Params, module);
        if (Backend)
        {
          Item = item;
          OnSetBackend(Backend);
          this->wait();
          ThrowIfError(Backend->Play());
          this->start();
        }
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    virtual void Play()
    {
      //play only if any module selected or set
      if (!Backend)
      {
        return;
      }
      try
      {
        ThrowIfError(Backend->Play());
        this->start();
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    virtual void Stop()
    {
      if (!Backend)
      {
        return;
      }
      try
      {
        ThrowIfError(Backend->Stop());
        this->wait();
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    virtual void Pause()
    {
      if (!Backend)
      {
        return;
      }
      try
      {
        const ZXTune::Sound::Backend::State curState = Backend->GetCurrentState();
        if (ZXTune::Sound::Backend::STARTED == curState)
        {
          ThrowIfError(Backend->Pause());
        }
        else if (ZXTune::Sound::Backend::PAUSED == curState)
        {
          ThrowIfError(Backend->Play());
        }
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    virtual void Seek(int frame)
    {
      if (!Backend)
      {
        return;
      }
      try
      {
        ThrowIfError(Backend->SetPosition(frame));
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    virtual void run()
    {
      try
      {
        using namespace ZXTune;

        //notify about start
        OnStartModule(Backend, Item);

        const Async::Signals::Collector::Ptr signaller = Backend->CreateSignalsCollector(
          Sound::Backend::MODULE_RESUME | Sound::Backend::MODULE_PAUSE |
          Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH);
        for (;;)
        {
          if (uint_t sigmask = signaller->WaitForSignals(100/*10fps*/))
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
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }
  private:
    ZXTune::Sound::Backend::Ptr CreateBackend(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
    {
      using namespace ZXTune;
      //create backend
      const Sound::CreateBackendParameters::Ptr createParams = CreateBackendParameters(*this, params, module);
      std::list<Error> errors;
      const Sound::BackendsScope::Ptr systemBackends = Sound::BackendsScope::CreateSystemScope(params);
      for (Sound::BackendCreator::Iterator::Ptr backends = systemBackends->Enumerate();
        backends->IsValid(); backends->Next())
      {
        const Sound::BackendCreator::Ptr creator = backends->Get();
        Sound::Backend::Ptr result;
        if (const Error& err = creator->CreateBackend(createParams, result))
        {
          errors.push_back(err);
        }
        else
        {
          return result;
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
    const Parameters::Accessor::Ptr Params;
    Playlist::Item::Data::Ptr Item;
    ZXTune::Sound::Backend::Ptr Backend;
  };
}

PlaybackSupport::PlaybackSupport(QObject& parent) : QThread(&parent)
{
}

PlaybackSupport* PlaybackSupport::Create(QObject& parent, Parameters::Accessor::Ptr sndOptions)
{
  REGISTER_METATYPE(ZXTune::Sound::Backend::Ptr);
  REGISTER_METATYPE(Error);
  return new PlaybackSupportImpl(parent, sndOptions);
}

ZXTune::Sound::CreateBackendParameters::Ptr CreateBackendParameters(PlaybackSupport& supp, Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
{
  return boost::make_shared<BackendParams>(&supp, params, module);
}

ZXTune::Sound::CreateBackendParameters::Ptr CreateBackendParameters(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
{
  return boost::make_shared<BackendParams>(static_cast<PlaybackSupport*>(0), params, module);
}
