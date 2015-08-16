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
   * @param id identifier of specified author or null if all authors required
   * @param visitor result receiver
   */
  public abstract void queryAuthors(Integer id, AuthorsVisitor visitor) throws IOException;
  
  /**
   * Query tracks objects
   * @param author tracks owner
   * @param id filter by id. If not null, author filter may be ignored (but required for cache)
   * @param visitor result receiver
   */
  public abstract void queryAuthorTracks(Author author, Integer id, TracksVisitor visitor) throws IOException;

  /**
   * Query parties object
   * @param id identifier of specified party or null if all parties required
   * @param visitor result receiver
   */
  public abstract void queryParties(Integer id, PartiesVisitor visitor) throws IOException;

  /**
   * Query tracks objects
   * @param party filter by party
   * @param id filter by id. If not null, party filter may be ignored (but required for cache)
   * @param visitor result receiver
   */
  public abstract void queryPartyTracks(Party party, Integer id, TracksVisitor visitor) throws IOException;
  
  /**
   * Query top tracks (not cached
   * @param limit count
   * @param id filter by id
   * @param visitor result receiver
   */
  public abstract void queryTopTracks(int limit, Integer id, TracksVisitor visitor) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    return new CachingCatalog(context, remote, db);
  }
}
