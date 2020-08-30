/**
 *
 * @file
 *
 * @brief Tags and attributes of xspf playlists
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist.xspf;

final class Meta {
  
  static final String ENCODING = "UTF-8";
  static final String XMLNS = "http://xspf.org/ns/0/";
  static final String APPLICATION = "http://zxtune.googlecode.com";
  static final String XSPF_VERSION = "1";

  // V2 does not escape string values
  static final int VERSION = 2;
}
