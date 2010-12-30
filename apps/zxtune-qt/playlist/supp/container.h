/*
Abstract:
  Playlist container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED

//local includes
#include "controller.h"
//common includes
#include <parameters.h>
//qt includes
#include <QtCore/QObject>

namespace Playlist
{
  class Container : public QObject
  {
    Q_OBJECT
  protected:
    explicit Container(QObject& parent);
  public:
    typedef boost::shared_ptr<Container> Ptr;

    //creator
    static Ptr Create(QObject& parent, Parameters::Accessor::Ptr parameters);

    virtual Controller::Ptr CreatePlaylist(const QString& name) = 0;
    virtual Controller::Ptr OpenPlaylist(const QString& filename) = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED
