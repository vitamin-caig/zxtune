/*
Abstract:
  Common container plugins support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__
#define __CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__

//local includes
#include "container_catalogue.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/archived_decoders.h>

namespace ZXTune
{
  class ContainerFactory
  {
  public:
    typedef boost::shared_ptr<const ContainerFactory> Ptr;
    virtual ~ContainerFactory() {}

    virtual Binary::Format::Ptr GetFormat() const = 0;

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, Binary::Container::Ptr data) const = 0;
  };

  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, const String& info, uint_t caps,
    ContainerFactory::Ptr factory);

  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder);
}

#endif //__CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__
