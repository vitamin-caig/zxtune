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

namespace
{
  class PlaylistContainerImpl : public PlaylistContainer
  {
  public:
    explicit PlaylistContainerImpl(QObject* parent)
      : Provider(PlayitemsProvider::Create())
    {
      //setup self
      setParent(parent);
    }

    virtual PlaylistSupport* CreatePlaylist(const QString& name)
    {
      PlaylistSupport* const playlist = PlaylistSupport::Create(this, name, Provider);
      return playlist;
    }
  private:
    const PlayitemsProvider::Ptr Provider;
  };
}

PlaylistContainer* PlaylistContainer::Create(QObject* parent)
{
  return new PlaylistContainerImpl(parent);
}
