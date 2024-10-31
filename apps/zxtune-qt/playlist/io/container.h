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
#include "apps/zxtune-qt/playlist/supp/data.h"
// library includes
#include <parameters/accessor.h>
#include <parameters/identifier.h>
// qt includes
#include <QtCore/QLatin1String>

namespace Playlist
{
  using Parameters::operator""_id;

  const auto ATTRIBUTES_PREFIX = "zxtune.app.playlist"_id;
  // Should not be changed!!!
  const QLatin1String APPLICATION_ID("http://zxtune.googlecode.com");

  const auto ATTRIBUTE_NAME = ATTRIBUTES_PREFIX + "name"_id;
  const auto ATTRIBUTE_VERSION = ATTRIBUTES_PREFIX + "version"_id;
  const auto ATTRIBUTE_CREATOR = ATTRIBUTES_PREFIX + "creator"_id;
  const auto ATTRIBUTE_ITEMS = ATTRIBUTES_PREFIX + "items"_id;

  namespace IO
  {
    class Container
    {
    public:
      using Ptr = std::shared_ptr<const Container>;

      virtual ~Container() = default;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual unsigned GetItemsCount() const = 0;
      virtual Item::Collection::Ptr GetItems() const = 0;
    };
  }  // namespace IO
}  // namespace Playlist
