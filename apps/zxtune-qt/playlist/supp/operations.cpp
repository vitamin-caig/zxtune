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
#include <template_parameters.h>
//library includes
#include <binary/data_adapter.h>
#include <core/convert_parameters.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <io/providers/file_provider.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>

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
  class VisitorAdapter : public Playlist::Item::Visitor
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
  class ItemsWithDuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    ItemsWithDuplicatesCollector()
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
    const std::size_t totalItems = model.CountItems();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<T> modelWrapper(model, *progress);
    modelWrapper.ForAllItems(visitor);
  }

  template<class T>
  void VisitAsSelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems, Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const std::size_t totalItems = model.CountItems() + selectedItems.size();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<T> modelWrapper(model, *progress);

    PropertiesCollector<T> selectedProps;
    modelWrapper.ForSpecifiedItems(selectedItems, selectedProps);
    PropertiesFilter<T> filter(visitor, selectedProps.GetResult());
    modelWrapper.ForAllItems(filter);
  }

  template<class T>
  void VisitOnlySelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems, Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const std::size_t totalItems = selectedItems.size();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<uint32_t> modelWrapper(model, *progress);
    modelWrapper.ForSpecifiedItems(selectedItems, visitor);
  }

  // Remove dups
  class SelectAllDupsOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectAllDupsOperation(QObject& parent)
      : Playlist::Item::SelectionOperation(parent)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      DuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitAllItems(propertyModel, cb, dups);
      }
      emit ResultAcquired(dups.GetResult());
    }
  };

  class SelectDupsOfSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectDupsOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ItemsWithDuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, dups);
      }
      //select all rips but delete only nonselected
      const boost::shared_ptr<Playlist::Model::IndexSet> toRemove = dups.GetResult();
      std::for_each(SelectedItems->begin(), SelectedItems->end(), boost::bind<Playlist::Model::IndexSet::size_type>(&Playlist::Model::IndexSet::erase, toRemove.get(), _1));
      emit ResultAcquired(toRemove);
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  class SelectDupsInSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectDupsInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      DuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitOnlySelectedItems(propertyModel, *SelectedItems, cb, dups);
      }
      emit ResultAcquired(dups.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  // Select  ripoffs
  class SelectAllRipOffsOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectAllRipOffsOperation(QObject& parent)
      : Playlist::Item::SelectionOperation(parent)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ItemsWithDuplicatesCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitAllItems(propertyModel, cb, rips);
      }
      emit ResultAcquired(rips.GetResult());
    }
  };

  class SelectRipOffsOfSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectRipOffsOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ItemsWithDuplicatesCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, rips);
      }
      emit ResultAcquired(rips.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  class SelectRipOffsInSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectRipOffsInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ItemsWithDuplicatesCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitOnlySelectedItems(propertyModel, *SelectedItems, cb, rips);
      }
      emit ResultAcquired(rips.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  //other
  class SelectTypesOfSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectTypesOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ItemsWithDuplicatesCollector<String> types;
      {
        const TypedPropertyModel<String> propertyModel(stor, &Playlist::Item::Data::GetType);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, types);
      }
      emit ResultAcquired(types.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  class ProgressModelVisitor : public Playlist::Item::Visitor
  {
  public:
    ProgressModelVisitor(Playlist::Item::Visitor& delegate, Log::ProgressCallback& cb)
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
    Playlist::Item::Visitor& Delegate;
    Log::ProgressCallback& Callback;
    uint_t Done;
  };

  void ExecuteOperation(const Playlist::Item::Storage& stor, Playlist::Model::IndexSetPtr selectedItems, Playlist::Item::Visitor& visitor, Log::ProgressCallback& cb)
  {
    const std::size_t totalItems = selectedItems ? selectedItems->size() : stor.CountItems();
    const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(static_cast<uint_t>(totalItems), cb);
    ProgressModelVisitor progressed(visitor, *progress);
    if (selectedItems)
    {
      stor.ForSpecifiedItems(*selectedItems, progressed);
    }
    else
    {
      stor.ForAllItems(progressed);
    }
  }

  class InvalidModulesCollection : public Playlist::Item::Visitor
  {
  public:
    InvalidModulesCollection()
      : Result(boost::make_shared<Playlist::Model::IndexSet>())
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      //check for the data first to define is data valid or not
      const String type = data->GetType();
      if (!data->IsValid())
      {
        Result->insert(index);
      }
    }

    Playlist::Model::IndexSetPtr GetResult() const
    {
      return Result;
    }
  private:
    const boost::shared_ptr<Playlist::Model::IndexSet> Result;
  };

  class SelectUnavailableOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectUnavailableOperation(QObject& parent)
      : Playlist::Item::SelectionOperation(parent)
    {
    }

    SelectUnavailableOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
      : Playlist::Item::SelectionOperation(parent)
      , SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      InvalidModulesCollection invalids;
      ExecuteOperation(stor, SelectedItems, invalids, cb);
      emit ResultAcquired(invalids.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
  };

  class CollectingVisitor : public Playlist::Item::Visitor
                          , public Playlist::TextNotification
  {
  public:
    typedef boost::shared_ptr<CollectingVisitor> Ptr;
  };

  // Statistic
  class CollectStatisticOperation : public Playlist::Item::TextResultOperation
                                  , private Playlist::Item::Visitor
  {
  public:
    CollectStatisticOperation(QObject& parent, Playlist::Item::StatisticTextNotification::Ptr result)
      : Playlist::Item::TextResultOperation(parent)
      , SelectedItems()
      , Result(result)
    {
    }

    CollectStatisticOperation(QObject& parent, Playlist::Model::IndexSetPtr items, Playlist::Item::StatisticTextNotification::Ptr result)
      : Playlist::Item::TextResultOperation(parent)
      , SelectedItems(items)
      , Result(result)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ExecuteOperation(stor, SelectedItems, *this, cb);
      emit ResultAcquired(Result);
    }
  private:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      //check for the data first to define is data valid or not
      const String type = data->GetType();
      if (!data->IsValid())
      {
        Result->AddInvalid();
      }
      else
      {
        assert(!type.empty());
        Result->AddValid(type, data->GetDuration(), data->GetSize());
      }
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const Playlist::Item::StatisticTextNotification::Ptr Result;
  };

  // Exporting
  //cache parameter value
  class SaveParameters : public ZXTune::IO::FileCreatingParameters
  {
  public:
    explicit SaveParameters(Parameters::Accessor::Ptr params)
      : OverwriteValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT)
      , CreateDirectoriesValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT)
    {
      params->FindValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, OverwriteValue);
      params->FindValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, CreateDirectoriesValue);
    }

    virtual bool Overwrite() const
    {
      return OverwriteValue != 0;
    }

    virtual bool CreateDirectories() const
    {
      return CreateDirectoriesValue != 0;
    }
  private:
    Parameters::IntType OverwriteValue;
    Parameters::IntType CreateDirectoriesValue;
  };

  class ExportOperation : public Playlist::Item::TextResultOperation
                        , private Playlist::Item::Visitor
  {
  public:
    ExportOperation(QObject& parent, const String& nameTemplate, Parameters::Accessor::Ptr params, Playlist::Item::ConversionResultNotification::Ptr result)
      : Playlist::Item::TextResultOperation(parent)
      , SelectedItems()
      , NameTemplate(StringTemplate::Create(nameTemplate))
      , Params(params)
      , Result(result)
    {
    }

    ExportOperation(QObject& parent, Playlist::Model::IndexSetPtr items, const String& nameTemplate, Parameters::Accessor::Ptr params, Playlist::Item::ConversionResultNotification::Ptr result)
      : Playlist::Item::TextResultOperation(parent)
      , SelectedItems(items)
      , NameTemplate(StringTemplate::Create(nameTemplate))
      , Params(params)
      , Result(result)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      ExecuteOperation(stor, SelectedItems, *this, cb);
      emit ResultAcquired(Result);
    }
  private:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      const String path = data->GetFullPath();
      if (ZXTune::Module::Holder::Ptr holder = data->GetModule())
      {
        ExportItem(path, *holder);
      }
      else
      {
        Result->AddFailedToOpen(path);
      }
    }

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
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }

    void Save(const Dump& data, const String& filename) const
    {
      const Binary::OutputStream::Ptr stream = ZXTune::IO::CreateLocalFile(filename, Params);
      const Binary::DataAdapter toSave(&data[0], data.size());
      stream->ApplyData(toSave);
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
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const StringTemplate::Ptr NameTemplate;
    const SaveParameters Params;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };
}

