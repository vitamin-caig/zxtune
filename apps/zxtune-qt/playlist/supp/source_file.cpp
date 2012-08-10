/*
Abstract:
  File data source implementations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "source.h"
#include "ui/utils.h"
#include "playlist/io/import.h"
//common includes
#include <debug_log.h>
#include <error.h>
//library includes
#include <core/error_codes.h>
//qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>

#define FILE_TAG 4D987852

namespace
{
  const Debug::Stream Dbg("Playlist::IO");

  void ResolveItems(Playlist::ScannerCallback& callback, QStringList& unresolved, QStringList& resolved)
  {
    while (!callback.IsCanceled() && !unresolved.empty())
    {
      const QString& curItem = unresolved.takeFirst();
      if (QFileInfo(curItem).isDir())
      {
        Dbg("Resolving directory '%1%'", FromQString(curItem));
        const QDir::Filters filter = QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden;
        for (QDirIterator iterator(curItem, filter, QDirIterator::Subdirectories); !callback.IsCanceled() && iterator.hasNext(); )
        {
          const QString& curFile = iterator.next();
          Dbg("Added '%1%'", FromQString(curFile));
          resolved.append(curFile);
        }
      }
      else
      {
        Dbg("Added '%1%'", FromQString(curItem));
        resolved.append(curItem);
      }
    }
  }

  bool ProcessAsPlaylist(Playlist::Item::DataProvider::Ptr provider, const QString& path, Playlist::ScannerCallback& callback)
  {
    const Playlist::IO::Container::Ptr playlist = Playlist::IO::Open(provider, path);
    if (!playlist.get())
    {
      return false;
    }
    playlist->ForAllItems(callback);
    return true;
  }

  class DetectParametersWrapper : public Playlist::Item::DetectParameters
  {
  public:
    DetectParametersWrapper(Playlist::ScannerCallback& callback, unsigned curItemNum, const QString& curItem)
      : Callback(callback)
      , CurrentItemNum(curItemNum)
      , CurrentItem(curItem)
    {
    }

    virtual Parameters::Container::Ptr CreateInitialAdjustedParameters() const
    {
      return Parameters::Container::Create();
    }

    virtual void ProcessItem(Playlist::Item::Data::Ptr item)
    {
      Callback.OnItem(item);
      ThrowIfCanceled();
    }

    virtual void ShowProgress(unsigned progress)
    {
      Callback.OnProgress(progress, CurrentItemNum);
      ThrowIfCanceled();
    }

    virtual void ShowMessage(const String& message)
    {
      const QString text = ToQString(message);
      Callback.OnReport(text, CurrentItem);
    }
  private:
    void ThrowIfCanceled()
    {
      if (Callback.IsCanceled())
      {
        throw Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
      }
    }
  private:
    Playlist::ScannerCallback& Callback;
    const unsigned CurrentItemNum;
    const QString& CurrentItem;
  };

  class OpenFileScanner : public Playlist::ScannerSource
  {
  public:
    OpenFileScanner(Playlist::Item::DataProvider::Ptr provider, Playlist::ScannerCallback& callback, const QStringList& items)
      : Provider(provider)
      , Callback(callback)
      , UnresolvedItems(items)
    {
    }

    virtual unsigned Resolve()
    {
      ResolveItems(Callback, UnresolvedItems, ResolvedItems);
      return ResolvedItems.size();
    }

    virtual void Process()
    {
      const unsigned totalItems = ResolvedItems.size();
      for (unsigned curItemNum = 0; !ResolvedItems.empty() && !Callback.IsCanceled(); ++curItemNum)
      {
        const QString& curItem = ResolvedItems.takeFirst();
        if (!ProcessAsPlaylist(Provider, curItem, Callback))
        {
          OpenItem(curItem, curItemNum);
        }
        Callback.OnProgress(curItemNum * 100 / totalItems, curItemNum);
        Callback.OnReport(QString(), curItem);
      }
    }
  private:
    void OpenItem(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider->OpenModule(strPath, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    Playlist::ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };

  class DetectModuleScanner : public Playlist::ScannerSource
  {
  public:
    DetectModuleScanner(Playlist::Item::DataProvider::Ptr provider, Playlist::ScannerCallback& callback, const QStringList& items)
      : Provider(provider)
      , Callback(callback)
      , UnresolvedItems(items)
    {
    }

    virtual unsigned Resolve()
    {
      ResolveItems(Callback, UnresolvedItems, ResolvedItems);
      return ResolvedItems.size();
    }

    virtual void Process()
    {
      for (unsigned curItemNum = 0; !ResolvedItems.empty() && !Callback.IsCanceled(); ++curItemNum)
      {
        const QString& curItem = ResolvedItems.takeFirst();
        if (!ProcessAsPlaylist(Provider, curItem, Callback))
        {
          DetectSubitems(curItem, curItemNum);
        }
      }
    }
  private:
    void DetectSubitems(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider->DetectModules(strPath, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    Playlist::ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };
}

namespace Playlist
{
  ScannerSource::Ptr ScannerSource::CreateOpenFileSource(Playlist::Item::DataProvider::Ptr provider, ScannerCallback& callback,
    const QStringList& items)
  {
    return ScannerSource::Ptr(new OpenFileScanner(provider, callback, items));
  }

  ScannerSource::Ptr ScannerSource::CreateDetectFileSource(Playlist::Item::DataProvider::Ptr provider, ScannerCallback& callback,
    const QStringList& items)
  {
    return ScannerSource::Ptr(new DetectModuleScanner(provider, callback, items));
  }
}
