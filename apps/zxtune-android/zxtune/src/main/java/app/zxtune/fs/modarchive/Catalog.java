/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modarchive;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.VfsCache;

public abstract class Catalog {
  
  public static abstract class AuthorsVisitor {

    public void setCountHint(int count) {}
    public abstract void accept(Author obj);
  }

  public static abstract class GenresVisitor {
    
    public void setCountHint(int count) {}
    public abstract void accept(Genre obj);
  }
  
  public static abstract class TracksVisitor {

    public void setCountHint(int count) {}
    public abstract void accept(Track obj);
  }
  
  public static abstract class FoundTracksVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author author, Track track);
  }
  
  /**
   * Query authors by handle filter
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthors(AuthorsVisitor visitor) throws IOException;

  /**
   * Query all genres 
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryGenres(GenresVisitor visitor) throws IOException;
  
  /**
   * Query authors's tracks
   * @param author scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryTracks(Author author, TracksVisitor visitor) throws IOException;

  /**
   * Query genre's tracks
   * @param genre scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryTracks(Genre genre, TracksVisitor visitor) throws IOException;
  
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
  
  /**
   * Get track file content
   * @param id track identifier
   * @return content
   * @throws IOException
   */
  public abstract ByteBuffer getTrackContent(int id) throws IOException;

  public static Catalog create(Context context, HttpProvider http) {
    final Catalog remote = new RemoteCatalog(context, http);
    final Database db = new Database(context);
    final VfsCache cacheDir = VfsCache.create(context, "modarchive.org");
    return new CachingCatalog(remote, db, cacheDir);
  }
}
