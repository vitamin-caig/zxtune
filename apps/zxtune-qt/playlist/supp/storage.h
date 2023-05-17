/**
 *
 * @file
 *
 * @brief Playlist storage interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "model.h"

namespace Playlist::Item
{
  class Comparer
  {
  public:
    using Ptr = std::shared_ptr<const Comparer>;
    virtual ~Comparer() = default;

    virtual bool CompareItems(const Data& lh, const Data& rh) const = 0;
  };

  class Visitor
  {
  public:
    virtual ~Visitor() = default;

    virtual void OnItem(Model::IndexType index, Item::Data::Ptr data) = 0;
  };

  class Storage
  {
  public:
    using Ptr = std::shared_ptr<Storage>;

    virtual ~Storage() = default;

    // meta
    virtual Ptr Clone() const = 0;
    virtual Model::OldToNewIndexMap::Ptr ResetIndices() = 0;
    virtual unsigned GetVersion() const = 0;

    // create
    virtual void Add(Data::Ptr item) = 0;
    virtual void Add(Collection::Ptr items) = 0;
    // read
    virtual std::size_t CountItems() const = 0;
    virtual Data::Ptr GetItem(Model::IndexType idx) const = 0;
    virtual Collection::Ptr GetItems() const = 0;

    virtual void ForAllItems(Visitor& visitor) const = 0;
    virtual void ForSpecifiedItems(const Model::IndexSet& indices, Visitor& visitor) const = 0;
    // update
    virtual void MoveItems(const Model::IndexSet& indices, Model::IndexType destination) = 0;
    virtual void Sort(const Comparer& cmp) = 0;
    virtual void Shuffle() = 0;
    // delete
    virtual void RemoveItems(const Model::IndexSet& indices) = 0;

    static Ptr Create();
  };
}  // namespace Playlist::Item
