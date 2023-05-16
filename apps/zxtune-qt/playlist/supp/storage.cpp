/**
 *
 * @file
 *
 * @brief Playlist storage implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "storage.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <math/numeric.h>
// boost includes
#include <boost/iterator/counting_iterator.hpp>

namespace
{
  const Debug::Stream Dbg("Playlist::Storage");

  typedef std::pair<Playlist::Item::Data::Ptr, Playlist::Model::IndexType> IndexedItem;

  // simple std::list wrapper that guarantees contstant complexivity of size() method
  class ItemsContainer : private std::list<IndexedItem>
  {
    typedef std::list<IndexedItem> Parent;

  public:
    using Parent::value_type;
    using Parent::const_iterator;
    using Parent::iterator;

    using Parent::begin;
    using Parent::end;
    using Parent::sort;

    ItemsContainer()
      : Size()
    {}

    ItemsContainer(const ItemsContainer& rh) = default;

    Parent::size_type size() const
    {
      return Size;
    }

    bool empty() const
    {
      return 0 == Size;
    }

    void push_back(const value_type& val)
    {
      Parent::push_back(val);
      ++Size;
    }

    void erase(iterator it)
    {
      Parent::erase(it);
      --Size;
    }

    void erase(iterator first, iterator last)
    {
      const std::size_t count = std::distance(first, last);
      Parent::erase(first, last);
      Size -= count;
    }

    void splice(iterator position, ItemsContainer& x, iterator i)
    {
      Parent::splice(position, x, i);
      ++Size;
      --x.Size;
    }

    void splice(iterator position, ItemsContainer& x, iterator first, iterator last)
    {
      const std::size_t count = std::distance(first, last);
      Parent::splice(position, x, first, last);
      Size += count;
      x.Size -= count;
    }

    void splice(iterator position, ItemsContainer& x)
    {
      Parent::splice(position, x);
      Size += x.Size;
      x.Size = 0;
    }

    void swap(ItemsContainer& rh)
    {
      Parent::swap(rh);
      std::swap(Size, rh.Size);
    }

  private:
    std::size_t Size;
  };

  template<class IteratorType>
  class IteratorContainerWalker
  {
  public:
    virtual ~IteratorContainerWalker() = default;

    virtual void OnItem(IteratorType it) = 0;
  };

  class PlaylistItemVisitorAdapter : public IteratorContainerWalker<ItemsContainer::const_iterator>
  {
  public:
    explicit PlaylistItemVisitorAdapter(Playlist::Item::Visitor& delegate)
      : Delegate(delegate)
    {}

    void OnItem(ItemsContainer::const_iterator it) override
    {
      Delegate.OnItem(it->second, it->first);
    }

  private:
    Playlist::Item::Visitor& Delegate;
  };

  class RemoveItemsWalker : public IteratorContainerWalker<ItemsContainer::iterator>
  {
  public:
    explicit RemoveItemsWalker(ItemsContainer& container)
      : Container(container)
      , LastRangeStart(Container.end())
      , LastRangeEnd(Container.end())
    {}

    ~RemoveItemsWalker() override
    {
      Erase();
    }

    void OnItem(ItemsContainer::iterator it) override
    {
      if (it != LastRangeEnd)
      {
        Erase();
        LastRangeStart = it;
      }
      LastRangeEnd = ++it;
    }

  private:
    void Erase()
    {
      Container.erase(LastRangeStart, LastRangeEnd);
    }

  private:
    ItemsContainer& Container;
    ItemsContainer::iterator LastRangeStart;
    ItemsContainer::iterator LastRangeEnd;
  };

  class MoveItemsWalker : public IteratorContainerWalker<ItemsContainer::iterator>
  {
  public:
    MoveItemsWalker(ItemsContainer& src, ItemsContainer& dst)
      : Src(src)
      , Dst(dst)
      , LastRangeStart(Src.end())
      , LastRangeEnd(Src.end())
    {}

    ~MoveItemsWalker() override
    {
      Splice();
    }

    void OnItem(ItemsContainer::iterator it) override
    {
      if (it != LastRangeEnd)
      {
        Splice();
        LastRangeStart = it;
      }
      LastRangeEnd = ++it;
    }

  private:
    void Splice()
    {
      Dst.splice(Dst.end(), Src, LastRangeStart, LastRangeEnd);
    }

  private:
    ItemsContainer& Src;
    ItemsContainer& Dst;
    ItemsContainer::iterator LastRangeStart;
    ItemsContainer::iterator LastRangeEnd;
  };

  using namespace Playlist;

  class ItemsCollection : public Item::Collection
  {
  public:
    ItemsCollection(ItemsContainer::const_iterator begin, ItemsContainer::const_iterator end)
      : Current(std::move(begin))
      , Limit(std::move(end))
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

  const std::size_t CACHE_THRESHOLD = 200;

  class LinearStorage : public Item::Storage
  {
  public:
    LinearStorage()
      : Version(0)
    {
      Dbg("Created at {}", Self());
    }

    LinearStorage(const LinearStorage& rh)
      : Version(0)
      , Items(rh.Items)
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
      const Model::OldToNewIndexMap::RWPtr result = MakeRWPtr<Model::OldToNewIndexMap>();
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0),
                     std::inserter(*result, result->end()), &MakeIndexPair);
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0), Items.begin(),
                     &UpdateItemIndex);
      return result;
    }

    unsigned GetVersion() const override
    {
      return Version;
    }

    void Add(Item::Data::Ptr item) override
    {
      const IndexedItem idxItem(item, static_cast<Model::IndexType>(Items.size()));
      Items.push_back(idxItem);
      Modify();
    }

    void Add(Item::Collection::Ptr items) override
    {
      for (auto idx = static_cast<Model::IndexType>(Items.size()); items->IsValid(); items->Next(), ++idx)
      {
        const IndexedItem idxItem(items->Get(), idx);
        Items.push_back(idxItem);
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
        return Item::Data::Ptr();
      }
      const auto it = GetIteratorByIndex(idx);
      return it->first;
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
      PlaylistItemVisitorAdapter walker(visitor);
      ForChoosenItems(indices, walker);
    }

    void MoveItems(const Model::IndexSet& indices, Model::IndexType destination) override
    {
      if (!indices.count(destination))
      {
        MoveItemsInternal(indices, destination);
      }
      else
      {
        // try to move at the first nonselected and place before it
        for (Model::IndexType moveAfter = destination; moveAfter != Items.size(); ++moveAfter)
        {
          if (!indices.count(moveAfter))
          {
            MoveItemsInternal(indices, moveAfter);
            return;
          }
        }
        MoveItemsInternal(indices, static_cast<Model::IndexType>(Items.size()));
      }
    }

    void Sort(const Item::Comparer& cmp) override
    {
      Items.sort(ComparerWrapper(cmp));
      ClearCache();
      Modify();
    }

    void Shuffle() override
    {
      std::vector<ItemsContainer::const_iterator> iters;
      iters.reserve(Items.size());
      for (auto it = Items.begin(), lim = Items.end(); it != lim; ++it)
      {
        iters.push_back(it);
      }
      std::random_shuffle(iters.begin(), iters.end());
      ItemsContainer newOne;
      for (const auto& it : iters)
      {
        newOne.push_back(*it);
      }
      newOne.swap(Items);
      ClearCache();
      Modify();
    }

    void RemoveItems(const Model::IndexSet& indices) override
    {
      if (indices.empty())
      {
        return;
      }
      {
        RemoveItemsWalker walker(Items);
        ForChoosenItems(indices, walker);
      }
      ClearCache();
      Modify();
    }

  private:
    const void* Self() const
    {
      return this;
    }

    static Model::OldToNewIndexMap::value_type MakeIndexPair(const IndexedItem& item, Model::IndexType idx)
    {
      return Model::OldToNewIndexMap::value_type(item.second, idx);
    }

    static IndexedItem UpdateItemIndex(const IndexedItem& item, Model::IndexType idx)
    {
      return IndexedItem(item.first, idx);
    }

    class ComparerWrapper : public std::binary_function<ItemsContainer::value_type, ItemsContainer::value_type, bool>
    {
    public:
      explicit ComparerWrapper(const Item::Comparer& cmp)
        : Cmp(cmp)
      {}

      result_type operator()(first_argument_type lh, second_argument_type rh) const
      {
        return Cmp.CompareItems(*lh.first, *rh.first);
      }

    private:
      const Item::Comparer& Cmp;
    };

    typedef std::map<Model::IndexType, ItemsContainer::iterator> IndexToIterator;

    ItemsContainer::iterator GetIteratorByIndex(Model::IndexType idx) const
    {
      std::pair<Model::IndexType, ItemsContainer::iterator> entry = GetNearestIterator(idx);
      if (const std::ptrdiff_t delta = std::ptrdiff_t(idx) - entry.first)
      {
        std::advance(entry.second, delta);
        if (Math::Absolute(delta) > std::ptrdiff_t(CACHE_THRESHOLD))
        {
          Dbg("Cached iterator for idx={}. Nearest idx={}, delta={}", idx, entry.first, delta);
          entry.first += delta;
          IteratorsCache.insert(entry);
        }
      }
      return entry.second;
    }

    IndexToIterator::value_type GetNearestIterator(Model::IndexType idx) const
    {
      const IndexToIterator::value_type predefinedEntry = GetNearestPredefinedIterator(idx);
      const std::size_t predefinedDelta = Math::Absolute<std::ptrdiff_t>(std::ptrdiff_t(idx) - predefinedEntry.first);
      if (predefinedDelta <= CACHE_THRESHOLD || IteratorsCache.empty())
      {
        return predefinedEntry;
      }
      const IndexToIterator::value_type cachedEntry = GetNearestCachedIterator(idx);
      const std::size_t cachedDelta = Math::Absolute<std::ptrdiff_t>(std::ptrdiff_t(idx) - cachedEntry.first);
      if (cachedDelta <= CACHE_THRESHOLD)
      {
        return cachedEntry;
      }
      return predefinedDelta <= cachedDelta ? predefinedEntry : cachedEntry;
    }

    IndexToIterator::value_type GetNearestPredefinedIterator(Model::IndexType idx) const
    {
      const Model::IndexType firstIndex = 0;
      const auto lastIndex = Model::IndexType(Items.size() - 1);
      const std::size_t toFirst = idx - firstIndex;
      const std::size_t toLast = lastIndex - idx;
      if (toFirst <= toLast)
      {
        return IndexToIterator::value_type(firstIndex, Items.begin());
      }
      else
      {
        return IndexToIterator::value_type(lastIndex, --Items.end());
      }
    }

    IndexToIterator::value_type GetNearestCachedIterator(Model::IndexType idx) const
    {
      assert(!IteratorsCache.empty());
      const auto upper = IteratorsCache.upper_bound(idx);
      if (upper == IteratorsCache.begin())
      {
        return *upper;
      }
      auto lower = upper;
      --lower;
      if (upper == IteratorsCache.end())
      {
        return *lower;
      }
      // upper->first > idx
      const std::size_t toLower = idx - lower->first;
      const std::size_t toUpper = upper->first - idx;
      return *(toLower <= toUpper ? lower : upper);
    }

    void MoveItemsInternal(const Model::IndexSet& indices, Model::IndexType destination)
    {
      if (indices.empty())
      {
        return;
      }
      assert(!indices.count(destination));
      auto delimiter = GetIteratorByIndex(destination);

      ItemsContainer movedItems;
      {
        MoveItemsWalker walker(Items, movedItems);
        ForChoosenItems(indices, walker);
      }
      assert(indices.size() == movedItems.size());

      ItemsContainer afterItems;
      afterItems.splice(afterItems.begin(), Items, delimiter, Items.end());

      // gathering back
      Items.splice(Items.end(), movedItems);
      Items.splice(Items.end(), afterItems);
      ClearCache();
      Modify();
    }

    template<class IteratorType>
    void ForChoosenItems(const Playlist::Model::IndexSet& indices, IteratorContainerWalker<IteratorType>& walker) const
    {
      if (Items.empty() || indices.empty())
      {
        return;
      }
      assert(*indices.rbegin() < Items.size());
      if (indices.size() == Items.size())
      {
        // all items
        for (IteratorType it = Items.begin(), lim = Items.end(); it != lim;)
        {
          const IteratorType op = it;
          ++it;
          walker.OnItem(op);
        }
      }
      else
      {
        Playlist::Model::IndexType lastIndex = *indices.begin();
        IteratorType lastIterator = GetIteratorByIndex(lastIndex);
        for (auto curIndex : indices)
        {
          assert(curIndex >= lastIndex);
          if (const Playlist::Model::IndexType delta = curIndex - lastIndex)
          {
            std::advance(lastIterator, delta);
          }
          const IteratorType op = lastIterator;
          ++lastIterator;
          lastIndex = curIndex + 1;
          walker.OnItem(op);
        }
      }
    }

    void ClearCache()
    {
      Dbg("Cleared iterators cache");
      IteratorsCache.clear();
    }

    void Modify()
    {
      ++Version;
    }

  private:
    unsigned Version;
    mutable ItemsContainer Items;
    mutable IndexToIterator IteratorsCache;
  };
}  // namespace

namespace Playlist::Item
{
  Storage::Ptr Storage::Create()
  {
    return MakePtr<LinearStorage>();
  }
}  // namespace Playlist::Item
