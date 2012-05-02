/*
Abstract:
  Playlist scanner interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED

//local includes
#include "data_provider.h"
#include "playlist/io/container.h"
//qt includes
#include <QtCore/QThread>

namespace Playlist
{
  class Scanner : public QThread
  {
    Q_OBJECT
  protected:
    explicit Scanner(QObject& parent);
  public:
    typedef Scanner* Ptr;

    static Ptr Create(QObject& parent, Item::DataProvider::Ptr provider);

    virtual void AddItems(const QStringList& items, bool deepScan) = 0;
  public slots:
    //asynchronous, doesn't wait until real stop
    virtual void Cancel() = 0;
  signals:
    //for UI
    void OnScanStart();
    void OnProgressStatus(unsigned progress, unsigned itemsDone, unsigned totalItems);
    void OnProgressMessage(const QString& message, const QString& item);
    void OnScanStop();
    void OnResolvingStart();
    void OnResolvingStop();
    //for BL
    void OnGetItem(Playlist::Item::Data::Ptr);
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED
