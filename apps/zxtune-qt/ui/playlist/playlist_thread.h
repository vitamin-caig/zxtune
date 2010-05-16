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
#include "../../playitems_provider.h"
//common includes
#include <types.h>
//qt includes
#include <QtCore/QThread>

namespace Log
{
  struct MessageData;
}

class ProcessThread : public QThread
{
  Q_OBJECT
public:
  static ProcessThread* Create(QWidget* owner);

  virtual void AddItemPath(const String& path) = 0;
public slots:
  virtual void Cancel() = 0;
signals:
  void OnScanStart();
  void OnProgress(const Log::MessageData&);
  void OnGetItem(Playitem::Ptr);
  void OnScanStop();
};

#endif //ZXTUNE_QT_PLAYLIST_THREAD_H_DEFINED
