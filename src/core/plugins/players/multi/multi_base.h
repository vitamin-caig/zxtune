/**
* 
* @file
*
* @brief  multidevice-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_holder.h>
//std includes
#include <vector>

namespace Module
{
  namespace Multi
  {
    typedef std::vector<Module::Holder::Ptr> HoldersArray;
    
    //first holder is used as a main one
    Module::Holder::Ptr CreateHolder(Parameters::Accessor::Ptr params, const HoldersArray& holders);
  }
}
