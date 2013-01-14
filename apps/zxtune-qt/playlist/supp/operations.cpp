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
#include <template_parameters.h>
//library includes
#include <binary/data_adapter.h>
#include <core/convert_parameters.h>
#include <io/api.h>
#include <io/template.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
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
      if (data->GetState())
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
    explicit SelectDupsOfSelectedOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
    explicit SelectDupsInSelectedOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
    explicit SelectRipOffsOfSelectedOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
    explicit SelectRipOffsInSelectedOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
    explicit SelectTypesOfSelectedOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
      if (data->GetState())
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
    SelectUnavailableOperation()
    {
    }

    explicit SelectUnavailableOperation(Playlist::Model::IndexSetPtr items)
      : SelectedItems(items)
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
    explicit CollectStatisticOperation(Playlist::Item::StatisticTextNotification::Ptr result)
      : SelectedItems()
      , Result(result)
    {
    }

    CollectStatisticOperation(Playlist::Model::IndexSetPtr items, Playlist::Item::StatisticTextNotification::Ptr result)
      : SelectedItems(items)
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
      if (data->GetState())
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
  class ExportOperation : public Playlist::Item::TextResultOperation
                        , private Playlist::Item::Visitor
  {
  public:
    ExportOperation(const String& nameTemplate, Parameters::Accessor::Ptr params, Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems()
      , NameTemplate(IO::CreateFilenameTemplate(nameTemplate))
      , Params(params)
      , Result(result)
    {
    }

    ExportOperation(Playlist::Model::IndexSetPtr items, const String& nameTemplate, Parameters::Accessor::Ptr params, Playlist::Item::ConversionResultNotification::Ptr result)
      : SelectedItems(items)
      , NameTemplate(IO::CreateFilenameTemplate(nameTemplate))
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
        const Binary::Data::Ptr result = item.Convert(RAW_CONVERSION, props);
        const String filename = NameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
        Save(*result, filename);
        Result->AddSucceed();
      }
      catch (const Error& err)
      {
        Result->AddFailedToConvert(path, err);
      }
    }

    void Save(const Binary::Data& data, const String& filename) const
    {
      const Binary::OutputStream::Ptr stream = IO::CreateStream(filename, *Params, Log::ProgressCallback::Stub());
      stream->ApplyData(data);
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const Strings::Template::Ptr NameTemplate;
    const Parameters::Accessor::Ptr Params;
    const Playlist::Item::ConversionResultNotification::Ptr Result;
  };
}

namespace Playlist
{
  namespace Item
  {
    SelectionOperation::Ptr CreateSelectAllRipOffsOperation()
    {
      return boost::make_shared<SelectAllRipOffsOperation>();
    }

    SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectRipOffsOfSelectedOperation>(items);
    }

    SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectRipOffsInSelectedOperation>(items);
    }

    SelectionOperation::Ptr CreateSelectAllDuplicatesOperation()
    {
      return boost::make_shared<SelectAllDupsOperation>();
    }

    SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectDupsOfSelectedOperation>(items);
    }

    SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectDupsInSelectedOperation>(items);
    }

    SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectTypesOfSelectedOperation>(items);
    }

    SelectionOperation::Ptr CreateSelectAllUnavailableOperation()
    {
      return boost::make_shared<SelectUnavailableOperation>();
    }

    SelectionOperation::Ptr CreateSelectUnavailableInSelectedOperation(Playlist::Model::IndexSetPtr items)
    {
      return boost::make_shared<SelectUnavailableOperation>(items);
    }

    TextResultOperation::Ptr CreateCollectStatisticOperation(StatisticTextNotification::Ptr result)
    {
      return boost::make_shared<CollectStatisticOperation>(result);
    }

    TextResultOperation::Ptr CreateCollectStatisticOperation(Playlist::Model::IndexSetPtr items, StatisticTextNotification::Ptr result)
    {
      return boost::make_shared<CollectStatisticOperation>(items, result);
    }

    TextResultOperation::Ptr CreateExportOperation(const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ExportOperation>(nameTemplate, params, result);
    }

    TextResultOperation::Ptr CreateExportOperation(Playlist::Model::IndexSetPtr items, const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result)
    {
      return boost::make_shared<ExportOperation>(items, nameTemplate, params, result);
    }
  }
}
