/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED

//local includes
#include "data_provider.h"
#include "playlist/io/container.h"
//qt includes
#include <QtCore/QObject>

namespace Playlist
{
  class Scanner;
  class Model;

  namespace Item
  {
    //dynamic part
    enum State
    {
      STOPPED,
      PAUSED,
      PLAYING
    };

    class Iterator : public QObject
    {
      Q_OBJECT
    protected:
      explicit Iterator(QObject& parent);
    public:
      //access
      virtual const Data* GetData() const = 0;
      virtual State GetState() const = 0;
      //change
      virtual void SetState(State state) = 0;
    public slots:
      //navigate
      virtual bool Reset(unsigned idx) = 0;
      virtual bool Next() = 0;
      virtual bool Prev() = 0;
    signals:
      void OnItem(const Playlist::Item::Data&);
    };
  }

  class Controller : public QObject
  {
    Q_OBJECT
  protected:
    explicit Controller(QObject& parent);
  public:
    static Controller* Create(QObject& parent, const QString& name, Item::DataProvider::Ptr provider);

    virtual QString GetName() const = 0;
    virtual Scanner& GetScanner() const = 0;
    virtual Model& GetModel() const = 0;
    virtual Item::Iterator& GetIterator() const = 0;

    virtual IO::Container::Ptr GetContainer() const = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CONTROLLER_H_DEFINED
