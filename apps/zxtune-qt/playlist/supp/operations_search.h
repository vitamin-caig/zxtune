/**
 *
 * @file
 *
 * @brief Search operation factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "operations.h"

namespace Playlist::Item
{
  namespace Search
  {
    enum
    {
      TITLE = 1,
      AUTHOR = 2,
      PATH = 4,

      ALL = ~0
    };

    enum
    {
      CASE_SENSITIVE = 1,
      REGULAR_EXPRESSION = 2,
    };

    struct Data
    {
      QString Pattern;
      uint_t Scope;
      uint_t Options;

      Data()
        : Scope(ALL)
        , Options()
      {}
    };
  }  // namespace Search

  SelectionOperation::Ptr CreateSearchOperation(const Search::Data& data);
  SelectionOperation::Ptr CreateSearchOperation(Playlist::Model::IndexSet::Ptr items, const Search::Data& data);
}  // namespace Playlist::Item
