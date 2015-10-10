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
import app.zxtune.fs.HttpProvider;

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
   */
  public abstract void queryAuthors(AuthorsVisitor visitor) throws IOException;
  
  /**
   * Query tracks objects
   * @param author tracks owner
   * @param visitor result receiver
   */
  public abstract void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException;

  /**
   * Query parties object
   * @param visitor result receiver
   */
  public abstract void queryParties(PartiesVisitor visitor) throws IOException;

  /**
   * Query tracks objects
   * @param party filter by party
   * @param visitor result receiver
   */
  public abstract void queryPartyTracks(Party party, TracksVisitor visitor) throws IOException;
  
  /**
   * Query top tracks (not cached
   * @param limit count
   * @param visitor result receiver
   */
  public abstract void queryTopTracks(int limit, TracksVisitor visitor) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context, HttpProvider http) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(http);
    return new CachingCatalog(context, remote, db);
  }
}
