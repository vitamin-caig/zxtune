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

      virtual const Playlist::Controller& GetPlaylist() const = 0;
    public slots:
      virtual void AddItems(const QStringList& items) = 0;

      virtual void Play() = 0;
      virtual void Pause() = 0;
      virtual void Stop() = 0;
      virtual void Finish() = 0;
      virtual void Next() = 0;
      virtual void Prev() = 0;
      virtual void Clear() = 0;
    private slots:
      virtual void ListItemActivated(unsigned idx, const Playlist::Item::Data& data) = 0;
      virtual void Enable() = 0;
      virtual void Disable() = 0;
    signals:
      void OnItemActivated(const Playlist::Item::Data&);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_VIEW_H_DEFINED
