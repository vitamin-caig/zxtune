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
  
  public interface AuthorsVisitor {
    
    void setCountHint(int size);
    
    void accept(Author obj);
  }
  
  public interface PartiesVisitor {

    void setCountHint(int size);
    
    void accept(Party obj);
  }
  
  public interface TracksVisitor {
    
    void setCountHint(int size);

    void accept(Track obj);
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
   * @param id filter by id. If not null, author filter may be ignored (but required for cache)
   * @param author filter by author
   */
  public abstract void queryAuthorTracks(TracksVisitor visitor, Integer id, Integer author) throws IOException;

  /**
   * Query parties object
   * @param visitor result receiver
   * @param id identifier of specified party or null if all parties required
   */
  public abstract void queryParties(PartiesVisitor visitor, Integer id) throws IOException;

  /**
   * Query tracks objects
   * @param visitor result receiver
   * @param id filter by id. If not null, party filter may be ignored (but required for cache)
   * @param party filter by party
   */
  public abstract void queryPartyTracks(TracksVisitor visitor, Integer id, Integer party) throws IOException;
  
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
