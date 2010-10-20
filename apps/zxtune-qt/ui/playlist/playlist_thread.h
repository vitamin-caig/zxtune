/*
Abstract:
  Playitems process thread interface

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

class ProcessThread : public QThread
{
  Q_OBJECT
public:
  static ProcessThread* Create(QObject* owner, PlayitemsProvider::Ptr provider);

  virtual void AddItems(const QStringList& items) = 0;
public slots:
  virtual void Cancel() = 0;
signals:
  void OnScanStart();
  //files processing
  void OnProgress(unsigned progress, unsigned itemsDone, unsigned totalItems);
  void OnProgressMessage(const QString& message, const QString& item);
  void OnGetItem(Playitem::Ptr);
  void OnScanStop();
};

#endif //ZXTUNE_QT_PLAYLIST_THREAD_H_DEFINED
