/*
Abstract:
  Playlist data source interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED

//local includes
#include "data_provider.h"
//qt includes
#include <QtCore/QStringList>

namespace Playlist
{
  class ScannerCallback
  {
  public:
    virtual ~ScannerCallback() {}

    virtual bool IsCanceled() const = 0;

    virtual void OnItem(const Item::Data::Ptr& item) = 0;
    virtual void OnProgress(unsigned progress, unsigned curItem) = 0;
    virtual void OnReport(const QString& report, const QString& item) = 0;
    virtual void OnError(const class Error& err) = 0;
  };

  class ScannerSource
  {
  public:
    typedef boost::shared_ptr<ScannerSource> Ptr;

    virtual ~ScannerSource() {}

    virtual unsigned Resolve() = 0;
    virtual void Process() = 0;

    static Ptr CreateOpenFileSource(Item::DataProvider::Ptr provider, ScannerCallback& callback, const QStringList& items);
    static Ptr CreateDetectFileSource(Item::DataProvider::Ptr provider, ScannerCallback& callback, const QStringList& items);
    static Ptr CreateIteratorSource(ScannerCallback& callback, Item::Data::Iterator::Ptr iterator, int countHint = -1);
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED
