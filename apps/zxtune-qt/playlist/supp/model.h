/*
Abstract:
  Playlist model

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
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

    //creator
    static Model* Create(QObject& parent);

    //accessors
    virtual Playitem::Ptr GetItem(unsigned index) const = 0;
    virtual Playitem::Iterator::Ptr GetItems() const = 0;
    virtual Playitem::Iterator::Ptr GetItems(const QSet<unsigned>& items) const = 0;
    //modifiers
    virtual void AddItems(Playitem::Iterator::Ptr iter) = 0;
    virtual void Clear() = 0;
    virtual void RemoveItems(const QSet<unsigned>& items) = 0;
  public slots:
    virtual void AddItem(Playitem::Ptr item) = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED
