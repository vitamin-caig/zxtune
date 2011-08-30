/*
Abstract:
  Container data source implementations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "source.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <logging.h>

namespace
{
  class ContainerSource : public Playlist::ScannerSource
  {
  public:
    ContainerSource(Playlist::ScannerCallback& callback, Playlist::IO::Container::Ptr container)
      : Callback(callback)
      , Container(container)
      , CountHint(Container->GetItemsCount())
    {
    }

    virtual unsigned Resolve()
    {
      return CountHint;
    }

    virtual void Process()
    {
      Container->ForAllItems(Callback);
      //TODO: process progress
    }
  private:
    Playlist::ScannerCallback& Callback;
    const Playlist::IO::Container::Ptr Container;
    const unsigned CountHint;
  };
}

namespace Playlist
{
  ScannerSource::Ptr ScannerSource::CreateContainerSource(ScannerCallback& callback, IO::Container::Ptr container)
  {
    return ScannerSource::Ptr(new ContainerSource(callback, container));
  }
}
