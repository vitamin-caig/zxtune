/**
 *
 * @file
 *
 * @brief  Data provider interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <span>

// forward declarations
class Error;

namespace IO
{
  //! @brief Provider information interface
  class Provider
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const Provider>;

    //! Virtual destructor
    virtual ~Provider() = default;

    //! Provider's identifier
    virtual String Id() const = 0;
    //! Description in any form
    virtual String Description() const = 0;
    //! Actuality status
    virtual Error Status() const = 0;
  };

  //! @brief Enumerating supported %IO providers
  std::span<const Provider::Ptr> EnumerateProviders();
}  // namespace IO
