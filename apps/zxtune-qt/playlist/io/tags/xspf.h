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

#include "apps/zxtune-qt/ui/utils.h"

namespace XSPF
{
  constexpr const auto SUFFIX = ".xspf"_latin;

  constexpr const auto ROOT_TAG = "playlist"_latin;

  constexpr const auto VERSION_ATTR = "version"_latin;

  constexpr const auto VERSION_VALUE = "1"_latin;

  constexpr const auto XMLNS_ATTR = "xmlns"_latin;

  constexpr const auto XMLNS_VALUE = "http://xspf.org/ns/0/"_latin;

  constexpr const auto EXTENSION_TAG = "extension"_latin;

  constexpr const auto APPLICATION_ATTR = "application"_latin;

  constexpr const auto TRACKLIST_TAG = "trackList"_latin;

  constexpr const auto ITEM_TAG = "track"_latin;

  constexpr const auto ITEM_LOCATION_TAG = "location"_latin;

  constexpr const auto ITEM_CREATOR_TAG = "creator"_latin;

  constexpr const auto ITEM_TITLE_TAG = "title"_latin;

  constexpr const auto ITEM_ANNOTATION_TAG = "annotation"_latin;

  constexpr const auto ITEM_DURATION_TAG = "duration"_latin;

  // custom tags
  constexpr const auto EXTENDED_PROPERTY_TAG = "property"_latin;

  constexpr const auto EXTENDED_PROPERTY_NAME_ATTR = "name"_latin;

  constexpr const auto EMBEDDED_PREFIX = "?"_latin;
}  // namespace XSPF
