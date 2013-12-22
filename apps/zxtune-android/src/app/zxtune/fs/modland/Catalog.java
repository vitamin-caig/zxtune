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

public abstract class Catalog {

  public interface AuthorsVisitor {

    void accept(Author obj);
  }

  public interface TracksVisitor {

    void accept(Track obj);
  }

  /**
   * Query authors object
   * @param filter letter or set of letters
   * @param visitor result receiver
   */
  public abstract void queryAuthors(String filter, AuthorsVisitor visitor) throws IOException;

  /**
   * Query single author
   * @param id author's id
   * @return author object of null if nof found
   */
  public abstract Author queryAuthor(int id) throws IOException;

  /**
   * Query tracks objects
   * @param authorId author's identifier
   * @param visitor result receiver
   */
  public abstract void queryAuthorTracks(int authorId, TracksVisitor visitor) throws IOException;

  public abstract ByteBuffer getTrackContent(String id) throws IOException;

  public static Catalog create(Context context) {
    //final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    //return new CachingCatalog(context, remote, db);
    return remote;
  }
}
