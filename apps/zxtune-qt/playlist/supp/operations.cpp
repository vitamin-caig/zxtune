/**
 *
 * @file
 *
 * @brief Playlist common operations implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/playlist/supp/operations.h"
#include "apps/zxtune-qt/playlist/supp/operations_helpers.h"
#include "apps/zxtune-qt/playlist/supp/storage.h"
// common includes
#include <make_ptr.h>
// library includes
#include <tools/progress_callback_helpers.h>
// std includes
#include <numeric>

namespace
{
  template<class T>
  class PropertyModel
  {
  public:
    virtual ~PropertyModel() = default;

    class Visitor
    {
    public:
      virtual ~Visitor() = default;

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
    using GetFunctionType = T (Playlist::Item::Data::*)() const;

    VisitorAdapter(const GetFunctionType getter, typename PropertyModel<T>::Visitor& delegate)
      : Getter(getter)
      , Delegate(delegate)
    {}

    void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data) override
    {
      data->GetModule();
      if (data->GetState().GetIfError())
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
    {}

    std::size_t CountItems() const override
    {
      return Model.CountItems();
    }

    void ForAllItems(typename PropertyModel<T>::Visitor& visitor) const override
    {
      VisitorAdapter<T> adapter(Getter, visitor);
      Model.ForAllItems(adapter);
    }

    void ForSpecifiedItems(const Playlist::Model::IndexSet& items,
                           typename PropertyModel<T>::Visitor& visitor) const override
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
    {}

    std::size_t CountItems() const override
    {
      return Delegate.CountItems();
    }

    void ForAllItems(typename PropertyModel<T>::Visitor& visitor) const override
    {
      ProgressVisitorWrapper wrapper(visitor, Callback, Done);
      Delegate.ForAllItems(wrapper);
    }

    void ForSpecifiedItems(const Playlist::Model::IndexSet& items,
                           typename PropertyModel<T>::Visitor& visitor) const override
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
      {}

      void OnItem(Playlist::Model::IndexType index, const T& val) override
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
    mutable uint_t Done = 0;
  };

  template<class T>
  class PropertiesFilter : public PropertyModel<T>::Visitor
  {
  public:
    PropertiesFilter(typename PropertyModel<T>::Visitor& delegate, const std::set<T>& filter)
      : Delegate(delegate)
      , Filter(filter)
    {}

    void OnItem(Playlist::Model::IndexType index, const T& val) override
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
    void OnItem(Playlist::Model::IndexType /*index*/, const T& val) override
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
  class IndicesCollector : public PropertyModel<T>::Visitor
  {
  public:
    IndicesCollector()
      : Result(MakeRWPtr<Playlist::Model::IndexSet>())
    {}

    void OnItem(Playlist::Model::IndexType index, const T& /*val*/) override
    {
      Result->insert(index);
    }

    Playlist::Model::IndexSet::Ptr GetResult() const
    {
      return Result;
    }

  private:
    const Playlist::Model::IndexSet::RWPtr Result;
  };

  template<class T>
  class DuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    DuplicatesCollector()
      : Result(MakeRWPtr<Playlist::Model::IndexSet>())
    {}

    void OnItem(Playlist::Model::IndexType index, const T& val) override
    {
      if (!Visited.insert(val).second)
      {
        Result->insert(index);
      }
    }

    Playlist::Model::IndexSet::Ptr GetResult() const
    {
      return Result;
    }

  private:
    std::set<T> Visited;
    const Playlist::Model::IndexSet::RWPtr Result;
  };

  template<class T>
  class ItemsWithDuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    ItemsWithDuplicatesCollector()
      : Result(MakeRWPtr<Playlist::Model::IndexSet>())
    {}

    void OnItem(Playlist::Model::IndexType index, const T& val) override
    {
      const std::pair<typename PropToIndex::iterator, bool> result =
          Visited.insert(typename PropToIndex::value_type(val, index));
      if (!result.second)
      {
        Result->insert(result.first->second);
        Result->insert(index);
      }
    }

    Playlist::Model::IndexSet::RWPtr GetResult() const
    {
      return Result;
    }

  private:
    using PropToIndex = typename std::map<T, Playlist::Model::IndexType>;
    PropToIndex Visited;
    const Playlist::Model::IndexSet::RWPtr Result;
  };

  template<class T>
  void VisitAllItems(const PropertyModel<T>& model, Log::ProgressCallback& cb,
                     typename PropertyModel<T>::Visitor& visitor)
  {
    const std::size_t totalItems = model.CountItems();
    Log::PercentProgressCallback progress(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<T> modelWrapper(model, progress);
    modelWrapper.ForAllItems(visitor);
  }

  template<class T>
  void VisitAsSelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems,
                            Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const std::size_t totalItems = model.CountItems() + selectedItems.size();
    Log::PercentProgressCallback progress(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<T> modelWrapper(model, progress);

    PropertiesCollector<T> selectedProps;
    modelWrapper.ForSpecifiedItems(selectedItems, selectedProps);
    PropertiesFilter<T> filter(visitor, selectedProps.GetResult());
    modelWrapper.ForAllItems(filter);
  }

  template<class T>
  void VisitOnlySelectedItems(const PropertyModel<T>& model, const Playlist::Model::IndexSet& selectedItems,
                              Log::ProgressCallback& cb, typename PropertyModel<T>::Visitor& visitor)
  {
    const std::size_t totalItems = selectedItems.size();
    Log::PercentProgressCallback progress(static_cast<uint_t>(totalItems), cb);
    const PropertyModelWithProgress<uint32_t> modelWrapper(model, progress);
    modelWrapper.ForSpecifiedItems(selectedItems, visitor);
  }

  // Remove dups
  class SelectAllDupsOperation : public Playlist::Item::SelectionOperation
  {
  public:
    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
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
    explicit SelectDupsOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      ItemsWithDuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, dups);
      }
      // select all rips but delete only nonselected
      auto toRemove = dups.GetResult();
      std::for_each(SelectedItems->begin(), SelectedItems->end(),
                    [&toRemove](Playlist::Model::IndexSet::size_type idx) { toRemove->erase(idx); });
      emit ResultAcquired(std::move(toRemove));
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  class SelectDupsInSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectDupsInSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      DuplicatesCollector<uint32_t> dups;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
        VisitOnlySelectedItems(propertyModel, *SelectedItems, cb, dups);
      }
      emit ResultAcquired(dups.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  // Select  ripoffs
  class SelectAllRipOffsOperation : public Playlist::Item::SelectionOperation
  {
  public:
    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
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
    explicit SelectRipOffsOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      ItemsWithDuplicatesCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, rips);
      }
      emit ResultAcquired(rips.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  class SelectRipOffsInSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectRipOffsInSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      ItemsWithDuplicatesCollector<uint32_t> rips;
      {
        const TypedPropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
        VisitOnlySelectedItems(propertyModel, *SelectedItems, cb, rips);
      }
      emit ResultAcquired(rips.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  // other
  class SelectTypesOfSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectTypesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      IndicesCollector<String> types;
      {
        const TypedPropertyModel<String> propertyModel(stor, &Playlist::Item::Data::GetType);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, types);
      }
      emit ResultAcquired(types.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  class SelectFilesOfSelectedOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SelectFilesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      IndicesCollector<String> files;
      {
        const TypedPropertyModel<String> propertyModel(stor, &Playlist::Item::Data::GetFilePath);
        VisitAsSelectedItems(propertyModel, *SelectedItems, cb, files);
      }
      emit ResultAcquired(files.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };

  class InvalidModulesCollection : public Playlist::Item::Visitor
  {
  public:
    InvalidModulesCollection()
      : Result(MakeRWPtr<Playlist::Model::IndexSet>())
    {}

    void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data) override
    {
      // check for the data first to define is data valid or not
      if (data->GetState().GetIfError())
      {
        Result->insert(index);
      }
    }

    Playlist::Model::IndexSet::Ptr GetResult() const
    {
      return Result;
    }

  private:
    const Playlist::Model::IndexSet::RWPtr Result;
  };

  class SelectUnavailableOperation : public Playlist::Item::SelectionOperation
  {
  public:
    SelectUnavailableOperation() = default;

    explicit SelectUnavailableOperation(Playlist::Model::IndexSet::Ptr items)
      : SelectedItems(std::move(items))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      InvalidModulesCollection invalids;
      ExecuteOperation(stor, SelectedItems, invalids, cb);
      emit ResultAcquired(invalids.GetResult());
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
  };
}  // namespace

namespace Playlist::Item
{
  SelectionOperation::Ptr CreateSelectAllRipOffsOperation()
  {
    return MakePtr<SelectAllRipOffsOperation>();
  }

  SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectRipOffsOfSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectRipOffsInSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectAllDuplicatesOperation()
  {
    return MakePtr<SelectAllDupsOperation>();
  }

  SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectDupsOfSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectDupsInSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectTypesOfSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectFilesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectFilesOfSelectedOperation>(std::move(items));
  }

  SelectionOperation::Ptr CreateSelectAllUnavailableOperation()
  {
    return MakePtr<SelectUnavailableOperation>();
  }

  SelectionOperation::Ptr CreateSelectUnavailableInSelectedOperation(Playlist::Model::IndexSet::Ptr items)
  {
    return MakePtr<SelectUnavailableOperation>(std::move(items));
  }
}  // namespace Playlist::Item
