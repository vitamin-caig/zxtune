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
    typedef T (Playlist::Item::Data::*GetFunctionType)() const;

    PropertyModel(const Playlist::Item::Storage& model, const GetFunctionType getter)
      : Model(model)
      , Getter(getter)
    {
    }

    class Visitor
    {
    public:
      virtual ~Visitor() {}

      virtual void OnItem(Playlist::Model::IndexType index, const T& val) = 0;
    };

    void ForAllItems(Visitor& visitor) const;

    void ForSpecifiedItems(const Playlist::Model::IndexSet& items, Visitor& visitor) const;
  private:
    const Playlist::Item::Storage& Model;
    const GetFunctionType Getter;
  }; 

  template<class T>
  class VisitorAdapter : public Playlist::Model::Visitor
  {
  public:
    VisitorAdapter(const typename PropertyModel<T>::GetFunctionType getter, typename PropertyModel<T>::Visitor& delegate)
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
    const typename PropertyModel<T>::GetFunctionType Getter;
    typename PropertyModel<T>::Visitor& Delegate;
  };

  template<class T>
  void PropertyModel<T>::ForAllItems(typename PropertyModel<T>::Visitor& visitor) const
  {
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForAllItems(adapter);
  }

  template<class T>
  void PropertyModel<T>::ForSpecifiedItems(const Playlist::Model::IndexSet& items, typename PropertyModel<T>::Visitor& visitor) const
  {
    assert(!items.empty());
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForSpecifiedItems(items, adapter);
  }


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
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (!Visited.insert(val).second)
      {
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Visited;
    Playlist::Model::IndexSet Result;
  };

  template<class T>
  class RipOffsCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      const std::pair<typename PropToIndex::iterator, bool> result = Visited.insert(typename PropToIndex::value_type(val, index));
      if (!result.second)
      {
        Result.insert(result.first->second);
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    typedef typename std::map<T, Playlist::Model::IndexType> PropToIndex;
    PropToIndex Visited;
    Playlist::Model::IndexSet Result;
  };

  // Remove dups
  class RemoveAllDupsOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    virtual void Execute(Playlist::Item::Storage& stor, Log::ProgressCallback& /*cb*/)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForAllItems(dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      stor.RemoveItems(toRemove);
    }
  };

  class RemoveDupsOfSelectedOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit RemoveDupsOfSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      //select all rips but delete only nonselected
      RipOffsCollector<uint32_t> dups;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(dups, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      Playlist::Model::IndexSet toRemove = dups.GetResult();
      std::for_each(SelectedItems.begin(), SelectedItems.end(), boost::bind<Playlist::Model::IndexSet::size_type>(&Playlist::Model::IndexSet::erase, &toRemove, _1));
      stor.RemoveItems(toRemove);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
  };

  class RemoveDupsInSelectedOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit RemoveDupsInSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForSpecifiedItems(SelectedItems, dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      stor.RemoveItems(toRemove);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
  };

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

  // Select  ripoffs
  class SelectAllRipOffsOperation : public Playlist::Item::PromisedSelectionOperation
  {
  public:
    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForAllItems(rips);
      Result.Set(boost::make_shared<Playlist::Model::IndexSet>(rips.GetResult()));
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
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(rips, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      Result.Set(boost::make_shared<Playlist::Model::IndexSet>(rips.GetResult()));
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
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForSpecifiedItems(SelectedItems, rips);
      const Playlist::Model::IndexSet toSelect = rips.GetResult();
      Result.Set(boost::make_shared<Playlist::Model::IndexSet>(rips.GetResult()));
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
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, *tmp);
      }
      else
      {
        stor.ForAllItems(*tmp);
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
        Succeed(Strings::Format(Text::CONVERT_SUCCEED, path, filename));
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

    void Succeed(const String& status)
    {
      ++SucceedConvertions;
      Messages->AddMessage(status);
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

    StorageModifyOperation::Ptr CreateRemoveAllDuplicatesOperation()
    {
      return boost::make_shared<RemoveAllDupsOperation>();
    }

    StorageModifyOperation::Ptr CreateRemoveDuplicatesOfSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<RemoveDupsOfSelectedOperation>(boost::cref(items));
    }

    StorageModifyOperation::Ptr CreateRemoveDuplicatesInSelectedOperation(const Playlist::Model::IndexSet& items)
    {
      return boost::make_shared<RemoveDupsInSelectedOperation>(boost::cref(items));
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
  }
}
