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
#include <format.h>
//library includes
#include <async/signals_collector.h>
#include <sound/backend.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  class ConvertVisitor : public Playlist::Item::Visitor
  {
  public:
    ConvertVisitor(uint_t totalItems, ZXTune::Sound::BackendCreator::Ptr creator, Parameters::Accessor::Ptr params, Log::ProgressCallback& cb, Playlist::Item::ConversionResultNotification::Ptr result)
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
      if (ZXTune::Module::Holder::Ptr holder = data->GetModule())
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
    void ConvertItem(const String& path, ZXTune::Module::Holder::Ptr item)
    {
      try
      {
        const Log::ProgressCallback::Ptr curItemProgress = Log::CreateNestedPercentProgressCallback(TotalItems, DoneItems, Callback);
        const ZXTune::Module::Information::Ptr info = item->GetModuleInformation();
        const Log::ProgressCallback::Ptr framesProgress = Log::CreatePercentProgressCallback(info->FramesCount(), *curItemProgress);
        const ZXTune::Sound::CreateBackendParameters::Ptr params = CreateBackendParameters(BackendParameters, item);
        ZXTune::Sound::Backend::Ptr backend;
        ThrowIfError(Creator->CreateBackend(params, backend));
        Convert(*backend, *framesProgress);
        curItemProgress->OnProgress(100);
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }

    void Convert(ZXTune::Sound::Backend& backend, Log::ProgressCallback& callback) const
    {
      using namespace ZXTune::Sound;
      const uint_t flags = Backend::MODULE_STOP | Backend::MODULE_FINISH;
      const Async::Signals::Collector::Ptr collector = backend.CreateSignalsCollector(flags);
      const ZXTune::Module::TrackState::Ptr state = backend.GetTrackState();
      ThrowIfError(backend.Play());
      while (!collector->WaitForSignals(100))
      {
        callback.OnProgress(state->Frame());
      }
      ThrowIfError(backend.Stop());
    }

  private:
    const uint_t TotalItems;
    uint_t DoneItems;
    Log::ProgressCallback& Callback;
    const ZXTune::Sound::BackendCreator::Ptr Creator;
    const Parameters::Accessor::Ptr BackendParameters;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  ZXTune::Sound::BackendCreator::Ptr FindBackendCreator(const String& id)
  {
    using namespace ZXTune::Sound;
    for (BackendCreator::Iterator::Ptr backends = EnumerateBackends(); backends->IsValid(); backends->Next())
    {
      const BackendCreator::Ptr creator = backends->Get();
      if (creator->Id() != id)
      {
        continue;
      }
      return creator;
    }
    return BackendCreator::Ptr();
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
    const ZXTune::Sound::BackendCreator::Ptr Creator;
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
