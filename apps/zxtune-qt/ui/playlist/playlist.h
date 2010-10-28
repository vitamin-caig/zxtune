/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

class Playlist : public QObject
{
  Q_OBJECT
public:
  static Playlist* Create(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider);

  virtual class PlaylistScanner& GetScanner() const = 0;
  virtual class PlaylistModel& GetModel() const = 0;
};

class PlaylistWidget : public QWidget
{
  Q_OBJECT
public:
  static PlaylistWidget* Create(QWidget* parent, const Playlist& playlist, const class PlayitemStateCallback& callback);
signals:
  void OnItemSet(const Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
