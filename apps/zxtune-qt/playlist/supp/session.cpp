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
  const std::string THIS_MODULE("Playlist::Session");

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

  class FiledSession : public Playlist::Session
  {
  public:
    explicit FiledSession(const Playlist::Container::Ptr ctr)
      : Container(ctr)
      , Directory(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
    {
      const QString dirPath(Text::PLAYLISTS_DIR);
      Require(Directory.mkpath(dirPath));
      Require(Directory.cd(dirPath));
      Files = Directory.entryList(QStringList("*.xspf"), QDir::Files | QDir::Readable, QDir::Name);
      Log::Debug(THIS_MODULE, "%1% stored playlists", Files.size());
    }

    virtual void Load()
    {
      for (QStringList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        const QString& fileName = *it;
        const QString& fullPath = Directory.absoluteFilePath(fileName);
        Log::Debug(THIS_MODULE, "Loading stored playlist '%1%'", FromQString(fullPath));
        Container->OpenPlaylist(fullPath);
      }
    }

    virtual void Save(Playlist::Controller::Iterator::Ptr it)
    {
      const QStringList& newFiles = SaveFiles(it);
      Log::Debug(THIS_MODULE, "Saved %1% playlists", newFiles.size());
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
        const QString& fileName = QString("%1.xspf").arg(idx);
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
    const Playlist::Container::Ptr Container;
    QDir Directory;
    QStringList Files;
  };
}

namespace Playlist
{
  Session::Ptr Session::Create(Container::Ptr ctr)
  {
    return boost::make_shared<FiledSession>(ctr);
  }
}
