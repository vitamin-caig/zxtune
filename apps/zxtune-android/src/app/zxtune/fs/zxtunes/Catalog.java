/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;

/**
 * 
 */
public abstract class Catalog {
  
  public interface AuthorsVisitor {
    
    void accept(Author obj);
  }
  
  public interface TracksVisitor {
    
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
   * @param author author's identifier
   * @param id filter by id. If not null, author filter is ignored
   * @param author filter by author
   */
  public abstract void queryTracks(TracksVisitor visitor, Integer id, Integer author) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    return new CachingCatalog(context, remote, db);
  }
}
