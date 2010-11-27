/*
Abstract:
  Playlist model

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED

//local includes
#include "data.h"
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

    //creator
    static Ptr Create(QObject& parent);

    //accessors
    virtual unsigned CountItems() const = 0;
    virtual Item::Data::Ptr GetItem(unsigned index) const = 0;
    virtual Item::Data::Iterator::Ptr GetItems() const = 0;
    virtual Item::Data::Iterator::Ptr GetItems(const QSet<unsigned>& items) const = 0;
    //modifiers
    virtual void AddItems(Item::Data::Iterator::Ptr iter) = 0;
    virtual void Clear() = 0;
    virtual void RemoveItems(const QSet<unsigned>& items) = 0;
  public slots:
    virtual void AddItem(Playlist::Item::Data::Ptr item) = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_MODEL_H_DEFINED
