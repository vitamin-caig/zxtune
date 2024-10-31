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

#include "module/players/xsf/xsf_metainformation.h"

#include <binary/container.h>
#include <strings/array.h>

namespace Module::XSF
{
  struct File
  {
    File() = default;
    File(const File&) = delete;
    File(File&& rh) noexcept  //= default
    {
      *this = std::move(rh);
    }
    File& operator=(const File&) = delete;

    File& operator=(File&& rh) noexcept  //= default
    {
      Version = rh.Version;
      ReservedSection = std::move(rh.ReservedSection);
      PackedProgramSection = std::move(rh.PackedProgramSection);
      Meta = std::move(rh.Meta);
      Dependencies = std::move(rh.Dependencies);
      return *this;
    }

    uint_t Version = 0;
    Binary::Container::Ptr ReservedSection;
    Binary::Container::Ptr PackedProgramSection;
    MetaInformation::Ptr Meta;

    Strings::Array Dependencies;
  };
}  // namespace Module::XSF
