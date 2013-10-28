/**
*
* @file      binary/container_factories.h
* @brief     Binary data container factories
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_CONTAINER_FACTORIES_H_DEFINED
#define BINARY_CONTAINER_FACTORIES_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <binary/container.h>

namespace Binary
{
  //! @brief Creating data container based on raw data
  //! @invariant Source data is copied
  Container::Ptr CreateContainer(const void* data, std::size_t size);
  //! @brief Taking ownership of source data
  Container::Ptr CreateContainer(std::auto_ptr<Dump> data);
  //! @brief Sharing ownership of source data
  Container::Ptr CreateContainer(boost::shared_ptr<const Dump> data, std::size_t offset, std::size_t size);
  //! @brief Sharing ownership
  Container::Ptr CreateContainer(Data::Ptr data);
}

#endif //BINARY_CONTAINER_FACTORIES_H_DEFINED
