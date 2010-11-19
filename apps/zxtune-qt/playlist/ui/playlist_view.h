/*
Abstract:
  Playlist view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace Playlist
{
  class Support;

  namespace UI
  {
    class View : public QWidget
    {
      Q_OBJECT
    protected:
      explicit View(QWidget& parent);
    public:
      static View* Create(QWidget& parent, Playlist::Support& playlist);

      virtual const class Playlist::Support& GetPlaylist() const = 0;
    public slots:
      virtual void Update() = 0;
    signals:
      void OnItemActivated(const class Playitem&);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
