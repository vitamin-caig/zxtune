/*
Abstract:
  Playlist container view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_CONTAINER_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_CONTAINER_VIEW_H_DEFINED

//common includes
#include <parameters.h>
//qt includes
#include <QtGui/QWidget>

class QMenu;
class QStringList;

namespace Playlist
{
  namespace Item
  {
    class Data;
  }

  namespace UI
  {
    class ContainerView : public QWidget
    {
      Q_OBJECT
    protected:
      explicit ContainerView(QWidget& parent);
    public:
      //creator
      static ContainerView* Create(QWidget& parent, Parameters::Container::Ptr parameters);

      virtual void CreatePlaylist(const QStringList& items) = 0;
      virtual QMenu* GetActionsMenu() const = 0;

    public slots:
      //navigate
      virtual void Play() = 0;
      virtual void Pause() = 0;
      virtual void Stop() = 0;
      virtual void Finish() = 0;
      virtual void Next() = 0;
      virtual void Prev() = 0;
      //actions
      virtual void Clear() = 0;
      virtual void AddFiles() = 0;
      virtual void AddFolder() = 0;
      //playlist actions
      virtual void CreatePlaylist() = 0;
      virtual void LoadPlaylist() = 0;
      virtual void SavePlaylist() = 0;

      virtual void CloseCurrentPlaylist() = 0;
      virtual void ClosePlaylist(int index) = 0;
    private slots:
      virtual void PlaylistItemActivated(const Playlist::Item::Data&) = 0;
    signals:
      void OnItemActivated(const Playlist::Item::Data&);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_CONTAINER_VIEW_H_DEFINED
