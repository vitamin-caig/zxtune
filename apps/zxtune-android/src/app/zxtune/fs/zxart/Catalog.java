/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxart;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;

public abstract class Catalog {
  
  public static abstract class AuthorsVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author obj);
  }
  
  public static abstract class PartiesVisitor {

    public void setCountHint(int size) {}
    
    public abstract void accept(Party obj);
  }
  
  public static abstract class TracksVisitor {
    
    public void setCountHint(int size) {}

    public abstract void accept(Track obj);
  }

  /**
   * Query authors object
   * @param visitor result receiver
   * @param id identifier of specified author or null if all authors required
   */
  public abstract void queryAuthors(AuthorsVisitor visitor, Integer id) throws IOException;
  
  /**
   * Query tracks objects
   * @param visitor result receiver
   * @param author tracks owner
   * @param id filter by id. If not null, author filter may be ignored (but required for cache)
   */
  public abstract void queryAuthorTracks(TracksVisitor visitor, Author author, Integer id) throws IOException;

  /**
   * Query parties object
   * @param visitor result receiver
   * @param id identifier of specified party or null if all parties required
   */
  public abstract void queryParties(PartiesVisitor visitor, Integer id) throws IOException;

  /**
   * Query tracks objects
   * @param visitor result receiver
   * @param party filter by party
   * @param id filter by id. If not null, party filter may be ignored (but required for cache)
   */
  public abstract void queryPartyTracks(TracksVisitor visitor, Party party, Integer id) throws IOException;
  
  /**
   * Query top tracks (not cached
   * @param visitor result receiver
   * @param id filter by id
   */
  public abstract void queryTopTracks(TracksVisitor visitor, Integer id, int limit) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    return new CachingCatalog(context, remote, db);
  }
}
