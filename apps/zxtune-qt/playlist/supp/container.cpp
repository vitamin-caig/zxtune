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
#include "playlist.h"
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

  class PlaylistContainerImpl : public PlaylistContainer
  {
  public:
    PlaylistContainerImpl(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
      : PlaylistContainer(parent)
      , Provider(PlayitemsProvider::Create(ioParams, coreParams))
    {
    }

    virtual PlaylistSupport* CreatePlaylist(const QString& name)
    {
      PlaylistSupport* const playlist = PlaylistSupport::Create(*this, name, Provider);
      return playlist;
    }

    virtual PlaylistSupport* OpenPlaylist(const QString& filename)
    {
      if (PlaylistIOContainer::Ptr container = ::OpenPlaylist(Provider, filename))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString plName = GetPlaylistName(*plParams);
        const int plSize = GetPlaylistSize(*plParams);
        PlaylistSupport* const playlist = CreatePlaylist(plName);
        PlaylistScanner& scanner = playlist->GetScanner();
        scanner.AddItems(container->GetItems(), plSize);
        return playlist;
      }
      return 0;
    }
  private:
    const PlayitemsProvider::Ptr Provider;
  };
}

PlaylistContainer::PlaylistContainer(QObject& parent) : QObject(&parent)
{
}

PlaylistContainer* PlaylistContainer::Create(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
{
  return new PlaylistContainerImpl(parent, ioParams, coreParams);
}
