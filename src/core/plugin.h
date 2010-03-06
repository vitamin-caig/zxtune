/**
*
* @file     core/plugin.h
* @brief    Plugins related interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_PLUGIN_H_DEFINED__
#define __CORE_PLUGIN_H_DEFINED__

#include <types.h>

#include <vector>

namespace ZXTune
{
  //! @brief Basic plugin information
  struct PluginInformation
  {
    PluginInformation() : Capabilities()
    {
    }
    //! Identification string
    String Id;
    //! Textual description
    String Description;
    //! Current version string
    String Version;
    //! Plugin capabilities @see plugin_attrs.h
    uint_t Capabilities;
  };

  //! @brief Set of plugins information
  typedef std::vector<PluginInformation> PluginInformationArray;
  
  //! @brief Currently supported plugins enumeration
  //! @param plugins Reference to result value
  void EnumeratePlugins(PluginInformationArray& plugins);
}

#endif //__CORE_PLUGIN_H_DEFINED__
