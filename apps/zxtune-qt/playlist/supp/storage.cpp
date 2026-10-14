/**
 *
 * @file
 *
 * @brief Playlist storage implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/supp/storage.h"

#include "debug/log.h"

#include "make_ptr.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

namespace
{
  const Debug::Stream Dbg("Playlist::Storage");

  using IndexedItem = std::pair<Playlist::Item::Data::Ptr, Playlist::Model::IndexType>;

  using ItemsContainer = std::vector<IndexedItem>;

  using namespace Playlist;

  class ItemsCollection : public Item::Collection
  {
  public:
    ItemsCollection(ItemsContainer::const_iterator begin, ItemsContainer::const_iterator end)
      : Current(begin)
      , Limit(end)
    {}

    bool IsValid() const override
    {
      return Current != Limit;
    }

    Item::Data::Ptr Get() const override
    {
      return Current->first;
    }

    void Next() override
    {
      ++Current;
    }

  private:
    ItemsContainer::const_iterator Current;
    const ItemsContainer::const_iterator Limit;
  };

  class LinearStorage : public Item::Storage
  {
  public:
    LinearStorage()
    {
      Dbg("Created at {}", Self());
    }

    LinearStorage(const LinearStorage& rh)
      : Items(rh.Items)
    {
      Dbg("Created at {} (cloned from {} with {} items)", Self(), rh.Self(), Items.size());
    }

    ~LinearStorage() override
    {
      Dbg("Destroyed at {} with {} items", Self(), Items.size());
    }

    Item::Storage::Ptr Clone() const override
    {
      return MakePtr<LinearStorage>(*this);
    }

    Model::OldToNewIndexMap::Ptr ResetIndices() override
    {
      // TODO: use array in mapping
      auto result = MakeRWPtr<Model::OldToNewIndexMap>();
      Model::IndexType newIdx = 0;
      for (auto& item : Items)
      {
        const auto oldIdx = std::exchange(item.second, newIdx);
        result->emplace(oldIdx, newIdx);
        ++newIdx;
      }
      return result;
    }

    unsigned GetVersion() const override
    {
      return Version;
    }

    void Add(Item::Data::Ptr item) override
    {
      Items.emplace_back(std::move(item), static_cast<Model::IndexType>(Items.size()));
      Modify();
    }

    void Add(Item::Collection::Ptr items) override
    {
      for (auto idx = static_cast<Model::IndexType>(Items.size()); items->IsValid(); items->Next(), ++idx)
      {
        Items.emplace_back(items->Get(), idx);
      }
      Modify();
    }

    std::size_t CountItems() const override
    {
      return Items.size();
    }

    Item::Data::Ptr GetItem(Model::IndexType idx) const override
    {
      if (idx >= Model::IndexType(Items.size()))
      {
        return {};
      }
      return Items[idx].first;
    }

    Item::Collection::Ptr GetItems() const override
    {
      return MakePtr<ItemsCollection>(Items.begin(), Items.end());
    }

    void ForAllItems(Item::Visitor& visitor) const override
    {
      for (const auto& item : Items)
      {
        visitor.OnItem(item.second, item.first);
      }
    }

    void ForSpecifiedItems(const Model::IndexSet& indices, Playlist::Item::Visitor& visitor) const override
    {
      if (Items.empty() || indices.empty())
      {
        return;
      }
      assert(*indices.rbegin() < Items.size());
      for (const auto idx : indices)
      {
        visitor.OnItem(Items[idx].second, Items[idx].first);
      }
    }

    void MoveItems(const Model::IndexSet& indices, Model::IndexType destination) override
    {
      if (indices.empty())
      {
        return;
      }
      if (indices.count(destination))
      {
        // try to move at the first nonselected and place before it
        Model::IndexType moveAfter = destination;
        while (moveAfter != Items.size() && indices.count(moveAfter))
        {
          ++moveAfter;
        }
        destination = moveAfter;
      }
      MoveItemsInternal(indices, destination);
    }

    void Sort(const Item::Comparer& cmp) override
    {
      std::stable_sort(Items.begin(), Items.end(), ComparerWrapper(cmp));
      Modify();
    }

    void Shuffle() override
    {
      std::shuffle(Items.begin(), Items.end(), std::mt19937(std::random_device()()));
      Modify();
    }

    void RemoveItems(const Model::IndexSet& indices) override
    {
      if (indices.empty())
      {
        return;
      }
      assert(*indices.rbegin() < Items.size());
      std::size_t out = 0;
      for (std::size_t in = 0; in != Items.size(); ++in)
      {
        if (!indices.count(static_cast<Model::IndexType>(in)))
        {
          if (out != in)
          {
            Items[out] = std::move(Items[in]);
          }
          ++out;
        }
      }
      Items.resize(out);
      Modify();
    }

  private:
    const void* Self() const
    {
      return this;
    }

    class ComparerWrapper
    {
    public:
      explicit ComparerWrapper(const Item::Comparer& cmp)
        : Cmp(cmp)
      {}

      bool operator()(const ItemsContainer::value_type& lh, const ItemsContainer::value_type& rh) const
      {
        return Cmp.CompareItems(*lh.first, *rh.first);
      }

    private:
      const Item::Comparer& Cmp;
    };

    //! Places selected items right before the item originally at destination
    void MoveItemsInternal(const Model::IndexSet& indices, Model::IndexType destination)
    {
      assert(!indices.count(destination));

      ItemsContainer moved;
      moved.reserve(indices.size());
      for (const auto idx : indices)
      {
        assert(idx < Items.size());
        moved.push_back(std::move(Items[idx]));
      }

      ItemsContainer result;
      result.reserve(Items.size());
      const auto total = static_cast<Model::IndexType>(Items.size());
      for (Model::IndexType idx = 0; idx <= total; ++idx)
      {
        if (idx == destination)
        {
          result.insert(result.end(), std::make_move_iterator(moved.begin()), std::make_move_iterator(moved.end()));
        }
        if (idx == total)
        {
          break;
        }
        if (!indices.count(idx))
        {
          result.push_back(std::move(Items[idx]));
        }
      }
      Items = std::move(result);
      Modify();
    }

    void Modify()
    {
      ++Version;
    }

  private:
    unsigned Version = 0;
    ItemsContainer Items;
  };
}  // namespace

namespace Playlist::Item
{
  Storage::Ptr Storage::Create()
  {
    return MakePtr<LinearStorage>();
  }
}  // namespace Playlist::Item
