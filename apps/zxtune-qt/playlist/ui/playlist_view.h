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

class Playitem;
namespace Playlist
{
  class Controller;

  namespace UI
  {
    class View : public QWidget
    {
      Q_OBJECT
    protected:
      explicit View(QWidget& parent);
    public:
      static View* Create(QWidget& parent, Playlist::Controller& playlist);

      virtual const Playlist::Controller& GetPlaylist() const = 0;
    public slots:
      virtual void AddItems(const QStringList& items, bool deepScan) = 0;

      virtual void Play() = 0;
      virtual void Pause() = 0;
      virtual void Stop() = 0;
      virtual void Finish() = 0;
      virtual void Next() = 0;
      virtual void Prev() = 0;
      virtual void Clear() = 0;
    signals:
      void OnItemActivated(const Playitem&);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
