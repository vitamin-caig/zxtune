/*
Abstract:
  Playlist items storage implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "storage.h"
//common includes
#include <debug_log.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/iterator/counting_iterator.hpp>

namespace
{
  const Debug::Stream Dbg("Playlist::Storage");

  typedef std::pair<Playlist::Item::Data::Ptr, Playlist::Model::IndexType> IndexedItem;

  //simple std::list wrapper that guarantees contstant complexivity of size() method
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
    {
    }

    ItemsContainer(const ItemsContainer& rh)
      : Parent(rh)
      , Size(rh.Size)
    {
    }

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
  private:
    std::size_t Size;
  };

  template<class IteratorType>
  class IteratorContainerWalker
  {
  public:
    virtual ~IteratorContainerWalker() {}

    virtual void OnItem(IteratorType it) = 0;
  };

  class PlaylistItemVisitorAdapter : public IteratorContainerWalker<ItemsContainer::const_iterator>
  {
  public:
    explicit PlaylistItemVisitorAdapter(Playlist::Item::Visitor& delegate)
      : Delegate(delegate)
    {
    }

    virtual void OnItem(ItemsContainer::const_iterator it)
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
    {
    }

    virtual ~RemoveItemsWalker()
    {
      Erase();
    }

    virtual void OnItem(ItemsContainer::iterator it)
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
    {
    }

    virtual ~MoveItemsWalker()
    {
      Splice();
    }

    virtual void OnItem(ItemsContainer::iterator it)
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

  const std::size_t CACHE_THRESHOLD = 200;

  class LinearStorage : public Item::Storage
  {
  public:
    LinearStorage()
      : Version(0)
    {
    }

    LinearStorage(const LinearStorage& rh)
      : Version(0)
      , Items(rh.Items)
    {
    }

    virtual Item::Storage::Ptr Clone() const
    {
      return Item::Storage::Ptr(new LinearStorage(*this));
    }
   
    virtual Model::OldToNewIndexMap::Ptr ResetIndices()
    {
      const boost::shared_ptr<Model::OldToNewIndexMap> result = boost::make_shared<Model::OldToNewIndexMap>();
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0), std::inserter(*result, result->end()),
        boost::bind(&MakeIndexPair, _1, _2));
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0), Items.begin(),
        boost::bind(&UpdateItemIndex, _1, _2));
      return result;
    }

    virtual unsigned GetVersion() const
    {
      return Version;
    }

    virtual void AddItem(Item::Data::Ptr item)
    {
      const IndexedItem idxItem(item, static_cast<Model::IndexType>(Items.size()));
      Items.push_back(idxItem);
      Modify();
    }

    virtual std::size_t CountItems() const
    {
      return Items.size();
    }

    virtual Item::Data::Ptr GetItem(Model::IndexType idx) const
    {
      if (idx >= Model::IndexType(Items.size()))
      {
        return Item::Data::Ptr();
      }
      const ItemsContainer::const_iterator it = GetIteratorByIndex(idx);
      return it->first;
    }

    virtual void ForAllItems(Playlist::Item::Visitor& visitor) const
    {
      for (ItemsContainer::const_iterator it = Items.begin(), lim = Items.end(); it != lim; ++it)
      {
        visitor.OnItem(it->second, it->first);
      }
    }

    virtual void ForSpecifiedItems(const Model::IndexSet& indices, Playlist::Item::Visitor& visitor) const
    {
      PlaylistItemVisitorAdapter walker(visitor);
      ForChoosenItems(indices, walker);
    }

    virtual void MoveItems(const Model::IndexSet& indices, Model::IndexType destination)
    {
      if (!indices.count(destination))
      {
        MoveItemsInternal(indices, destination);
      }
      else
      {
        //try to move at the first nonselected and place before it
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

    virtual void Sort(const Item::Comparer& cmp)
    {
      Items.sort(ComparerWrapper(cmp));
      ClearCache();
      Modify();
    }

    virtual void RemoveItems(const Model::IndexSet& indices)
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
      {
      }

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
        if (absolute(delta) > std::ptrdiff_t(CACHE_THRESHOLD))
        {
          Dbg("Cached iterator for idx=%1%. Nearest idx=%2%, delta=%3%", idx, entry.first, delta);
          entry.first += delta;
          IteratorsCache.insert(entry);
        }
      }
      return entry.second;
    }

    IndexToIterator::value_type GetNearestIterator(Model::IndexType idx) const
    {
      const IndexToIterator::value_type predefinedEntry = GetNearestPredefinedIterator(idx);
      const std::size_t predefinedDelta = absolute<std::ptrdiff_t>(std::ptrdiff_t(idx) - predefinedEntry.first);
      if (predefinedDelta <= CACHE_THRESHOLD || IteratorsCache.empty())
      {
        return predefinedEntry;
      }
      const IndexToIterator::value_type cachedEntry = GetNearestCachedIterator(idx);
      const std::size_t cachedDelta = absolute<std::ptrdiff_t>(std::ptrdiff_t(idx) - cachedEntry.first);
      if (cachedDelta <= CACHE_THRESHOLD)
      {
        return cachedEntry;
      }
      return predefinedDelta <= cachedDelta
        ? predefinedEntry
        : cachedEntry;
    }

    IndexToIterator::value_type GetNearestPredefinedIterator(Model::IndexType idx) const
    {
      const Model::IndexType firstIndex = 0;
      const Model::IndexType lastIndex = Model::IndexType(Items.size() - 1);
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
      const IndexToIterator::const_iterator upper = IteratorsCache.upper_bound(idx);
      if (upper == IteratorsCache.begin())
      {
        return *upper;
      }
      IndexToIterator::const_iterator lower = upper;
      --lower;
      if (upper == IteratorsCache.end())
      {
        return *lower;
      }
      //upper->first > idx
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
      ItemsContainer::iterator delimiter = GetIteratorByIndex(destination);

      ItemsContainer movedItems;
      {
        MoveItemsWalker walker(Items, movedItems);
        ForChoosenItems(indices, walker);
      }
      assert(indices.size() == movedItems.size());

      ItemsContainer afterItems;
      afterItems.splice(afterItems.begin(), Items, delimiter, Items.end());

      //gathering back
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
        //all items
        for (IteratorType it = Items.begin(), lim = Items.end(); it != lim; )
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
        for (Playlist::Model::IndexSet::const_iterator idxIt = indices.begin(), idxLim = indices.end(); idxIt != idxLim; ++idxIt)
        {
          const Playlist::Model::IndexType curIndex = *idxIt;
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
}

namespace Playlist
{
  namespace Item
  {
    Storage::Ptr Storage::Create()
    {
      return boost::make_shared<LinearStorage>();
    }
  }
}
