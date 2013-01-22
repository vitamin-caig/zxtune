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
  //TODO: simplify this shitcode
  class BackendParams : public ZXTune::Sound::CreateBackendParameters
  {
  public:
    BackendParams(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module, ZXTune::Sound::BackendCallback::Ptr callback)
      : Params(params)
      , Module(module)
      , Properties(Module->GetModuleProperties())
      , Callback(callback)
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

    virtual ZXTune::Sound::Converter::Ptr GetFilter() const
    {
      return ZXTune::Sound::Converter::Ptr();
    }

    virtual ZXTune::Sound::BackendCallback::Ptr GetCallback() const
    {
      return Callback;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const ZXTune::Module::Holder::Ptr Module;
    const Parameters::Accessor::Ptr Properties;
    const ZXTune::Sound::BackendCallback::Ptr Callback;
  };

  class PlaybackSupportImpl : public PlaybackSupport
                            , private ZXTune::Sound::BackendCallback
  {
  public:
    PlaybackSupportImpl(QObject& parent, Parameters::Accessor::Ptr sndOptions)
      : PlaybackSupport(parent)
      , Params(sndOptions)
    {
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
          Backend->Play();
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
        Backend->Play();
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
        Backend->Stop();
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
          Backend->Pause();
        }
        else if (ZXTune::Sound::Backend::PAUSED == curState)
        {
          Backend->Play();
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
        Backend->SetPosition(frame);
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    //BackendCallback
    virtual void OnStart(ZXTune::Module::Holder::Ptr /*module*/)
    {
      LastUpdateTime = std::clock();
      emit OnStartModule(Backend, Item);
    }

    virtual void OnFrame(const ZXTune::Module::TrackState& /*state*/)
    {
      const std::clock_t curTime = std::clock();
      if (curTime - LastUpdateTime >= CLOCKS_PER_SEC / 10/*fps*/)
      {
        LastUpdateTime = curTime;
        emit OnUpdateState();
      }
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
    ZXTune::Sound::Backend::Ptr CreateBackend(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module)
    {
      using namespace ZXTune;
      //create backend
      const ZXTune::Sound::BackendCallback::Ptr cb(static_cast<ZXTune::Sound::BackendCallback*>(this), NullDeleter<ZXTune::Sound::BackendCallback>());
      const Sound::CreateBackendParameters::Ptr createParams = CreateBackendParameters(params, module, cb);
      std::list<Error> errors;
      const Sound::BackendsScope::Ptr systemBackends = Sound::BackendsScope::CreateSystemScope(params);
      for (Sound::BackendCreator::Iterator::Ptr backends = systemBackends->Enumerate();
        backends->IsValid(); backends->Next())
      {
        const Sound::BackendCreator::Ptr creator = backends->Get();
        Sound::Backend::Ptr result;
        try
        {
          return creator->CreateBackend(createParams);
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
    const Parameters::Accessor::Ptr Params;
    Playlist::Item::Data::Ptr Item;
    ZXTune::Sound::Backend::Ptr Backend;
    std::clock_t LastUpdateTime;
  };
}

PlaybackSupport::PlaybackSupport(QObject& parent) : QObject(&parent)
{
}

PlaybackSupport* PlaybackSupport::Create(QObject& parent, Parameters::Accessor::Ptr sndOptions)
{
  REGISTER_METATYPE(ZXTune::Sound::Backend::Ptr);
  REGISTER_METATYPE(Error);
  return new PlaybackSupportImpl(parent, sndOptions);
}

ZXTune::Sound::CreateBackendParameters::Ptr CreateBackendParameters(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module, ZXTune::Sound::BackendCallback::Ptr callback)
{
  return boost::make_shared<BackendParams>(params, module, callback);
}
