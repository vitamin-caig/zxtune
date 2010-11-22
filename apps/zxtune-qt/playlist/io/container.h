/*
Abstract:
  Playlist container interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED

//local includes
#include "playlist/supp/data.h"
//common includes
#include <parameters.h>

namespace Playlist
{
  const Char ATTRIBUTE_NAME[] = 
  {
    'z','x','t','u','n','e','.','a','p','p','.','p','l','a','y','l','i','s','t','.','n', 'a', 'm', 'e', 0
  };
  const Char ATTRIBUTE_SIZE[] =
  {
    'z','x','t','u','n','e','.','a','p','p','.','p','l','a','y','l','i','s','t','.','s', 'i', 'z', 'e', 0
  };

  namespace IO
  {
    class Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      virtual ~Container() {}

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual Item::Data::Iterator::Ptr GetItems() const = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
