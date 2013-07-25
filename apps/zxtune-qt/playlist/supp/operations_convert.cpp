/*
Abstract:
  Playlist convert operations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "operations.h"
#include "storage.h"
#include <apps/zxtune-qt/supp/playback_supp.h>
//common includes
#include <contract.h>
#include <error_tools.h>
//library includes
#include <async/src/event.h>
#include <sound/backend.h>
#include <strings/format.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  //TODO: rework
  class ConvertCallback : public Sound::BackendCallback
  {
    enum EventType
    {
      STOPPED = 1,
      CANCELED = 2
    };
  public:
    explicit ConvertCallback(Log::ProgressCallback& callback)
      : Callback(callback)
      , Event()
    {
    }

    virtual void OnStart(Module::Holder::Ptr /*module*/)
    {
      Event.Reset();
    }

    virtual void OnFrame(const Module::TrackState& state)
    {
      try
      {
        Callback.OnProgress(state.Frame());
      }
      catch (const std::exception&)
      {
        Event.Set(CANCELED);
      }
    }

    virtual void OnStop()
    {
      Event.Set(STOPPED);
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

    void WaitForFinish()
    {
      if (Event.WaitForAny(STOPPED, CANCELED) == CANCELED)
      {
        throw std::exception();
      }
    }
  private:
    Log::ProgressCallback& Callback;
    Async::Event<uint_t> Event;
  };

  class ConvertVisitor : public Playlist::Item::Visitor
  {
  public:
    ConvertVisitor(uint_t totalItems, Sound::BackendCreator::Ptr creator, Parameters::Accessor::Ptr params, Log::ProgressCallback& cb, Playlist::Item::ConversionResultNotification::Ptr result)
      : TotalItems(totalItems)
      , DoneItems(0)
      , Callback(cb)
      , Creator(creator)
      , BackendParameters(params)
      , Result(result)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      const String path = data->GetFullPath();
      if (Module::Holder::Ptr holder = data->GetModule())
      {
        ConvertItem(path, holder);
      }
      else
      {
        Result->AddFailedToOpen(path);
      }
      ++DoneItems;
    }
  private:
    void ConvertItem(const String& path, Module::Holder::Ptr item)
    {
      try
      {
        const Log::ProgressCallback::Ptr curItemProgress = Log::CreateNestedPercentProgressCallback(TotalItems, DoneItems, Callback);
        const Module::Information::Ptr info = item->GetModuleInformation();
        const Log::ProgressCallback::Ptr framesProgress = Log::CreatePercentProgressCallback(info->FramesCount(), *curItemProgress);
        ConvertCallback cb(*framesProgress);
        const Sound::CreateBackendParameters::Ptr params = MakeBackendParameters(BackendParameters, item,
          Sound::BackendCallback::Ptr(&cb, NullDeleter<Sound::BackendCallback>()));
        const Sound::Backend::Ptr backend = Creator->CreateBackend(params);
        const Sound::PlaybackControl::Ptr control = backend->GetPlaybackControl();
        control->Play();
        cb.WaitForFinish();
        control->Stop();
        curItemProgress->OnProgress(100);
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }
  private:
    const uint_t TotalItems;
    uint_t DoneItems;
    Log::ProgressCallback& Callback;
    const Sound::BackendCreator::Ptr Creator;
    const Parameters::Accessor::Ptr BackendParameters;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  Sound::BackendCreator::Ptr FindBackendCreator(const String& id)
  {
    for (Sound::BackendCreator::Iterator::Ptr backends = Sound::EnumerateBackends(); backends->IsValid(); backends->Next())
    {
      const Sound::BackendCreator::Ptr creator = backends->Get();
      if (creator->Id() == id)
      {
        return creator;
      }
    }
    return Sound::BackendCreator::Ptr();
  }


  class ConvertOperation : public Playlist::Item::TextResultOperation
  {
  public:
    ConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Parameters::Accessor::Ptr params, Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems(items)
      , Creator(FindBackendCreator(type))
      , Params(params)
      , Result(result)
    {
      Require(Creator);
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      ConvertVisitor visitor(totalItems, Creator, Params, cb, Result);
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, visitor);
      }
      else
      {
        stor.ForAllItems(visitor);
      }
      emit ResultAcquired(Result);
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const Sound::BackendCreator::Ptr Creator;
    const Parameters::Accessor::Ptr Params;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };
}

namespace Playlist
{
  namespace Item
  {
    TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ConvertOperation>(items, type, params, result);
    }
  }
}
