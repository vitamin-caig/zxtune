/**
 *
 * @file
 *
 * @brief Playlist controller interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

/**
 * Playlist-related control functionality interface
 */
public interface PlaylistControl {

  void delete(long[] ids);
  
  void deleteAll();
  
  void move(long id, int delta);
  
  //should be name-compatible with Database
  enum SortBy {
    title,
    author,
    duration
  }

  enum SortOrder {
    asc,
    desc
  }
  
  void sort(SortBy by, SortOrder order);
}
