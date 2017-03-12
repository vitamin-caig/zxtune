/**
*
* @file
*
* @brief  Binary data container factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <binary/container.h>

namespace Binary
{
  //! @brief Creating data container based on raw data
  //! @invariant Source data is copied
  Container::Ptr CreateContainer(const void* data, std::size_t size);
  //! @invariant Source data is not copied. Dangerous!!!
  Container::Ptr CreateNonCopyContainer(const void* data, std::size_t size);
  //! @brief Taking ownership of source data
  Container::Ptr CreateContainer(std::unique_ptr<Dump> data);
  //! @brief Sharing ownership of source data
  Container::Ptr CreateContainer(std::shared_ptr<const Dump> data, std::size_t offset, std::size_t size);
  //! @brief Sharing ownership
  Container::Ptr CreateContainer(Data::Ptr data);
}
