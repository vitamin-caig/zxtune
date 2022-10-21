/**
 *
 * @file
 *
 * @brief Sessions support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "session.h"
#include "container.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
// std includes
#include <algorithm>
#include <list>
// qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>

namespace
{
  const Debug::Stream Dbg("Playlist::Session");

  QStringList Substract(const QStringList& lh, const QStringList& rh)
  {
    QStringList result(lh);
    std::for_each(rh.begin(), rh.end(), [&result](const QString& s) { result.removeAll(s); });
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
    return QString::fromLatin1("%1.xspf").arg(val);
  }

  class FiledSession : public Playlist::Session
  {
  public:
    FiledSession()
      : Directory(QStandardPaths::locate(QStandardPaths::DataLocation, "", QStandardPaths::LocateDirectory))
    {
      const auto dirPath = QCoreApplication::applicationName() + "/Playlists";
      Require(Directory.mkpath(dirPath));
      Require(Directory.cd(dirPath));
      Files = Directory.entryList(QStringList(BuildPlaylistFileName('*')), QDir::Files | QDir::Readable, QDir::Name);
      Dbg("{} stored playlists", Files.size());
    }

    bool Empty() const override
    {
      return Files.empty();
    }

    void Load(Playlist::Container::Ptr container) override
    {
      for (QStringList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        const QString& fileName = *it;
        const QString& fullPath = Directory.absoluteFilePath(fileName);
        Dbg("Loading stored playlist '{}'", FromQString(fullPath));
        container->OpenPlaylist(fullPath);
      }
    }

    void Save(Playlist::Controller::Iterator::Ptr it) override
    {
      const QStringList& newFiles = SaveFiles(it);
      Dbg("Saved {} playlists", newFiles.size());
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
      for (const auto& name : files)
      {
        Directory.remove(name);
      }
    }

  private:
    QDir Directory;
    QStringList Files;
  };
}  // namespace

namespace Playlist
{
  Session::Ptr Session::Create()
  {
    return MakePtr<FiledSession>();
  }
}  // namespace Playlist
