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
#include <error.h>
#include <logging.h>
//qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>

namespace
{
  void ResolveItems(Playlist::ScannerCallback& callback, QStringList& unresolved, QStringList& resolved)
  {
    while (!callback.IsCanceled() && !unresolved.empty())
    {
      const QString& curItem = unresolved.takeFirst();
      if (QFileInfo(curItem).isDir())
      {
        for (QDirIterator iterator(curItem, QDir::Files, QDirIterator::Subdirectories); !callback.IsCanceled() && iterator.hasNext(); )
        {
          resolved.append(iterator.next());
        }
      }
      else
      {
        resolved.append(curItem);
      }
    }
  }

  bool ProcessAsPlaylist(PlayitemsProvider::Ptr provider, const QString& path, Playlist::ScannerCallback& callback)
  {
    const Playlist::IO::Container::Ptr playlist = Playlist::IO::Open(provider, path);
    if (!playlist.get())
    {
      return false;
    }
    const Playitem::Iterator::Ptr iter = playlist->GetItems();
    for (; !callback.IsCanceled() && iter->IsValid(); iter->Next())
    {
      const Playitem::Ptr item = iter->Get();
      callback.OnPlayitem(item);
    }
    return true;
  }

  class DetectParametersWrapper : public PlayitemDetectParameters
  {
  public:
    DetectParametersWrapper(Playlist::ScannerCallback& callback, unsigned curItemNum, const QString& curItem)
      : Callback(callback)
      , CurrentItemNum(curItemNum)
      , CurrentItem(curItem)
    {
    }

    virtual bool ProcessPlayitem(Playitem::Ptr item)
    {
      Callback.OnPlayitem(item);
      return !Callback.IsCanceled();
    }

    virtual void ShowProgress(unsigned progress)
    {
      Callback.OnProgress(progress, CurrentItemNum);
    }

    virtual void ShowMessage(const String& message)
    {
      const QString text = ToQString(message);
      Callback.OnReport(text, CurrentItem);
    }
  private:
    Playlist::ScannerCallback& Callback;
    const unsigned CurrentItemNum;
    const QString& CurrentItem;
  };

  class OpenFileScanner : public Playlist::ScannerSource
  {
  public:
    OpenFileScanner(PlayitemsProvider::Ptr provider, Playlist::ScannerCallback& callback, const QStringList& items)
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
    const PlayitemsProvider::Ptr Provider;
    Playlist::ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };

  class DetectModuleScanner : public Playlist::ScannerSource
  {
  public:
    DetectModuleScanner(PlayitemsProvider::Ptr provider, Playlist::ScannerCallback& callback, const QStringList& items)
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
    const PlayitemsProvider::Ptr Provider;
    Playlist::ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };
}

namespace Playlist
{
  ScannerSource::Ptr ScannerSource::CreateOpenFileSource(PlayitemsProvider::Ptr provider, ScannerCallback& callback, const QStringList& items)
  {
    return ScannerSource::Ptr(new OpenFileScanner(provider, callback, items));
  }

  ScannerSource::Ptr ScannerSource::CreateDetectFileSource(PlayitemsProvider::Ptr provider, ScannerCallback& callback, const QStringList& items)
  {
    return ScannerSource::Ptr(new DetectModuleScanner(provider, callback, items));
  }
}
