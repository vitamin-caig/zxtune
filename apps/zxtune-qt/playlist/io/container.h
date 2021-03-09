/**
 *
 * @file
 *
 * @brief  Playlist container interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "playlist/supp/data.h"
// library includes
#include <parameters/accessor.h>

namespace Playlist
{
  const Parameters::NameType ATTRIBUTES_PREFIX("zxtune.app.playlist");

  const Parameters::NameType ATTRIBUTE_NAME = ATTRIBUTES_PREFIX + "name";
  const Parameters::NameType ATTRIBUTE_VERSION = ATTRIBUTES_PREFIX + "version";
  const Parameters::NameType ATTRIBUTE_CREATOR = ATTRIBUTES_PREFIX + "creator";
  const Parameters::NameType ATTRIBUTE_ITEMS = ATTRIBUTES_PREFIX + "items";

  namespace IO
  {
    class Container
    {
    public:
      typedef std::shared_ptr<const Container> Ptr;

      virtual ~Container() = default;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual unsigned GetItemsCount() const = 0;
      virtual Item::Collection::Ptr GetItems() const = 0;
    };
  }  // namespace IO
}  // namespace Playlist
