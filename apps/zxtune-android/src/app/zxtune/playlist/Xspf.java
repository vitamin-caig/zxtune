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
  
  final static String ENCODING = "UTF-8";
  final static String XMLNS = "http://xspf.org/ns/0/";
  final static String APPLICATION = "http://zxtune.googlecode.com";
  final static String VERSION = "1";
  
  final static class Tags {

    final static String PLAYLIST = "playlist";
    final static String EXTENSION = "extension";

    final static String PROPERTY = "property";
    
    final static String TRACKLIST = "trackList";
    
    final static String TRACK = "track";
    final static String LOCATION = "location";
    final static String CREATOR = "creator";
    final static String TITLE = "title";
    final static String DURATION = "duration";
  }
  
  final static class Attributes {
    final static String VERSION = "version";
    final static String XMLNS = "xmlns";
    final static String APPLICATION = "application";
    final static String NAME = "name";
  }
  
  final static class Properties {
    final static String PLAYLIST_CREATOR = "zxtune.app.playlist.creator";
    final static String PLAYLIST_NAME = "zxtune.app.playlist.name";
    final static String PLAYLIST_VERSION = "zxtune.app.playlist.version";
    final static String PLAYLIST_ITEMS = "zxtune.app.playlist.items";
  }
}
