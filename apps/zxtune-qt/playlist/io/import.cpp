/*
Abstract:
  Playlist import implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#include "import.h"

namespace Playlist
{
  namespace IO
  {
    Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename)
    {
      if (Container::Ptr res = OpenAYL(provider, filename))
      {
        return res;
      }
      else if (Container::Ptr res = OpenXSPF(provider, filename))
      {
        return res;
      }
      return Container::Ptr();
    }
  }
}
