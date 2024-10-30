/**
 *
 * @file
 *
 * @brief  Parameters-related types definitions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>
#include <types.h>
// library includes
#include <binary/data.h>
#include <binary/view.h>

//! @brief Namespace is used to keep parameters-working related types and functions
namespace Parameters
{
  //@{
  //! @name Value types

  //! @brief Integer parameters type
  using IntType = int64_t;
  //! @brief String parameters type
  using StringType = String;
  //@}

  //@{
  //! @name Other types

  //! @brief Optional string quiotes
  const StringType::value_type STRING_QUOTE = '\'';
  //! @brief Mandatory data prefix
  const StringType::value_type DATA_PREFIX = '#';
  //@}
}  // namespace Parameters
