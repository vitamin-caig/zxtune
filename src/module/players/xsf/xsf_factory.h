/**
*
* @file
*
* @brief  Xsf-based files common code
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "xsf_file.h"
//library includes
#include <module/players/factory.h>
//std includes
#include <map>

namespace Module
{
  namespace XSF
  {
    class Factory
    {
    public:
      using Ptr = std::shared_ptr<const Factory>;
      virtual ~Factory() = default;

      virtual Holder::Ptr CreateSinglefileModule(const File& file, Parameters::Container::Ptr properties) const = 0;
      virtual Holder::Ptr CreateMultifileModule(const File& file, const std::map<String, File>& additionalFiles, Parameters::Container::Ptr properties) const = 0;
    };
  
    Module::Factory::Ptr CreateFactory(XSF::Factory::Ptr delegate);
  }
}