namespace Playlist
{
  namespace Item
  {
    SelectionOperation::SelectionOperation(QObject &parent)
      : QObject(&parent)
    {
    }

    TextResultOperation::TextResultOperation(QObject &parent)
      : QObject(&parent)
    {
    }
  }
}

namespace Playlist
{
  namespace Item
  {
    SelectionOperation::Ptr CreateSelectAllRipOffsOperation(QObject& parent)
    {
      return boost::make_shared<SelectAllRipOffsOperation>(boost::ref(parent));
    }

    SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectRipOffsOfSelectedOperation>(boost::ref(parent), items);
    }

    SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectRipOffsInSelectedOperation>(boost::ref(parent), items);
    }

    SelectionOperation::Ptr CreateSelectAllDuplicatesOperation(QObject& parent)
    {
      return boost::make_shared<SelectAllDupsOperation>(boost::ref(parent));
    }

    SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectDupsOfSelectedOperation>(boost::ref(parent), items);
    }

    SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectDupsInSelectedOperation>(boost::ref(parent), items);
    }

    SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectTypesOfSelectedOperation>(boost::ref(parent), items);
    }

    SelectionOperation::Ptr CreateSelectAllUnavailableOperation(QObject& parent)
    {
      return boost::make_shared<SelectUnavailableOperation>(boost::ref(parent));
    }

    SelectionOperation::Ptr CreateSelectUnavailableInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectUnavailableOperation>(boost::ref(parent), items);
    }

    TextResultOperation::Ptr CreateCollectStatisticOperation(QObject& parent, StatisticTextNotification::Ptr result)
    {
      return boost::make_shared<CollectStatisticOperation>(boost::ref(parent), result);
    }

    TextResultOperation::Ptr CreateCollectStatisticOperation(QObject& parent, Playlist::Model::IndexSetPtr items, StatisticTextNotification::Ptr result)
    {
      return boost::make_shared<CollectStatisticOperation>(boost::ref(parent), items, result);
    }

    TextResultOperation::Ptr CreateExportOperation(QObject& parent, const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ExportOperation>(boost::ref(parent), nameTemplate, params, result);
    }

    TextResultOperation::Ptr CreateExportOperation(QObject& parent, Playlist::Model::IndexSetPtr items, const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ExportOperation>(boost::ref(parent), items, nameTemplate, params, result);
    }
  }
}
