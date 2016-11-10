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

import android.content.Context;

import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.VfsCache;

public abstract class Catalog {
  
  public static abstract class AuthorsVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author obj);
  }
  
  public static abstract class TracksVisitor {
    
    public void setCountHint(int size) {}

    public abstract void accept(Track obj);
  }
  
  public static abstract class FoundTracksVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author author, Track track);
  }

  /**
   * Query authors object
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthors(AuthorsVisitor visitor) throws IOException;
  
  /**
   * Query tracks objects
   * @param author scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException;
  
  /**
   * Checks whether tracks can be found directly from catalogue instead of scanning
   */
  public abstract boolean searchSupported();
  
  /**
   * Find tracks by query substring
   * @param query string to search in filename/title
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void findTracks(String query, FoundTracksVisitor visitor) throws IOException;
  
  public abstract ByteBuffer getTrackContent(int id) throws IOException;
  
  public static Catalog create(Context context, HttpProvider http, VfsCache cache) {
    final Catalog remote = new RemoteCatalog(http);
    final Database db = new Database(context, cache);
    return new CachingCatalog(remote, db);
  }
}
