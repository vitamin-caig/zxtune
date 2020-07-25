/**
 *
 * @file
 *
 * @brief Tags and attributes of xspf playlists
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

final class Xspf {
  
  static final String ENCODING = "UTF-8";
  static final String XMLNS = "http://xspf.org/ns/0/";
  static final String APPLICATION = "http://zxtune.googlecode.com";
  static final String VERSION = "1";
  
  static final class Tags {

    static final String PLAYLIST = "playlist";
    static final String EXTENSION = "extension";

    static final String PROPERTY = "property";
    
    static final String TRACKLIST = "trackList";
    
    static final String TRACK = "track";
    static final String LOCATION = "location";
    static final String CREATOR = "creator";
    static final String TITLE = "title";
    static final String DURATION = "duration";
  }
  
  static final class Attributes {
    static final String VERSION = "version";
    static final String XMLNS = "xmlns";
    static final String APPLICATION = "application";
    static final String NAME = "name";
  }
  
  static final class Properties {
    static final String PLAYLIST_CREATOR = "zxtune.app.playlist.creator";
    static final String PLAYLIST_NAME = "zxtune.app.playlist.name";
    static final String PLAYLIST_VERSION = "zxtune.app.playlist.version";
    static final String PLAYLIST_ITEMS = "zxtune.app.playlist.items";
  }
}
