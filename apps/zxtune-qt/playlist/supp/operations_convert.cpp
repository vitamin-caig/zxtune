/**
 *
 * @file
 *
 * @brief Convert operation implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "operations_convert.h"
#include "operations_helpers.h"
#include "storage.h"
#include <apps/zxtune-qt/supp/playback_supp.h>
#include <apps/zxtune-qt/ui/utils.h>
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <async/src/event.h>
#include <io/api.h>
#include <io/template.h>
#include <parameters/merged_accessor.h>
#include <parameters/template.h>
#include <sound/backend.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>
#include <tools/progress_callback_helpers.h>
// std includes
#include <numeric>

namespace
{
  // TODO: rework
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
    {}

    void OnStart() override
    {
      Event.Reset();
    }

    void OnFrame(const Module::State& state) override
    {
      try
      {
        Callback.OnProgress(state.At().Get());
      }
      catch (const std::exception&)
      {
        Event.Set(CANCELED);
      }
    }

    void OnStop() override
    {
      Event.Set(STOPPED);
    }

    void OnPause() override {}

    void OnResume() override {}

    void OnFinish() override {}

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

  // TODO: simplify
  class ConvertVisitor : public Playlist::Item::Visitor
  {
  public:
    ConvertVisitor(uint_t totalItems, String type, Sound::Service::Ptr service, Log::ProgressCallback& cb,
                   Playlist::Item::ConversionResultNotification::Ptr result)
      : TotalItems(totalItems)
      , Callback(cb)
      , Type(std::move(type))
      , Service(std::move(service))
      , Result(std::move(result))
    {}

    void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data) override
    {
      const String path = data->GetFullPath();
      if (auto holder = data->GetModule())
      {
        ConvertItem(path, std::move(holder));
      }
      else
      {
        Result->AddFailedToOpen(path);
      }
      ++DoneItems;
    }

  private:
    void ConvertItem(StringView path, Module::Holder::Ptr item)
    {
      try
      {
        Log::NestedProgressCallback curItemProgress(TotalItems, DoneItems, Callback);
        const auto info = item->GetModuleInformation();
        Log::PercentProgressCallback framesProgress(info->Duration().Get(), curItemProgress);
        ConvertCallback cb(framesProgress);
        const auto backend =
            Service->CreateBackend(Sound::BackendId::FromString(Type), std::move(item),
                                   Sound::BackendCallback::Ptr(&cb, NullDeleter<Sound::BackendCallback>()));
        const auto control = backend->GetPlaybackControl();
        control->Play();
        cb.WaitForFinish();
        control->Stop();
        curItemProgress.OnProgress(100);
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }

  private:
    const uint_t TotalItems;
    uint_t DoneItems = 0;
    Log::ProgressCallback& Callback;
    const String Type;
    const Sound::Service::Ptr Service;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  class SoundFormatConvertOperation : public Playlist::Item::TextResultOperation
  {
  public:
    SoundFormatConvertOperation(Playlist::Model::IndexSet::Ptr items, String type, Sound::Service::Ptr service,
                                Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems(std::move(items))
      , Type(std::move(type))
      , Service(std::move(service))
      , Result(std::move(result))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
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
    const Playlist::Model::IndexSet::Ptr SelectedItems;
    const String Type;
    const Sound::Service::Ptr Service;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  // Exporting
  class ExportOperation
    : public Playlist::Item::TextResultOperation
    , private Playlist::Item::Visitor
  {
  public:
    ExportOperation(StringView nameTemplate, Parameters::Accessor::Ptr params,
                    Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems()
      , NameTemplate(IO::CreateFilenameTemplate(nameTemplate))
      , Params(std::move(params))
      , Result(std::move(result))
    {}

    ExportOperation(Playlist::Model::IndexSet::Ptr items, StringView nameTemplate, Parameters::Accessor::Ptr params,
                    Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems(std::move(items))
      , NameTemplate(IO::CreateFilenameTemplate(nameTemplate))
      , Params(std::move(params))
      , Result(std::move(result))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      ExecuteOperation(stor, SelectedItems, *this, cb);
      emit ResultAcquired(Result);
    }

  private:
    void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data) override
    {
      const String path = data->GetFullPath();
      if (const Module::Holder::Ptr holder = data->GetModule())
      {
        ExportItem(path, *holder, *data->GetModuleData());
      }
      else
      {
        Result->AddFailedToOpen(path);
      }
    }

    void ExportItem(StringView path, const Module::Holder& item, Binary::View content)
    {
      try
      {
        const auto props = item.GetModuleProperties();
        const auto& filename =
            NameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
        Save(content, filename);
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }

    void Save(Binary::View data, StringView filename) const
    {
      const auto stream = IO::CreateStream(filename, *Params, Log::ProgressCallback::Stub());
      stream->ApplyData(data);
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
    const Strings::Template::Ptr NameTemplate;
    const Parameters::Accessor::Ptr Params;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };

  Parameters::Accessor::Ptr CreateSoundParameters(const Playlist::Item::Conversion::Options& opts)
  {
    auto overriden = Parameters::Container::Create();
    overriden->SetValue(Parameters::ZXTune::Sound::Backends::File::FILENAME, opts.FilenameTemplate);
    overriden->SetValue(Parameters::ZXTune::Sound::LOOPED, 0);
    return Parameters::CreateMergedAccessor(std::move(overriden), opts.Params);
  }
}  // namespace

namespace Playlist::Item
{
  TextResultOperation::Ptr CreateSoundFormatConvertOperation(Playlist::Model::IndexSet::Ptr items, StringView type,
                                                             Sound::Service::Ptr service,
                                                             ConversionResultNotification::Ptr result)
  {
    return MakePtr<SoundFormatConvertOperation>(std::move(items), type.to_string(), std::move(service),
                                                std::move(result));
  }

  TextResultOperation::Ptr CreateExportOperation(StringView nameTemplate, Parameters::Accessor::Ptr params,
                                                 ConversionResultNotification::Ptr result)
  {
    return MakePtr<ExportOperation>(nameTemplate, std::move(params), std::move(result));
  }

  TextResultOperation::Ptr CreateExportOperation(Playlist::Model::IndexSet::Ptr items, StringView nameTemplate,
                                                 Parameters::Accessor::Ptr params,
                                                 ConversionResultNotification::Ptr result)
  {
    return MakePtr<ExportOperation>(std::move(items), nameTemplate, std::move(params), std::move(result));
  }

  TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSet::Ptr items, const Conversion::Options& opts,
                                                  ConversionResultNotification::Ptr result)
  {
    if (opts.Type.empty())
    {
      return CreateExportOperation(std::move(items), opts.FilenameTemplate, opts.Params, std::move(result));
    }
    else
    {
      auto soundParams = CreateSoundParameters(opts);
      auto service = Sound::CreateFileService(std::move(soundParams));
      return CreateSoundFormatConvertOperation(std::move(items), opts.Type, std::move(service), std::move(result));
    }
  }

  TextResultOperation::Ptr CreateConvertOperation(const Conversion::Options& opts,
                                                  ConversionResultNotification::Ptr result)
  {
    return CreateConvertOperation({}, opts, std::move(result));
  }
}  // namespace Playlist::Item
