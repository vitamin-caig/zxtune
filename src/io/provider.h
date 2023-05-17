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
#include <iterator.h>
#include <types.h>

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
    //! Iterator type
    using Iterator = ObjectIterator<Provider::Ptr>;

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
  Provider::Iterator::Ptr EnumerateProviders();
}  // namespace IO
