/*
Abstract:
  Playlists session implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "container.h"
#include "session.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
#include <debug_log.h>
//std includes
#include <algorithm>
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtGui/QDesktopServices>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("Playlist::Session");

  QStringList Substract(const QStringList& lh, const QStringList& rh)
  {
    QStringList result(lh);
    std::for_each(rh.begin(), rh.end(), boost::bind(&QStringList::removeAll, &result, _1));
    return result;
  }

  class TasksSet
  {
  public:
    void Add(Playlist::Controller::Ptr ctrl)
    {
      Models.push_back(ctrl->GetModel());
    }

    void Wait()
    {
      while (!Models.empty())
      {
        Models.front()->WaitOperationFinish();
        Models.pop_front();
      }
    }
  private:
    std::list<Playlist::Model::Ptr> Models;
  };

  template<class T>
  QString BuildPlaylistFileName(const T& val)
  {
    return QString::fromAscii("%1.xspf").arg(val);
  }

  class FiledSession : public Playlist::Session
  {
  public:
    FiledSession()
      : Directory(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
    {
      const QLatin1String dirPath(Text::PLAYLISTS_DIR);
      Require(Directory.mkpath(dirPath));
      Require(Directory.cd(dirPath));
      Files = Directory.entryList(QStringList(BuildPlaylistFileName('*')), QDir::Files | QDir::Readable, QDir::Name);
      Dbg("%1% stored playlists", Files.size());
    }

    virtual void Load(Playlist::Container::Ptr container)
    {
      for (QStringList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        const QString& fileName = *it;
        const QString& fullPath = Directory.absoluteFilePath(fileName);
        Dbg("Loading stored playlist '%1%'", FromQString(fullPath));
        container->OpenPlaylist(fullPath);
      }
    }

    virtual void Save(Playlist::Controller::Iterator::Ptr it)
    {
      const QStringList& newFiles = SaveFiles(it);
      Dbg("Saved %1% playlists", newFiles.size());
      const QStringList& toRemove = Substract(Files, newFiles);
      RemoveFiles(toRemove);
      Files = newFiles;
    }
  private:
    QStringList SaveFiles(Playlist::Controller::Iterator::Ptr it)
    {
      TasksSet tasks;
      QStringList newFiles;
      for (int idx = 0; it->IsValid(); it->Next(), ++idx)
      {
        const Playlist::Controller::Ptr ctrl = it->Get();
        const QString& fileName = BuildPlaylistFileName(idx);
        const QString& fullPath = Directory.absoluteFilePath(fileName);
        Playlist::Save(ctrl, fullPath, 0);
        tasks.Add(ctrl);
        newFiles.push_back(fileName);
      }
      tasks.Wait();
      return newFiles;
    }

    void RemoveFiles(const QStringList& files)
    {
      std::for_each(files.begin(), files.end(), boost::bind(&QDir::remove, &Directory, _1));
    }
  private:
    QDir Directory;
    QStringList Files;
  };
}

namespace Playlist
{
  Session::Ptr Session::Create()
  {
    return boost::make_shared<FiledSession>();
  }
}
