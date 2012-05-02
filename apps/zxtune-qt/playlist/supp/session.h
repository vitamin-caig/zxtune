/*
Abstract:
  Playlists session interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_SESSION_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_SESSION_H_DEFINED

//local includes
#include "container.h"

namespace Playlist
{
  class Session
  {
  public:
    typedef boost::shared_ptr<Session> Ptr;

    virtual ~Session() {}

    virtual void Load() = 0;
    virtual void Save(Controller::Iterator::Ptr it) = 0;

    //creator
    static Ptr Create(Container::Ptr container);
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_SESSION_H_DEFINED
