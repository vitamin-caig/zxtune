/**
*
* @file     core/module_holder.h
* @brief    Modules holder interface definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_HOLDER_H_DEFINED__
#define __CORE_MODULE_HOLDER_H_DEFINED__

//library includes
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

    //! @brief %Module holder interface
    class Holder
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<Holder> Ptr;
      //! @brief Read-only pointer type
      typedef boost::shared_ptr<const Holder> ConstPtr;

      virtual ~Holder() {}

      //! @brief Retrieving plugin info
      //! @param info Reference to returned value
      virtual void GetPluginInformation(PluginInformation& info) const = 0;

      //! @brief Retrieving info about loaded module
      //! @param info Reference to returned value
      virtual void GetModuleInformation(Information& info) const = 0;

      //! @brief Modifying attributes set
      //! @param attrs Attributes to add/modify
      //! @param replaceExisting Replace existing attributes
      virtual void ModifyCustomAttributes(const Parameters::Map& attrs, bool replaceExisting) = 0;
      
      //! @brief Creating new player instance
      //! @return New player
      virtual Player::Ptr CreatePlayer() const = 0;

      //! @brief Converting to specified format
      //! @param param Specify format to convert
      //! @param dst Result data
      //! @return Error() in case of success
      virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const = 0;
    };
  }
}

#endif //__CORE_MODULE_HOLDER_H_DEFINED__
