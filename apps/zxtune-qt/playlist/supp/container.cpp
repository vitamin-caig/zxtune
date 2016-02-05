/**
* 
* @file
*
* @brief Playlist container implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "controller.h"
#include "container.h"
#include "scanner.h"
#include "storage.h"
#include "playlist/io/export.h"
#include "playlist/io/import.h"
#include "ui/utils.h"
#include "ui/tools/errordialog.h"
//common includes
#include <error.h>
#include <make_ptr.h>
//library includes
#include <platform/version/api.h>
//boost includes
#include <boost/ref.hpp>
//text includes
#include "text/text.h"

namespace
{
  QString GetPlaylistName(const Parameters::Accessor& params)
  {
    Parameters::StringType name;
    params.FindValue(Playlist::ATTRIBUTE_NAME, name);
    return ToQString(name);
  }

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(const String& name, const Playlist::Item::Storage& storage)
      : Properties(Parameters::Container::Create())
      , Storage(storage)
    {
      Properties->SetValue(Playlist::ATTRIBUTE_NAME, name);
      Properties->SetValue(Playlist::ATTRIBUTE_CREATOR, Platform::Version::GetProgramVersionString());
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual unsigned GetItemsCount() const
    {
      return Storage.CountItems();
    }

    virtual Playlist::Item::Collection::Ptr GetItems() const
    {
      return Storage.GetItems();
    }
  private:
    const Parameters::Container::Ptr Properties;
    const Playlist::Item::Storage& Storage;
  };

  class SavePlaylistOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    SavePlaylistOperation(const QString& name, const QString& filename, Playlist::IO::ExportFlags flags)
      : Name(FromQString(name))
      , Filename(filename)
      , Flags(flags)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& storage, Log::ProgressCallback& cb)
    {
      const Playlist::IO::Container::Ptr container = MakePtr<ContainerImpl>(Name, storage);
      try
      {
        Playlist::IO::SaveXSPF(container, Filename, cb, Flags);
      }
      catch (const Error& e)
      {
        ShowErrorMessage(QString(), e);
      }
    }
  private:
    const String Name;
    const QString Filename;
    const Playlist::IO::ExportFlags Flags;
  };

  class LoadPlaylistOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    LoadPlaylistOperation(Playlist::Item::DataProvider::Ptr provider, const QString& filename, Playlist::Controller& ctrl)
      : Provider(provider)
      , Filename(filename)
      , Controller(ctrl)
    {
    }

    //do not track progress since view may not be created
    virtual void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& cb)
    {
      if (Playlist::IO::Container::Ptr container = Playlist::IO::Open(Provider, Filename, cb))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString name = GetPlaylistName(*plParams);
        Controller.SetName(name);
        storage.Add(container->GetItems());
      }
      else
      {
        //TODO: handle error
      }
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const QString Filename;
    Playlist::Controller& Controller;
  };

  class PlaylistContainer : public Playlist::Container
  {
  public:
    explicit PlaylistContainer(Parameters::Accessor::Ptr parameters)
      : Params(parameters)
    {
    }

    virtual Playlist::Controller::Ptr CreatePlaylist(const QString& name) const
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      const Playlist::Controller::Ptr ctrl = Playlist::Controller::Create(name, provider);
      return ctrl;
    }

    virtual void OpenPlaylist(const QString& filename)
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      const Playlist::Controller::Ptr playlist = Playlist::Controller::Create(QLatin1String(Text::PLAYLIST_LOADING_HEADER), provider);
      const Playlist::Item::StorageModifyOperation::Ptr op = MakePtr<LoadPlaylistOperation>(provider, filename, boost::ref(*playlist));
      playlist->GetModel()->PerformOperation(op);
      emit PlaylistCreated(playlist);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };
}

namespace Playlist
{
  Container::Ptr Container::Create(Parameters::Accessor::Ptr parameters)
  {
    return MakePtr<PlaylistContainer>(parameters);
  }

  void Save(Controller::Ptr ctrl, const QString& filename, uint_t flags)
  {
    const QString name = ctrl->GetName();
    const Playlist::Item::StorageAccessOperation::Ptr op = MakePtr<SavePlaylistOperation>(name, filename, flags);
    ctrl->GetModel()->PerformOperation(op);
  }
}
