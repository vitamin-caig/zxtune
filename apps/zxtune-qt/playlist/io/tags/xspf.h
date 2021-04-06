/**
 *
 * @file
 *
 * @brief AYL format tags
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace XSPF
{
  constexpr const auto SUFFIX = ".xspf";

  constexpr const auto ROOT_TAG = "playlist";

  constexpr const auto VERSION_ATTR = "version";

  constexpr const auto VERSION_VALUE = "1";

  constexpr const auto XMLNS_ATTR = "xmlns";

  constexpr const auto XMLNS_VALUE = "http://xspf.org/ns/0/";

  constexpr const auto EXTENSION_TAG = "extension";

  constexpr const auto APPLICATION_ATTR = "application";

  constexpr const auto TRACKLIST_TAG = "trackList";

  constexpr const auto ITEM_TAG = "track";

  constexpr const auto ITEM_LOCATION_TAG = "location";

  constexpr const auto ITEM_CREATOR_TAG = "creator";

  constexpr const auto ITEM_TITLE_TAG = "title";

  constexpr const auto ITEM_ANNOTATION_TAG = "annotation";

  constexpr const auto ITEM_DURATION_TAG = "duration";

  // custom tags
  constexpr const auto EXTENDED_PROPERTY_TAG = "property";

  constexpr const auto EXTENDED_PROPERTY_NAME_ATTR = "name";

  constexpr const auto EMBEDDED_PREFIX = "?";
}  // namespace XSPF
