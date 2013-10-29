/**
* 
* @file
*
* @brief Convert operation implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "operations_convert.h"
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

    virtual void OnStart()
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

  //TODO: simplify
  class ConvertVisitor : public Playlist::Item::Visitor
  {
  public:
    ConvertVisitor(uint_t totalItems, const String& type, Sound::Service::Ptr service, Log::ProgressCallback& cb, Playlist::Item::ConversionResultNotification::Ptr result)
      : TotalItems(totalItems)
      , DoneItems(0)
      , Callback(cb)
      , Type(type)
      , Service(service)
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
        const Sound::Backend::Ptr backend = Service->CreateBackend(Type, item, Sound::BackendCallback::Ptr(&cb, NullDeleter<Sound::BackendCallback>()));
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
    const String Type;
    const Sound::Service::Ptr Service;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  class ConvertOperation : public Playlist::Item::TextResultOperation
  {
  public:
    ConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Sound::Service::Ptr service, Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems(items)
      , Type(type)
      , Service(service)
      , Result(result)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      ConvertVisitor visitor(totalItems, Type, Service, cb, Result);
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
    const String Type;
    const Sound::Service::Ptr Service;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };
}

namespace Playlist
{
  namespace Item
  {
    TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Sound::Service::Ptr service, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ConvertOperation>(items, type, service, result);
    }
  }
}
