/**
*
* @file
*
* @brief  Xsf-based files structure support. File
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "xsf_metainformation.h"
//library includes
#include <binary/container.h>
#include <binary/container_factories.h>
#include <strings/array.h>

namespace Module
{
  namespace XSF
  {
    struct File
    {
      File() = default;
      File(const File&) = delete;
      File(File&& rh)//= default
      {
        *this = std::move(rh);
      }
      File& operator = (const File&) = delete;
      
      File& operator = (File&& rh)//= default
      {
        Version = rh.Version;
        ReservedSection = std::move(rh.ReservedSection);
        PackedProgramSection = std::move(rh.PackedProgramSection);
        Meta = std::move(rh.Meta);
        Dependencies = std::move(rh.Dependencies);
        return *this;
      }
      
      void CloneData()
      {
        Clone(ReservedSection);
        Clone(PackedProgramSection);
      }
      
      uint_t Version = 0;
      Binary::Container::Ptr ReservedSection;
      Binary::Container::Ptr PackedProgramSection;
      MetaInformation::Ptr Meta;
      
      Strings::Array Dependencies;
      
    private:
      static void Clone(Binary::Container::Ptr& data)
      {
        if (data)
        {
          data = Binary::CreateContainer(data);
        }
      }
    };
  }
}
