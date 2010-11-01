/*
Abstract:
  Playlist scanner interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_THREAD_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_THREAD_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QThread>

class PlaylistScanner : public QThread
{
  Q_OBJECT
public:
  static PlaylistScanner* Create(QObject* owner, PlayitemsProvider::Ptr provider);

  virtual void AddItems(const QStringList& items, bool deepScan) = 0;
public slots:
  //asynchronous, doesn't wait until real stop
  virtual void Cancel() = 0;
signals:
  //for UI
  void OnScanStart();
  void OnProgress(unsigned progress, unsigned itemsDone, unsigned totalItems);
  void OnProgressMessage(const QString& message, const QString& item);
  void OnScanStop();
  //for BL
  void OnGetItem(Playitem::Ptr);
};

#endif //ZXTUNE_QT_PLAYLIST_THREAD_H_DEFINED
