/*
Abstract:
  Find operations factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_FIND_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_FIND_H_DEFINED

//local includes
#include "operations.h"

namespace Playlist
{
  namespace Item
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
        {
        }
      };
    }

    SelectionOperation::Ptr CreateSearchOperation(QObject& parent, const Search::Data& data);
    SelectionOperation::Ptr CreateSearchOperation(QObject& parent, Playlist::Model::IndexSetPtr items, const Search::Data& data);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_FIND_H_DEFINED
