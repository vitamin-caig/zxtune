/**
 *
 * @file
 *
 * @brief  Shared library interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <string_type.h>
#include <string_view.h>

#include <memory>
#include <vector>

namespace Platform
{
  class SharedLibrary
  {
  public:
    using Ptr = std::shared_ptr<const SharedLibrary>;
    virtual ~SharedLibrary() = default;

    virtual void* GetSymbol(const String& name) const = 0;

    //! @param name Library name without platform-dependent prefixes and extension
    // E.g. Load("SDL") will try to load "libSDL.so" for Linux and and "SDL.dll" for Windows
    // If platform-dependent full filename is specified, no substitution is made
    static Ptr Load(StringView name);

    class Name
    {
    public:
      virtual ~Name() = default;

      virtual StringView Base() const = 0;
      virtual std::vector<StringView> PosixAlternatives() const = 0;
      virtual std::vector<StringView> WindowsAlternatives() const = 0;
    };

    static Ptr Load(const Name& name);
  };
}  // namespace Platform
