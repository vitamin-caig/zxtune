/*
Abstract:
  Playlist operations

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
#include <error_tools.h>
#include <format.h>
#include <messages_collector.h>
#include <template.h>
#include <template_parameters.h>
//library includes
#include <async/signals_collector.h>
#include <core/convert_parameters.h>
#include <io/fs_tools.h>
#include <sound/backend.h>
#include <sound/backends_parameters.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
//qt includes
//TODO: remove
#include <QtCore/QCoreApplication>
//text includes
#include "text/text.h"

namespace
{
  template<class T>
  class PropertyModel
  {
  public:
    virtual ~PropertyModel() {}

    class Visitor
    {
    public:
      virtual ~Visitor() {}

      virtual void OnItem(Playlist::Model::IndexType index, const T& val) = 0;
    };

    virtual std::size_t CountItems() const = 0;

    virtual void ForAllItems(Visitor& visitor) const = 0;

    virtual void ForSpecifiedItems(const Playlist::Model::IndexSet& items, Visitor& visitor) const = 0;
  }; 

  template<class T>
  class VisitorAdapter : public Playlist::Model::Visitor
  {
  public:
    typedef T (Playlist::Item::Data::*GetFunctionType)() const;

    VisitorAdapter(const GetFunctionType getter, typename PropertyModel<T>::Visitor& delegate)
      : Getter(getter)
      , Delegate(delegate)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      if (!data->IsValid())
      {
        return;
      }
      const T val = ((*data).*Getter)();
      Delegate.OnItem(index, val);
    }

  private:
    const GetFunctionType Getter;
    typename PropertyModel<T>::Visitor& Delegate;
  };

  template<class T>
  class TypedPropertyModel : public PropertyModel<T>
  {
  public:
    TypedPropertyModel(const Playlist::Item::Storage& model, const typename VisitorAdapter<T>::GetFunctionType getter)
      : Model(model)
      , Getter(getter)
    {
    }

    virtual std::size_t CountItems() const
    {
      return Model.CountItems();
    }

    virtual void ForAllItems(typename PropertyModel<T>::Visitor& visitor) const
    {
      VisitorAdapter<T> adapter(Getter, visitor);
      Model.ForAllItems(adapter);
    }

    virtual void ForSpecifiedItems(const Playlist::Model::IndexSet& items, typename PropertyModel<T>::Visitor& visitor) const
    {
      assert(!items.empty());
      VisitorAdapter<T> adapter(Getter, visitor);
      Model.ForSpecifiedItems(items, adapter);
    }
  private:
    const Playlist::Item::Storage& Model;
    const typename VisitorAdapter<T>::GetFunctionType Getter;
  };

  template<class T>
  class PropertyModelWithProgress : public PropertyModel<T>
  {
  public:
    PropertyModelWithProgress(const PropertyModel<T>& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
      , Done(0)
    {
    }

    virtual std::size_t CountItems() const
    {
      return Delegate.CountItems();
    }

    virtual void ForAllItems(typename PropertyModel<T>::Visitor& visitor) const
    {
      ProgressVisitorWrapper wrapper(visitor, Callback, Done);
      Delegate.ForAllItems(wrapper);
    }

    virtual void ForSpecifiedItems(const Playlist::Model::IndexSet& items, typename PropertyModel<T>::Visitor& visitor) const
    {
      ProgressVisitorWrapper wrapper(visitor, Callback, Done);
      Delegate.ForSpecifiedItems(items, wrapper);
    }
  private:
    class ProgressVisitorWrapper : public PropertyModel<T>::Visitor
    {
    public:
      ProgressVisitorWrapper(typename PropertyModel<T>::Visitor& delegate, Log::ProgressCallback& cb, uint_t& done)
        : Delegate(delegate)
        , Callback(cb)
        , Done(done)
      {
      }

      virtual void OnItem(Playlist::Model::IndexType index, const T& val)
      {
        Delegate.OnItem(index, val);
        Callback.OnProgress(++Done);
      }
    private:
      typename PropertyModel<T>::Visitor& Delegate;
      Log::ProgressCallback& Callback;
      uint_t& Done;
    };
  private:
    const PropertyModel<T>& Delegate;
    Log::ProgressCallback& Callback;
    mutable uint_t Done;
  };

  template<class T>
  class PropertiesFilter : public PropertyModel<T>::Visitor
  {
  public:
    PropertiesFilter(typename PropertyModel<T>::Visitor& delegate, const std::set<T>& filter)
      : Delegate(delegate)
      , Filter(filter)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (Filter.count(val))
      {
        Delegate.OnItem(index, val);
      }
    }
  private:
    typename PropertyModel<T>::Visitor& Delegate;
    const std::set<T>& Filter;
  };

  template<class T>
  class PropertiesCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, const T& val)
    {
      Result.insert(val);
    }

    const std::set<T>& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Result;
  };

  template<class T>
  class DuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    DuplicatesCollector()
      : Result(boost::make_shared<Playlist::Model::IndexSet>())
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (!Visited.insert(val).second)
      {
        Result->insert(index);
      }
    }

    boost::shared_ptr<Playlist::Model::IndexSet> GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Visited;
    const boost::shared_ptr<Playlist::Model::IndexSet> Result;
  };

  template<class T>
  class RipOffsCollector : public PropertyModel<T>::Visitor
  {
  public:
    RipOffsCollector()
      : Result(boost::make_shared<Playlist::Model::IndexSet>())
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      const std::pair<typename PropToIndex::iterator, bool> result = Visited.insert(typename PropToIndex::value_type(val, index));
      if (!result.second)
      {
        Result->insert(result.first->second);
        Result->insert(index);
      }
    }

    boost::shared_ptr<Playlist::Model::IndexSet> GetResult() const
    {
      return Result;
    }
  private:
    typedef typename std::map<T, Playlist::Model::IndexType> PropToIndex;
    PropToIndex Visited;
    const boost::shared_ptr<Playlist::Model::IndexSet> Result;
  };

  template<class T>
  void VisitAllItems(const PropertyModel<T>& model, Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const uint_t totalItems = model.CountItems();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems, cb);
    const PropertyModelWithProgress<T> modelWrapper(model, *progress);
    modelWrapper.ForAllItems(visitor);
  }

  template<class T>
  void VisitAsSelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems, Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const uint_t totalItems = model.CountItems() + selectedItems.size();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems, cb);
    const PropertyModelWithProgress<T> modelWrapper(model, *progress);

    PropertiesCollector<T> selectedProps;
    modelWrapper.ForSpecifiedItems(selectedItems, selectedProps);
    PropertiesFilter<T> filter(visitor, selectedProps.GetResult());
    modelWrapper.ForAllItems(filter);
  }

  template<class T>
  void VisitOnlySelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems, Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const uint_t totalItems = selectedItems.size();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems, cb);
    const PropertyModelWithProgress<uint32_t> modelWrapper(model, *progress);
    modelWrapper.ForSpecifiedItems(selectedItems, visitor);
  }

  template<class ResultType>
  class Promise
  {
  public:
    Promise()
      : Signals(Async::Signals::Dispatcher::Create())
    {
    }

    void Set(ResultType res)
    {
      Result = res;
      Signals->Notify(1);
    }

    ResultType Get() const
    {
      //TODO: remove dependency from QT
      const Async::Signals::Collector::Ptr collector = Signals->CreateCollector(1);
      while (!collector->WaitForSignals(100))
      {
        QCoreApplication::processEvents();
      }
      return Result;
    }
  private:
    const Async::Signals::Dispatcher::Ptr Signals;
    ResultType Result;
  };

  // Remove dups
  class SelectAllDupsOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      DuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitAllItems(propertyModel, cb, dups);
      }
      Result.Set(dups.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class SelectDupsOfSelectedOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    explicit SelectDupsOfSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      RipOffsCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitAsSelectedItems(propertyModel, SelectedItems, cb, dups);
      }
      //select all rips but delete only nonselected
      const boost::shared_ptr<Playlist::Model::IndexSet> toRemove = dups.GetResult();
      std::for_each(SelectedItems.begin(), SelectedItems.end(), boost::bind<Playlist::Model::IndexSet::size_type>(&Playlist::Model::IndexSet::erase, toRemove.get(), _1));
      Result.Set(toRemove);
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class SelectDupsInSelectedOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    explicit SelectDupsInSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      DuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitOnlySelectedItems(propertyModel, SelectedItems, cb, dups);
      }
      Result.Set(dups.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  // Select  ripoffs
  class SelectAllRipOffsOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      RipOffsCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitAllItems(propertyModel, cb, rips);
      }
      Result.Set(rips.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class SelectRipOffsOfSelectedOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    explicit SelectRipOffsOfSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      RipOffsCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitAsSelectedItems(propertyModel, SelectedItems, cb, rips);
      }
      Result.Set(rips.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class SelectRipOffsInSelectedOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    explicit SelectRipOffsInSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      RipOffsCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitOnlySelectedItems(propertyModel, SelectedItems, cb, rips);
      }
      Result.Set(rips.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class SelectTypesOfSelectedOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    explicit SelectTypesOfSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      RipOffsCollector<String> types;
      {
        const TypedPropertyModel<String> propertyModel(stor, &Playlist::Item::Data::GetType);
        VisitAsSelectedItems(propertyModel, SelectedItems, cb, types);
      }
      Result.Set(types.GetResult());
    }

    virtual Playlist::Item::SelectionPtr GetResult() const
    {
      return Result.Get();
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Promise<Playlist::Item::SelectionPtr> Result;
  };

  class CollectingVisitor : public Playlist::Model::Visitor
                          , public Playlist::Item::TextOperationResult
  {
  public:
    typedef boost::shared_ptr<CollectingVisitor> Ptr;
  };

  class ProgressModelVisitor : public Playlist::Model::Visitor
  {
  public:
    ProgressModelVisitor(Playlist::Model::Visitor& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
      , Done(0)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      Delegate.OnItem(index, data);
      Callback.OnProgress(++Done);
    }
  private:
    Playlist::Model::Visitor& Delegate;
    Log::ProgressCallback& Callback;
    uint_t Done;
  };

  class PromisedTextResultOperationBase : public Playlist::Item::PromisedTextResultOperation
  {
  public:
    PromisedTextResultOperationBase()
      : SelectedItems()
    {
    }

    explicit PromisedTextResultOperationBase(const Playlist::Model::IndexSet& items)
      : SelectedItems(&items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      CollectingVisitor::Ptr tmp = CreateCollector();
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems, cb);
      ProgressModelVisitor progressed(*tmp, *progress);
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, progressed);
      }
      else
      {
        stor.ForAllItems(progressed);
      }
      Result.Set(tmp);
    }

    virtual Playlist::Item::TextOperationResult::Ptr GetResult() const
    {
      return Result.Get();
    }
  protected:
    virtual CollectingVisitor::Ptr CreateCollector() const = 0;
  private:
    const Playlist::Model::IndexSet* const SelectedItems;
    Promise<Playlist::Item::TextOperationResult::Ptr> Result;
  };

  // Collect paths
  class PathesCollector : public CollectingVisitor
  {
  public:
    PathesCollector()
      : Messages(Log::MessagesCollector::Create())
    {
    }

    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      if (!data->IsValid())
      {
        return;
      }
      const String path = data->GetFullPath();
      Messages->AddMessage(path);
    }

    virtual String GetBasicResult() const
    {
      return Strings::Format(Text::PATHS_COLLECTING_STATUS, Messages->CountMessages());
    }

    virtual String GetDetailedResult() const
    {
      return Messages->GetMessages('\n');
    }
  private:
    const Log::MessagesCollector::Ptr Messages;
  };

  class CollectPathsOperation : public PromisedTextResultOperationBase
  {
  public:
    explicit CollectPathsOperation(const Playlist::Model::IndexSet& items)
      : PromisedTextResultOperationBase(items)
    {
    }
  protected:
    virtual CollectingVisitor::Ptr CreateCollector() const
    {
      return boost::make_shared<PathesCollector>();
    }
  };

  // Statistic
  class StatisticCollector : public CollectingVisitor
  {
  public:
    StatisticCollector()
      : Processed()
      , Invalids()
      , Duration()
      , Size()
    {
    }

    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      //check for the data first to define is data valid or not
      const String type = data->GetType();
      if (!data->IsValid())
      {
        ++Invalids;
        return;
      }
      assert(!type.empty());
      ++Processed;
      Duration += data->GetDuration();
      Size += data->GetSize();
      ++Types[type];
    }

    virtual String GetBasicResult() const
    {
      const String duration = Strings::FormatTime(Duration.Get(), 1000);
      return Strings::Format(Text::BASIC_STATISTIC_TEMPLATE,
        Processed, Invalids,
        duration,
        Size,
        Types.size()
        );
    }

    virtual String GetDetailedResult() const
    {
      return std::accumulate(Types.begin(), Types.end(), String(), &TypeStatisticToString);
    }
  private:
    static String TypeStatisticToString(const String& prev, const std::pair<String, std::size_t>& item)
    {
      const String& next = Strings::Format(Text::TYPE_STATISTIC_TEMPLATE, item.first, item.second);
      return prev.empty()
        ? next
        : prev + '\n' + next;
    }
  private:
    std::size_t Processed;
    std::size_t Invalids;
    Time::Stamp<uint64_t, 1000> Duration;
    uint64_t Size;
    std::map<String, std::size_t> Types;
  };

  class CollectStatisticOperation : public PromisedTextResultOperationBase
  {
  public:
    CollectStatisticOperation()
      : PromisedTextResultOperationBase()
    {
    }

    explicit CollectStatisticOperation(const Playlist::Model::IndexSet& items)
      : PromisedTextResultOperationBase(items)
    {
    }
  protected:
    virtual CollectingVisitor::Ptr CreateCollector() const
    {
      return boost::make_shared<StatisticCollector>();
    }
  };

  // Exporting
  class ExportVisitor : public CollectingVisitor
  {
  public:
    ExportVisitor(const String& nameTemplate, bool overwrite)
      : NameTemplate(StringTemplate::Create(nameTemplate))
      , Overwrite(overwrite)
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
        ExportItem(path, *holder);
      }
      else
      {
        Failed(Strings::Format(Text::CONVERT_FAILED_OPEN, path));
      }
    }

    virtual String GetBasicResult() const
    {
      return Strings::Format(Text::CONVERT_STATUS, SucceedConvertions, FailedConvertions);
    }

    virtual String GetDetailedResult() const
    {
      return Messages->GetMessages('\n');
    }
  private:
    class ModuleFieldsSource : public Parameters::FieldsSourceAdapter<SkipFieldsSource>
    {
    public:
      typedef Parameters::FieldsSourceAdapter<SkipFieldsSource> Parent;
      explicit ModuleFieldsSource(const Parameters::Accessor& params)
        : Parent(params)
      {
      }

      String GetFieldValue(const String& fieldName) const
      {
        return ZXTune::IO::MakePathFromString(Parent::GetFieldValue(fieldName), '_');
      }
    };

    void ExportItem(const String& path, const ZXTune::Module::Holder& item)
    {
      static const ZXTune::Module::Conversion::RawConvertParam RAW_CONVERSION;
      try
      {
        const Parameters::Accessor::Ptr props = item.GetModuleProperties();
        Dump result;
        ThrowIfError(item.Convert(RAW_CONVERSION, props, result));
        const String filename = NameTemplate->Instantiate(ModuleFieldsSource(*props));
        Save(result, filename);
        Succeed();
      }
      catch (const Error& err)
      {
        Failed(Strings::Format(Text::CONVERT_FAILED, path, err.GetText()));
      }
    }

    void Save(const Dump& data, const String& filename) const
    {
      const std::auto_ptr<std::ofstream> stream = ZXTune::IO::CreateFile(filename, Overwrite);
      stream->write(safe_ptr_cast<const char*>(&data[0]), static_cast<std::streamsize>(data.size() * sizeof(data.front())));
      //TODO: handle possible errors
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
    const StringTemplate::Ptr NameTemplate;
    const bool Overwrite;
    const Log::MessagesCollector::Ptr Messages;
    std::size_t SucceedConvertions;
    std::size_t FailedConvertions;
  };

  class ExportOperation : public PromisedTextResultOperationBase
  {
  public:
    explicit ExportOperation(const String& nameTemplate)
      : PromisedTextResultOperationBase()
      , NameTemplate(nameTemplate)
    {
    }

    ExportOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate)
      : PromisedTextResultOperationBase(items)
      , NameTemplate(nameTemplate)
    {
    }
  protected:
    virtual CollectingVisitor::Ptr CreateCollector() const
    {
      return boost::make_shared<ExportVisitor>(NameTemplate, true);
    }
  private:
    const String NameTemplate;
  };

  // Converting
  class BackendParams : public Parameters::Accessor
  {
  public:
    BackendParams(const String& filename, bool overwrite)
      : Filename(filename)
      , Overwrite(overwrite)
    {
    }

    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == Parameters::ZXTune::Sound::Backends::Wav::OVERWRITE)
      {
        val = Overwrite;
        return true;
      }
      return false;
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == Parameters::ZXTune::Sound::Backends::Wav::FILENAME)
      {
        val = Filename;
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetIntValue(Parameters::ZXTune::Sound::Backends::Wav::FILENAME, Overwrite);
      visitor.SetStringValue(Parameters::ZXTune::Sound::Backends::Wav::FILENAME, Filename);
    }
  private:
    const String Filename;
    const bool Overwrite;
  };

  ZXTune::Sound::Backend::Ptr CreateBackend(ZXTune::Sound::CreateBackendParameters::Ptr params, const String& id)
  {
    using namespace ZXTune::Sound;
    for (BackendCreator::Iterator::Ptr backends = EnumerateBackends(); backends->IsValid(); backends->Next())
    {
      const BackendCreator::Ptr creator = backends->Get();
      if (creator->Id() != id)
      {
        continue;
      }
      Backend::Ptr result;
      ThrowIfError(creator->CreateBackend(params, result));
      return result;
    }
    //TODO:
    return Backend::Ptr();
  }

  class ConvertVisitor : public CollectingVisitor
  {
  public:
    ConvertVisitor(const String& nameTemplate, bool overwrite)
      : BackendParameters(boost::make_shared<BackendParams>(nameTemplate, overwrite))
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

    virtual String GetBasicResult() const
    {
      return Strings::Format(Text::CONVERT_STATUS, SucceedConvertions, FailedConvertions);
    }

    virtual String GetDetailedResult() const
    {
      return Messages->GetMessages('\n');
    }
  private:
    void ConvertItem(const String& path, ZXTune::Module::Holder::Ptr item)
    {
      static const Char WAVE_BACKEND_ID[] = {'w', 'a', 'v', '\0'};
      try
      {
        const ZXTune::Sound::CreateBackendParameters::Ptr params = CreateBackendParameters(BackendParameters, item);
        const ZXTune::Sound::Backend::Ptr backend = CreateBackend(params, WAVE_BACKEND_ID);
        Convert(*backend);
        Succeed();
      }
      catch (const Error& err)
      {
        Failed(Strings::Format(Text::CONVERT_FAILED, path, err.GetText()));
      }
    }

    void Convert(ZXTune::Sound::Backend& backend) const
    {
      using namespace ZXTune::Sound;
      const uint_t flags = Backend::MODULE_STOP | Backend::MODULE_FINISH;
      const Async::Signals::Collector::Ptr collector = backend.CreateSignalsCollector(flags);
      ThrowIfError(backend.Play());
      while (!collector->WaitForSignals(100)) {}
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
    const Parameters::Accessor::Ptr BackendParameters;
    const Log::MessagesCollector::Ptr Messages;
    std::size_t SucceedConvertions;
    std::size_t FailedConvertions;
  };

  class ConvertOperation : public PromisedTextResultOperationBase
  {
  public:
    ConvertOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate)
      : PromisedTextResultOperationBase(items)
      , NameTemplate(nameTemplate)
    {
    }
  protected:
    virtual CollectingVisitor::Ptr CreateCollector() const
    {
      return boost::make_shared<ConvertVisitor>(NameTemplate, true);
    }
  private:
    const String NameTemplate;
  };
}

namespace Playlist
{
  namespace Item
  {
    PromisedSelectionOperation::Ptr CreateSelectAllRipOffsOperation()
    {
      return boost::make_shared<SelectAllRipOffsOperation>();
    }

    PromisedSelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<SelectRipOffsOfSelectedOperation>(boost::cref(items));
    }

    PromisedSelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<SelectRipOffsInSelectedOperation>(boost::cref(items));
    }

    PromisedSelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<SelectTypesOfSelectedOperation>(boost::cref(items));
    }

    PromisedSelectionOperation::Ptr CreateSelectAllDuplicatesOperation()
    {
      return boost::make_shared<SelectAllDupsOperation>();
    }

    PromisedSelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<SelectDupsOfSelectedOperation>(boost::cref(items));
    }

    PromisedSelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<SelectDupsInSelectedOperation>(boost::cref(items));
    }

    PromisedTextResultOperation::Ptr CreateCollectPathsOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<CollectPathsOperation>(boost::cref(items));
    }

    PromisedTextResultOperation::Ptr CreateCollectStatisticOperation()
    {
      return boost::make_shared<CollectStatisticOperation>();
    }

    PromisedTextResultOperation::Ptr CreateCollectStatisticOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<CollectStatisticOperation>(boost::cref(items));
    }

    PromisedTextResultOperation::Ptr CreateExportOperation(const String& nameTemplate)
    {
      return boost::make_shared<ExportOperation>(nameTemplate);
    }

    PromisedTextResultOperation::Ptr CreateExportOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate)
    {
      return boost::make_shared<ExportOperation>(boost::cref(items), nameTemplate);
    }

    PromisedTextResultOperation::Ptr CreateConvertOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate)
    {
      return boost::make_shared<ConvertOperation>(boost::cref(items), nameTemplate);
    }
  }
}
