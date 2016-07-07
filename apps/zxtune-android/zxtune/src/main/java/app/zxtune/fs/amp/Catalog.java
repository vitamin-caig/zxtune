/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.VfsCache;

public abstract class Catalog {
  
  public static abstract class GroupsVisitor {
    
    public void setCountHint(int count) {}
    public abstract void accept(Group obj);
  }

  public static abstract class AuthorsVisitor {

    public void setCountHint(int count) {}
    public abstract void accept(Author obj);
  }

  public static abstract class TracksVisitor {

    public void setCountHint(int count) {}
    public abstract void accept(Track obj);
  }
  
  public static abstract class FoundTracksVisitor {
    
    public void setCountHint(int size) {}
    
    public abstract void accept(Author author, Track track);
  }
  
  public static final String NON_LETTER_FILTER = "0-9";

  /**
   * Query all groups 
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryGroups(GroupsVisitor visitor) throws IOException;
  
  /**
   * Query authors by handle filter
   * @param handleFilter letter(s) or '0-9' for non-letter entries
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthors(String handleFilter, AuthorsVisitor visitor) throws IOException;

  /**
   * Query authors by country id
   * @param country scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthors(Country country, AuthorsVisitor visitor) throws IOException;
  
  /**
   * Query authors by group id
   * @param group scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryAuthors(Group group, AuthorsVisitor visitor) throws IOException;
  
  /**
   * Query authors's tracks
   * @param author scope
   * @param visitor result receiver
   * @throws IOException
   */
  public abstract void queryTracks(Author author, TracksVisitor visitor) throws IOException;
  
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

  public static Catalog create(Context context, HttpProvider http, VfsCache cache) {
    final Catalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    final VfsCache cacheDir = cache.createNested("amp.dascene.net");
    return new CachingCatalog(remote, db, cacheDir);
  }
}
