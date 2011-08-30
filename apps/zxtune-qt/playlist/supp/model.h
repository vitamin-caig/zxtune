/*
Abstract:
  Playlist model

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED

//local includes
#include "data.h"
//std includes
#include <map>
#include <set>
//qt includes
#include <QtCore/QAbstractItemModel>

namespace Playlist
{
  class Model : public QAbstractItemModel
  {
    Q_OBJECT
  protected:
    explicit Model(QObject& parent);
  public:
    enum Columns
    {
      COLUMN_TYPEICON,
      COLUMN_TITLE,
      COLUMN_DURATION,

      COLUMNS_COUNT
    };

    typedef Model* Ptr;
    typedef unsigned IndexType;
    typedef std::set<IndexType> IndexSet;
    typedef std::map<IndexType, IndexType> OldToNewIndexMap;

    //creator
    static Ptr Create(QObject& parent);

    //accessors
    virtual unsigned CountItems() const = 0;
    virtual Item::Data::Ptr GetItem(IndexType index) const = 0;

    class Visitor
    {
    public:
      virtual ~Visitor() {}

      virtual void OnItem(IndexType index, const Item::Data& data) = 0;
    };
    virtual void ForAllItems(Visitor& visitor) const = 0;
    virtual void ForSpecifiedItems(const IndexSet& items, Visitor& visitor) const = 0;

    virtual Item::Data::Iterator::Ptr GetItems() const = 0;
    //modifiers
    virtual void AddItems(Item::Data::Iterator::Ptr iter) = 0;
    virtual void Clear() = 0;
    virtual void RemoveItems(const IndexSet& items) = 0;
    virtual void MoveItems(const IndexSet& items, IndexType target) = 0;
  public slots:
    virtual void AddItem(Playlist::Item::Data::Ptr item) = 0;
  signals:
    void OnIndexesChanged(const Playlist::Model::OldToNewIndexMap& map);
    void OnSortStart();
    void OnSortStop();
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED
