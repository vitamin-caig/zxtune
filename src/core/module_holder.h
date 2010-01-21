/*
Abstract:
  Modules holder interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_MODULE_HOLDER_H_DEFINED__
#define __CORE_MODULE_HOLDER_H_DEFINED__

#include <core/module_player.h>

namespace ZXTune
{
  //forward declarations
  struct PluginInformation;

  namespace Module
  {
    namespace Conversion
    {
      struct Parameter;
    }

    /// Module holder interface
    class Holder
    {
    public:
      typedef boost::shared_ptr<Holder> Ptr;

      virtual ~Holder() {}

      /// Retrieving player info
      virtual void GetPluginInformation(PluginInformation& info) const = 0;

      /// Retrieving information about loaded module
      virtual void GetModuleInformation(Information& info) const = 0;

      /// Building player from holder
      virtual Player::Ptr CreatePlayer() const = 0;

      /// Converting
      virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const = 0;
    };
  }
}

#endif //__CORE_MODULE_HOLDER_H_DEFINED__
