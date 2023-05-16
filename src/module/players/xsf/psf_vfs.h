/**
 *
 * @file
 *
 * @brief  PSF related stuff. Vfs
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/container.h>
#include <strings/map.h>
// std includes
#include <memory>

namespace Module::PSF
{
  class PsxVfs
  {
  public:
    using Ptr = std::shared_ptr<const PsxVfs>;
    using RWPtr = std::shared_ptr<PsxVfs>;

    void Add(StringView name, Binary::Container::Ptr data)
    {
      Files[Normalize(name)] = std::move(data);
    }

    Binary::Container::Ptr Find(StringView name) const
    {
      return Files.Get(Normalize(name));
    }

    static void Parse(const Binary::Container& data, PsxVfs& vfs);

  private:
    static String Normalize(StringView str);

  private:
    Strings::ValueMap<Binary::Container::Ptr> Files;
  };
}  // namespace Module::PSF
