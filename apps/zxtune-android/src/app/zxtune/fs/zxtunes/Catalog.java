/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

import java.io.IOException;

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
  public abstract void queryAuthors(AuthorsVisitor visitor, Integer id);
  
  /**
   * Query tracks objects
   * @param visitor result receiver
   * @param author author's identifier
   * @param id filter by id. If not null, other filters are ignored
   * @param author filter by author. If null date filter is ignored
   * @param date additionally filter by author's date 
   */
  public abstract void queryTracks(TracksVisitor visitor, Integer id, Integer author, Integer date);
  
  public abstract byte[] getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context) {
    final Database db = new Database(context);
    final Catalog remote = new RemoteCatalog(context);
    return remote;//new CachingCatalog(context, remote, db);
  }
}
