/*
Abstract:
  Playlist container view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_CONTAINER_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_CONTAINER_VIEWH_DEFINED

//qt includes
#include <QtGui/QWidget>

class PlaylistContainerView : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaylistContainerView(QWidget& parent);
public:
  //creator
  static PlaylistContainerView* Create(QWidget& parent);

  virtual void CreatePlaylist(const class QStringList& items) = 0;
  virtual class QMenu* GetActionsMenu() const = 0;

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
  virtual void AddFolders() = 0;
  //playlist actions
  virtual void CreatePlaylist() = 0;
  virtual void LoadPlaylist() = 0;
  virtual void SavePlaylist() = 0;

  virtual void CloseCurrentPlaylist() = 0;
  virtual void ClosePlaylist(int index) = 0;
private slots:
  virtual void PlaylistItemActivated(const class Playitem&) = 0;
signals:
  void OnItemActivated(const class Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_VIEW_H_DEFINED
