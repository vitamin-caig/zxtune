/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxtunes;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import app.zxtune.fs.HttpProvider;

public abstract class Catalog {
  
  public static abstract class AuthorsVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author obj);
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
   * @param author scope
   * @param visitor result receiver
   */
  public abstract void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context, HttpProvider http) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(http);
    return new CachingCatalog(context, remote, db);
  }
}
