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

//local includes
#include "playlist/supp/controller.h"
#include "playlist/supp/data.h"
//common includes
#include <parameters.h>
//qt includes
#include <QtGui/QWidget>

class QMenu;
class QStringList;

namespace Playlist
{
  namespace UI
  {
    class View;

    class ContainerView : public QWidget
    {
      Q_OBJECT
    protected:
      explicit ContainerView(QWidget& parent);
    public:
      //creator
      static ContainerView* Create(QWidget& parent, Parameters::Container::Ptr parameters);

      virtual void Setup(const QStringList& items) = 0;
      virtual void Teardown() = 0;

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
      virtual void RenamePlaylist() = 0;

      virtual void CloseCurrentPlaylist() = 0;
      virtual void ClosePlaylist(int index) = 0;
    private slots:
      virtual void OnPlaylistCreated(Playlist::Controller::Ptr) = 0;
      virtual void OnPlaylistRenamed(const QString& name) = 0;
      virtual void PlaylistItemActivated(Playlist::Item::Data::Ptr) = 0;
    signals:
      void OnItemActivated(Playlist::Item::Data::Ptr);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_CONTAINER_VIEW_H_DEFINED
