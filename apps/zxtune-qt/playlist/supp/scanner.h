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
  class ScanStatus
  {
  public:
    typedef boost::shared_ptr<const ScanStatus> Ptr;
    virtual ~ScanStatus() {}

    virtual unsigned DoneFiles() const = 0;
    virtual unsigned FoundFiles() const = 0;
    virtual QString CurrentFile() const = 0;
    virtual bool SearchFinished() const = 0;
  };

  class Scanner : public QObject
  {
    Q_OBJECT
  protected:
    explicit Scanner(QObject& parent);
  public:
    typedef Scanner* Ptr;

    static Ptr Create(QObject& parent, Item::DataProvider::Ptr provider);

    virtual void AddItems(const QStringList& items) = 0;
  public slots:
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Stop() = 0;
  signals:
    //for UI
    void ScanStarted(Playlist::ScanStatus::Ptr status);
    void ScanProgressChanged(unsigned progress);
    void ScanMessageChanged(const QString& message);
    void ScanStopped();
    //for BL
    void ItemFound(Playlist::Item::Data::Ptr item);
    void ErrorOccurred(const Error& e);
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED
