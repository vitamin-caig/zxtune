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
#include <messages_collector.h>
//library includes
#include <async/signals_collector.h>
#include <sound/backend.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
//text includes
#include "text/text.h"

namespace
{
  class ConvertVisitor : public Playlist::Item::Visitor
                       , public Playlist::TextNotification
  {
  public:
    ConvertVisitor(uint_t totalItems, ZXTune::Sound::BackendCreator::Ptr creator, Parameters::Accessor::Ptr params, Log::ProgressCallback& cb)
      : TotalItems(totalItems)
      , Callback(cb)
      , Creator(creator)
      , BackendParameters(params)
      , Messages(Log::MessagesCollector::Create())
      , SucceedConvertions(0)
      , FailedConvertions(0)
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
        Failed(Strings::Format(Text::CONVERT_FAILED_OPEN, path));
      }
    }

    virtual String Category() const
    {
      return FromStdString("Convert");
    }

    virtual String Text() const
    {
      return Strings::Format(Text::CONVERT_STATUS, SucceedConvertions, FailedConvertions);
    }

    virtual String Details() const
    {
      return Messages->GetMessages('\n');
    }
  private:
    void ConvertItem(const String& path, ZXTune::Module::Holder::Ptr item)
    {
      try
      {
        const Log::ProgressCallback::Ptr curItemProgress = Log::CreateNestedPercentProgressCallback(TotalItems, SucceedConvertions + FailedConvertions, Callback);
        const ZXTune::Module::Information::Ptr info = item->GetModuleInformation();
        const Log::ProgressCallback::Ptr framesProgress = Log::CreatePercentProgressCallback(info->FramesCount(), *curItemProgress);
        const ZXTune::Sound::CreateBackendParameters::Ptr params = CreateBackendParameters(BackendParameters, item);
        ZXTune::Sound::Backend::Ptr backend;
        ThrowIfError(Creator->CreateBackend(params, backend));
        Convert(*backend, *framesProgress);
        curItemProgress->OnProgress(100);
        Succeed();
      }
      catch (const Error& err)
      {
        Failed(Strings::Format(Text::CONVERT_FAILED, path, err.GetText()));
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
    }

    void Failed(const String& status)
    {
      ++FailedConvertions;
      Messages->AddMessage(status);
    }

    void Succeed()
    {
      ++SucceedConvertions;
      //Do not add text about succeed convertion
    }

  private:
    const uint_t TotalItems;
    Log::ProgressCallback& Callback;
    const ZXTune::Sound::BackendCreator::Ptr Creator;
    const Parameters::Accessor::Ptr BackendParameters;
    const Log::MessagesCollector::Ptr Messages;
    std::size_t SucceedConvertions;
    std::size_t FailedConvertions;
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
    ConvertOperation(QObject& parent, Playlist::Model::IndexSetPtr items,
      const String& type, Parameters::Accessor::Ptr params)
      : Playlist::Item::TextResultOperation(parent)
      , SelectedItems(items)
      , Creator(FindBackendCreator(type))
      , Params(params)
    {
      Require(Creator);
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      const boost::shared_ptr<ConvertVisitor> tmp = boost::make_shared<ConvertVisitor>(totalItems, Creator, Params, boost::ref(cb));
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, *tmp);
      }
      else
      {
        stor.ForAllItems(*tmp);
      }
      OnResult(tmp);
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const ZXTune::Sound::BackendCreator::Ptr Creator;
    const Parameters::Accessor::Ptr Params;
  };
}

namespace Playlist
{
  namespace Item
  {
    TextResultOperation::Ptr CreateConvertOperation(QObject& parent, Playlist::Model::IndexSetPtr items,
      const String& type, Parameters::Accessor::Ptr params)
    {
      return boost::make_shared<ConvertOperation>(boost::ref(parent), items, type, params);
    }
  }
}
