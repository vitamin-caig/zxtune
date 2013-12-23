/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;

import app.zxtune.fs.zxtunes.Author;

public abstract class Catalog {

  public interface GroupsVisitor {

    void accept(Group obj);
  }

  public interface TracksVisitor {

    void accept(Track obj);
  }

  public interface Grouping {

    /**
     * Query set of group objects by filter
     * @param filter letter(s) or '#' for non-letter entries
     * @param visitor result receiver
     * @throws IOException
     */
    public void query(String filter, GroupsVisitor visitor) throws IOException;

    /**
     * Query single group object
     * @param id object identifier
     * @return null if no object found
     * @throws IOException
     */
    public Group query(int id) throws IOException;

    /**
     * Query group's tracks
     * @param id object identifier
     * @param visitor result receiver
     * @throws IOException
     */
    public void queryTracks(int id, TracksVisitor visitor) throws IOException;
  }

  public abstract Grouping getAuthors();
  public abstract Grouping getCollections();

  /**
   * Get track file content
   * @param path path to module starting from /pub/..
   * @return content
   * @throws IOException
   */
  public abstract ByteBuffer getTrackContent(String path) throws IOException;

  public static Catalog create(Context context) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    return new CachingCatalog(context, remote, db);
  }
}
