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
    PlaylistContainerImpl(QObject* parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
      : Provider(PlayitemsProvider::Create(ioParams, coreParams))
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

PlaylistContainer* PlaylistContainer::Create(QObject* parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
{
  return new PlaylistContainerImpl(parent, ioParams, coreParams);
}
