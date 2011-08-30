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
      , Params(parameters)
    {
    }

    virtual Playlist::Controller::Ptr CreatePlaylist(const QString& name)
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      return Playlist::Controller::Create(*this, name, provider);
    }

    virtual Playlist::Controller::Ptr OpenPlaylist(const QString& filename)
    {
      const Playlist::Item::DataProvider::Ptr provider = Playlist::Item::DataProvider::Create(Params);
      if (Playlist::IO::Container::Ptr container = Playlist::IO::Open(provider, filename))
      {
        const Parameters::Accessor::Ptr plParams = container->GetProperties();
        const QString name = GetPlaylistName(*plParams);
        const Playlist::Controller::Ptr playlist = Playlist::Controller::Create(*this, name, provider);
        const Playlist::Scanner::Ptr scanner = playlist->GetScanner();
        scanner->AddItems(container);
        return playlist;
      }
      return Playlist::Controller::Ptr();
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
    return boost::make_shared<ContainerImpl>(boost::ref(parent), parameters);
  }
}
