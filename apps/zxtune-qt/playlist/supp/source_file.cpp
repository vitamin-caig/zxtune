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
//common includes
#include <error.h>
#include <logging.h>
//qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>

namespace
{
  void ResolveItems(ScannerCallback& callback, QStringList& unresolved, QStringList& resolved)
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

  class DetectParametersWrapper : public PlayitemDetectParameters
  {
  public:
    DetectParametersWrapper(ScannerCallback& callback, unsigned curItemNum, const QString& curItem)
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

    virtual void ShowProgress(const Log::MessageData& msg)
    {
      if (0 != msg.Level)
      {
        return;
      }
      if (msg.Text)
      {
        const QString text = ToQString(*msg.Text);
        Callback.OnReport(text, CurrentItem);
      }
      if (msg.Progress)
      {
        const uint_t curProgress = *msg.Progress;
        Callback.OnProgress(curProgress, CurrentItemNum);
      }
    }
  private:
    ScannerCallback& Callback;
    const unsigned CurrentItemNum;
    const QString& CurrentItem;
  };

  class OpenFileScanner : public ScannerSource
  {
  public:
    OpenFileScanner(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
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
        OpenItem(curItem, curItemNum);
        Callback.OnProgress(curItemNum * 100 / totalItems, curItemNum);
        Callback.OnReport(QString(), curItem);
      }
    }
  private:
    void OpenItem(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider.OpenModule(strPath, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    PlayitemsProvider& Provider;
    ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };

  class DetectModuleScanner : public ScannerSource
  {
  public:
    DetectModuleScanner(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
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
        DetectSubitems(curItem, curItemNum);
      }
    }
  private:
    void DetectSubitems(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider.DetectModules(strPath, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    PlayitemsProvider& Provider;
    ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };
}

ScannerSource::Ptr ScannerSource::CreateOpenFileSource(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
{
  return ScannerSource::Ptr(new OpenFileScanner(provider, callback, items));
}

ScannerSource::Ptr ScannerSource::CreateDetectFileSource(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
{
  return ScannerSource::Ptr(new DetectModuleScanner(provider, callback, items));
}
