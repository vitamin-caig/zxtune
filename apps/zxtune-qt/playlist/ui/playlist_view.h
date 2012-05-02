/*
Abstract:
  Playlist view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_VIEW_H_DEFINED

//local includes
#include "playlist/supp/controller.h"
//qt includes
#include <QtGui/QWidget>

namespace Playlist
{
  namespace Item
  {
    class Data;
  }

  namespace UI
  {
    class View : public QWidget
    {
      Q_OBJECT
    protected:
      explicit View(QWidget& parent);
    public:
      static View* Create(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params);

      virtual Playlist::Controller::Ptr GetPlaylist() const = 0;
    public slots:
      virtual void AddItems(const QStringList& items) = 0;

      virtual void Play() = 0;
      virtual void Pause() = 0;
      virtual void Stop() = 0;
      virtual void Finish() = 0;
      virtual void Next() = 0;
      virtual void Prev() = 0;
      virtual void Clear() = 0;
      virtual void Rename() = 0;
      virtual void Save() = 0;
    private slots:
      virtual void ListItemActivated(unsigned idx, Playlist::Item::Data::Ptr data) = 0;
      virtual void LongOperationStart() = 0;
      virtual void LongOperationStop() = 0;
      virtual void LongOperationCancel() = 0;
    signals:
      void Renamed(const QString& name);
      void OnItemActivated(Playlist::Item::Data::Ptr);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_VIEW_H_DEFINED
