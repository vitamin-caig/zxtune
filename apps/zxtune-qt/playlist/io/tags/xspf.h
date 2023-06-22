/**
 *
 * @file
 *
 * @brief XSPF format tags
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// qt includes
#include <QtCore/QLatin1StringView>

namespace XSPF
{
  using namespace Qt::Literals::StringLiterals;

  constexpr const auto SUFFIX = ".xspf";

  constexpr const auto ROOT_TAG = "playlist"_L1;

  constexpr const auto VERSION_ATTR = "version"_L1;

  constexpr const auto VERSION_VALUE = "1";

  constexpr const auto XMLNS_ATTR = "xmlns"_L1;

  constexpr const auto XMLNS_VALUE = "http://xspf.org/ns/0/";

  constexpr const auto EXTENSION_TAG = "extension"_L1;

  constexpr const auto APPLICATION_ATTR = "application"_L1;

  constexpr const auto TRACKLIST_TAG = "trackList"_L1;

  constexpr const auto ITEM_TAG = "track"_L1;

  constexpr const auto ITEM_LOCATION_TAG = "location"_L1;

  constexpr const auto ITEM_CREATOR_TAG = "creator"_L1;

  constexpr const auto ITEM_TITLE_TAG = "title"_L1;

  constexpr const auto ITEM_ANNOTATION_TAG = "annotation"_L1;

  constexpr const auto ITEM_DURATION_TAG = "duration"_L1;

  // custom tags
  constexpr const auto EXTENDED_PROPERTY_TAG = "property"_L1;

  constexpr const auto EXTENDED_PROPERTY_NAME_ATTR = "name"_L1;

  constexpr const auto EMBEDDED_PREFIX = "?";
}  // namespace XSPF
