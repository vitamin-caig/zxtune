/*
Abstract:
  Playlist container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "controller.h"
#include "container.h"
#include "scanner.h"
#include "storage.h"
#include "playlist/io/export.h"
#include "playlist/io/import.h"
#include "ui/utils.h"
#include "apps/version/api.h"
//common includes
#include <error.h>
//boost includes
#include <boost/make_shared.hpp>
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
      Properties->SetValue(Playlist::ATTRIBUTE_CREATOR, GetProgramVersionString());
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual unsigned GetItemsCount() const
    {
      return Storage.CountItems();
    }

    virtual void ForAllItems(Playlist::Item::Callback& callback) const
    {
      ItemCallbackAdapter adapter(callback);
      Storage.ForAllItems(adapter);
    }
  private:
    class ItemCallbackAdapter : public Playlist::Item::Visitor
    {
    public:
      explicit ItemCallbackAdapter(Playlist::Item::Callback& delegate)
        : Delegate(delegate)
      {
      }

      virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
      {
        return Delegate.OnItem(data);
      }
    private:
      Playlist::Item::Callback& Delegate;
    };
  private:
    const Parameters::Container::Ptr Properties;
    const Playlist::Item::Storage& Storage;
  };

  class CallbackWrapper : public Playlist::IO::ExportCallback
  {
  public:
    explicit CallbackWrapper(Log::ProgressCallback& cb)
      : Delegate(cb)
    {
    }

    virtual void Progress(unsigned current)
    {
      Delegate.OnProgress(current);
    }

    virtual bool IsCanceled() const
    {
      return false;
    }
  private:
    Log::ProgressCallback& Delegate;
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
      const Playlist::IO::Container::Ptr container = boost::make_shared<ContainerImpl>(Name, storage);
      CallbackWrapper callback(cb);
      if (const Error& err = Playlist::IO::SaveXSPF(container, Filename, callback, Flags))
      {
        //TODO: handle error
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
    virtual void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& /*cb*/)
    {
      if (Playlist::IO::Container::Ptr container = Playlist::IO::Open(Provider, Filename))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString name = GetPlaylistName(*plParams);
        Controller.SetName(name);
        CallbackWrapper callback(storage);
        container->ForAllItems(callback);
      }
      else
      {
        //TODO: handle error
      }
    }
  private:
    class CallbackWrapper : public Playlist::Item::Callback
    {
    public:
      explicit CallbackWrapper(Playlist::Item::Storage& stor)
        : Storage(stor)
      {
      }

      virtual void OnItem(Playlist::Item::Data::Ptr data)
      {
        Storage.AddItem(data);
      }
    private:
      Playlist::Item::Storage& Storage;
    };
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const QString Filename;
    Playlist::Controller& Controller;
  };

  class PlaylistContainer : public Playlist::Container
  {
  public:
    PlaylistContainer(QObject& parent, Parameters::Accessor::Ptr parameters)
      : Playlist::Container(parent)
      , Params(parameters)
    {
    }

    virtual Playlist::Controller::Ptr CreatePlaylist(const QString& name)
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      const Playlist::Controller::Ptr ctrl = Playlist::Controller::Create(*this, name, provider);
      return ctrl;
    }

    virtual void OpenPlaylist(const QString& filename)
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      const Playlist::Controller::Ptr playlist = Playlist::Controller::Create(*this, QLatin1String(Text::PLAYLIST_LOADING_HEADER), provider);
      const Playlist::Item::StorageModifyOperation::Ptr op = boost::make_shared<LoadPlaylistOperation>(provider, filename, boost::ref(*playlist));
      playlist->GetModel()->PerformOperation(op);
      emit PlaylistCreated(playlist);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };
}

namespace Playlist
{
  Container::Container(QObject& parent) : QObject(&parent)
  {
  }

  Container::Ptr Container::Create(QObject& parent, Parameters::Accessor::Ptr parameters)
  {
    return boost::make_shared<PlaylistContainer>(boost::ref(parent), parameters);
  }

  void Save(Controller::Ptr ctrl, const QString& filename, uint_t flags)
  {
    const QString name = ctrl->GetName();
    const Playlist::Item::StorageAccessOperation::Ptr op = boost::make_shared<SavePlaylistOperation>(name, filename, flags);
    ctrl->GetModel()->PerformOperation(op);
  }
}
