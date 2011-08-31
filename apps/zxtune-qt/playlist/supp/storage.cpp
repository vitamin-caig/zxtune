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
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/iterator/counting_iterator.hpp>

namespace
{
  template<class Container, class IteratorType>
  void GetChoosenItems(Container& items, const Playlist::Model::IndexSet& indices, std::vector<IteratorType>& result)
  {
    if (items.empty() || indices.empty())
    {
      result.clear();
      return;
    }
    assert(*indices.rbegin() < items.size());
    std::vector<IteratorType> choosenItems;
    choosenItems.reserve(indices.size());
    if (indices.size() == items.size())
    {
      //all items
      for (IteratorType it = items.begin(), lim = items.end(); it != lim; ++it)
      {
        choosenItems.push_back(it);
      }
    }
    else
    {
      Playlist::Model::IndexType lastIndex = 0;
      IteratorType lastIterator = items.begin();
      for (Playlist::Model::IndexSet::const_iterator idxIt = indices.begin(), idxLim = indices.end(); idxIt != idxLim; ++idxIt)
      {
        const Playlist::Model::IndexType curIndex = *idxIt;
        assert(curIndex >= lastIndex);
        if (const Playlist::Model::IndexType delta = curIndex - lastIndex)
        {
          std::advance(lastIterator, delta);
        }
        choosenItems.push_back(lastIterator);
        lastIndex = curIndex;
      }
    }
    choosenItems.swap(result);
  }

  using namespace Playlist;

  class LinearStorage : public Item::Storage
  {
  public:
    virtual Item::Storage::Ptr Clone() const
    {
      return Item::Storage::Ptr(new LinearStorage(*this));
    }
   
    virtual void GetIndexRemapping(Model::OldToNewIndexMap& idxMap) const
    {
      Model::OldToNewIndexMap result;
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0), std::inserter(result, result.end()),
        boost::bind(&MakeIndexPair, _1, _2));
      idxMap.swap(result);
    }

    virtual void ResetIndices()
    {
      std::transform(Items.begin(), Items.end(), boost::counting_iterator<Model::IndexType>(0), Items.begin(),
        boost::bind(&UpdateItemIndex, _1, _2));
    }

    virtual void AddItem(Item::Data::Ptr item)
    {
      const IndexedItem idxItem(item, static_cast<Model::IndexType>(Items.size()));
      Items.push_back(idxItem);
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

    virtual void ForAllItems(Model::Visitor& visitor) const
    {
      for (ItemsContainer::const_iterator it = Items.begin(), lim = Items.end(); it != lim; ++it)
      {
        visitor.OnItem(it->second, it->first);
      }
    }

    virtual void ForSpecifiedItems(const Model::IndexSet& indices, Model::Visitor& visitor) const
    {
      std::vector<ItemsContainer::const_iterator> choosenIterators;
      GetChoosenItems(Items, indices, choosenIterators);
      for (std::vector<ItemsContainer::const_iterator>::const_iterator it = choosenIterators.begin(), lim = choosenIterators.end(); it != lim; ++it)
      {
        const IndexedItem& item = **it;
        visitor.OnItem(item.second, item.first);
      }
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
    }

    virtual void RemoveItems(const Model::IndexSet& indices)
    {
      if (indices.empty())
      {
        return;
      }
      std::vector<ItemsContainer::iterator> itersToRemove;
      GetChoosenItems(Items, indices, itersToRemove);
      assert(indices.size() == itersToRemove.size());
      std::for_each(itersToRemove.begin(), itersToRemove.end(),
        boost::bind(&ItemsContainer::erase, &Items, _1));
    }
  private:
    typedef std::pair<Item::Data::Ptr, Model::IndexType> IndexedItem;
    typedef std::list<IndexedItem> ItemsContainer;

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

    ItemsContainer::const_iterator GetIteratorByIndex(Model::IndexType idx) const
    {
      ItemsContainer::const_iterator it = Items.begin();
      std::advance(it, idx);
      return it;
    }

    ItemsContainer::iterator GetIteratorByIndex(Model::IndexType idx)
    {
      ItemsContainer::iterator it = Items.begin();
      std::advance(it, idx);
      return it;
    }

    void MoveItemsInternal(const Model::IndexSet& indices, Model::IndexType destination)
    {
      if (indices.empty())
      {
        return;
      }
      assert(!indices.count(destination));
      ItemsContainer::iterator delimiter = GetIteratorByIndex(destination);

      std::vector<ItemsContainer::iterator> movedIters;
      GetChoosenItems(Items, indices, movedIters);
      assert(indices.size() == movedIters.size());

      ItemsContainer movedItems;
      std::for_each(movedIters.begin(), movedIters.end(), 
        boost::bind(&ItemsContainer::splice, &movedItems, movedItems.end(), boost::ref(Items), _1));
      
      ItemsContainer afterItems;
      afterItems.splice(afterItems.begin(), Items, delimiter, Items.end());

      //gathering back
      Items.splice(Items.end(), movedItems);
      Items.splice(Items.end(), afterItems);
    }
  private:
    ItemsContainer Items;
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