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
#include "playlist/io/import.h"
#include "ui/utils.h"

namespace
{
  QString GetPlaylistName(const Parameters::Accessor& params)
  {
    Parameters::StringType name;
    if (params.FindStringValue(Playlist::ATTRIBUTE_NAME, name))
    {
      return ToQString(name);
    }
    assert(!"No playlist name");
    return QString::fromUtf8("NoName");
  }

  int GetPlaylistSize(const Parameters::Accessor& params)
  {
    Parameters::IntType size = 0;
    if (params.FindIntValue(Playlist::ATTRIBUTE_SIZE, size))
    {
      return static_cast<int>(size);
    }
    return -1;
  }

  class ContainerImpl : public Playlist::Container
  {
  public:
    ContainerImpl(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
      : Playlist::Container(parent)
      , Provider(PlayitemsProvider::Create(ioParams, coreParams))
    {
    }

    virtual Playlist::Controller* CreatePlaylist(const QString& name)
    {
      Playlist::Controller* const playlist = Playlist::Controller::Create(*this, name, Provider);
      return playlist;
    }

    virtual Playlist::Controller* OpenPlaylist(const QString& filename)
    {
      if (Playlist::IO::Container::Ptr container = Playlist::IO::Open(Provider, filename))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString plName = GetPlaylistName(*plParams);
        const int plSize = GetPlaylistSize(*plParams);
        Playlist::Controller* const playlist = CreatePlaylist(plName);
        Playlist::Scanner& scanner = playlist->GetScanner();
        scanner.AddItems(container->GetItems(), plSize);
        return playlist;
      }
      return 0;
    }
  private:
    const PlayitemsProvider::Ptr Provider;
  };
}

namespace Playlist
{
  Container::Container(QObject& parent) : QObject(&parent)
  {
  }

  Container* Container::Create(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
  {
    return new ContainerImpl(parent, ioParams, coreParams);
  }
}
