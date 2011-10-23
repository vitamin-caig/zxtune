/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED

//local includes
#include "data_provider.h"
#include "playlist/io/container.h"
#include "playlist/supp/model.h"
#include "playlist/supp/scanner.h"
//qt includes
#include <QtCore/QObject>

namespace Playlist
{
  namespace Item
  {
    //dynamic part
    enum State
    {
      STOPPED,
      PAUSED,
      PLAYING,
      ERROR,
    };

    enum Playorder
    {
      LOOPED = 1,
      RANDOMIZED = 2
    };

    class StateCallback
    {
    public:
      virtual ~StateCallback() {}

      virtual State GetState(const QModelIndex& index) const = 0;
    };

    class Iterator : public QObject
    {
      Q_OBJECT
    protected:
      explicit Iterator(QObject& parent);
    public:
      typedef Iterator* Ptr;

      //access
      virtual const Data* GetData() const = 0;
      virtual unsigned GetIndex() const = 0;
      virtual State GetState() const = 0;
      //change
      virtual void SetState(State state) = 0;
      //navigate
      virtual bool Next(unsigned playorderMode) = 0;
      virtual bool Prev(unsigned playorderMode) = 0;
    public slots:
      //navigate
      virtual void Reset(unsigned idx) = 0;
      //updates
      virtual void IndexesChanged(const Playlist::Model::OldToNewIndexMap& remapping) = 0;
    signals:
      void OnListItemActivated(unsigned idx, const Playlist::Item::Data& data);
    };
  }

  class Controller : public QObject
  {
    Q_OBJECT
  protected:
    explicit Controller(QObject& parent);
  public:
    typedef boost::shared_ptr<const Controller> Ptr;

    static Ptr Create(QObject& parent, const QString& name, Item::DataProvider::Ptr provider);

    virtual QString GetName() const = 0;
    virtual Scanner::Ptr GetScanner() const = 0;
    virtual Model::Ptr GetModel() const = 0;
    virtual Item::Iterator::Ptr GetIterator() const = 0;

    virtual IO::Container::Ptr GetContainer() const = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED
