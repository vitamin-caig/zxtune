/*
 * @file
 * @brief Playlist control remote interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

interface IPlaylist {

  void delete(in long[] ids);
  void deleteAll();
  void move(in long id, in int delta);
  void sort(in String field, in String order);
}
