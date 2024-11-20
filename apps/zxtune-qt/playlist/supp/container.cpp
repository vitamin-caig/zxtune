/**
 *
 * @file
 *
 * @brief Playlist container implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/supp/container.h"

#include "apps/zxtune-qt/playlist/io/container.h"
#include "apps/zxtune-qt/playlist/io/export.h"
#include "apps/zxtune-qt/playlist/io/import.h"
#include "apps/zxtune-qt/playlist/supp/controller.h"
#include "apps/zxtune-qt/playlist/supp/data.h"
#include "apps/zxtune-qt/playlist/supp/data_provider.h"
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/storage.h"
#include "apps/zxtune-qt/ui/tools/errordialog.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "parameters/container.h"
#include "parameters/identifier.h"
#include "platform/version/api.h"
#include "tools/iterators.h"

#include "error.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <utility>

namespace Log
{
  class ProgressCallback;
}  // namespace Log

namespace
{
  QString GetPlaylistName(const Parameters::Accessor& params)
  {
    return ToQString(Parameters::GetString(params, Playlist::ATTRIBUTE_NAME));
  }

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(StringView name, const Playlist::Item::Storage& storage)
      : Properties(Parameters::Container::Create())
      , Storage(storage)
    {
      Properties->SetValue(Playlist::ATTRIBUTE_NAME, name);
      Properties->SetValue(Playlist::ATTRIBUTE_CREATOR, Platform::Version::GetProgramVersionString());
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    unsigned GetItemsCount() const override
    {
      return Storage.CountItems();
    }

    Playlist::Item::Collection::Ptr GetItems() const override
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
    SavePlaylistOperation(const QString& name, QString filename, Playlist::IO::ExportFlags flags)
      : Name(FromQString(name))
      , Filename(std::move(filename))
      , Flags(flags)
    {}

    void Execute(const Playlist::Item::Storage& storage, Log::ProgressCallback& cb) override
    {
      const Playlist::IO::Container::Ptr container = MakePtr<ContainerImpl>(Name, storage);
      try
      {
        Playlist::IO::SaveXSPF(*container, Filename, cb, Flags);
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
    LoadPlaylistOperation(Playlist::Item::DataProvider::Ptr provider, QString filename, Playlist::Controller& ctrl)
      : Provider(std::move(provider))
      , Filename(std::move(filename))
      , Controller(ctrl)
    {}

    // do not track progress since view may not be created
    void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& cb) override
    {
      if (const auto container = Playlist::IO::Open(Provider, Filename, cb))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString name = GetPlaylistName(*plParams);
        Controller.SetName(name);
        storage.Add(container->GetItems());
      }
      else
      {
        // TODO: handle error
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
      : Params(std::move(parameters))
    {}

    Playlist::Controller::Ptr CreatePlaylist(const QString& name) const override
    {
      auto provider = Playlist::Item::DataProvider::Create(Params);
      return Playlist::Controller::Create(name, std::move(provider));
    }

    void OpenPlaylist(const QString& filename) override
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      const Playlist::Controller::Ptr playlist = Playlist::Controller::Create(QLatin1String("..."), provider);
      const Playlist::Item::StorageModifyOperation::Ptr op =
          MakePtr<LoadPlaylistOperation>(provider, filename, *playlist);
      playlist->GetModel()->PerformOperation(op);
      emit PlaylistCreated(playlist);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };
}  // namespace

namespace Playlist
{
  Container::Ptr Container::Create(Parameters::Accessor::Ptr parameters)
  {
    return MakePtr<PlaylistContainer>(std::move(parameters));
  }

  void Save(Controller& ctrl, const QString& filename, uint_t flags)
  {
    const QString name = ctrl.GetName();
    auto op = MakePtr<SavePlaylistOperation>(name, filename, flags);
    ctrl.GetModel()->PerformOperation(std::move(op));
  }
}  // namespace Playlist
