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
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

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

  class ContainerImpl : public Playlist::Container
  {
  public:
    ContainerImpl(QObject& parent, Parameters::Accessor::Ptr parameters)
      : Playlist::Container(parent)
      , Provider(Playlist::Item::DataProvider::Create(parameters))
    {
    }

    virtual Playlist::Controller::Ptr CreatePlaylist(const QString& name)
    {
      return Playlist::Controller::Create(*this, name, Provider);
    }

    virtual Playlist::Controller::Ptr OpenPlaylist(const QString& filename)
    {
      if (Playlist::IO::Container::Ptr container = Playlist::IO::Open(Provider, filename))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString plName = GetPlaylistName(*plParams);
        const unsigned plSize = container->GetItemsCount();
        const Playlist::Controller::Ptr playlist = CreatePlaylist(plName);
        const Playlist::Scanner::Ptr scanner = playlist->GetScanner();
        scanner->AddItems(container->GetItems(), plSize);
        return playlist;
      }
      return Playlist::Controller::Ptr();
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
  };
}

namespace Playlist
{
  Container::Container(QObject& parent) : QObject(&parent)
  {
  }

  Container::Ptr Container::Create(QObject& parent, Parameters::Accessor::Ptr parameters)
  {
    return boost::make_shared<ContainerImpl>(boost::ref(parent), parameters);
  }
}
