/**
*
* @file     core/plugin.h
* @brief    Plugins related interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_PLUGIN_H_DEFINED__
#define __CORE_PLUGIN_H_DEFINED__

//common includes
#include <iterator.h>
#include <types.h>
//std includes
#include <memory>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  //! @brief Basic plugin interface
  class Plugin
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Plugin> Ptr;
    //! Iterator type
    typedef ObjectIterator<Plugin::Ptr> Iterator;

    //! Virtual destructor
    virtual ~Plugin() {}

    //! Identification string
    virtual String Id() const = 0;
    //! Textual description
    virtual String Description() const = 0;
    //! Current version string
    virtual String Version() const = 0;
    //! Plugin capabilities @see plugin_attrs.h
    virtual uint_t Capabilities() const = 0;
  };

  Plugin::Iterator::Ptr EnumeratePlugins();
}

#endif //__CORE_PLUGIN_H_DEFINED__
