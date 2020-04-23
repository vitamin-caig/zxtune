/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public abstract static class AuthorsVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Author obj);
  }

  public abstract static class TracksVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Track obj);
  }

  public abstract static class FoundTracksVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Author author, Track track);
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

  /**
   * Checks whether tracks can be found directly from catalogue instead of scanning
   */
  public abstract boolean searchSupported();

  /**
   * Find tracks by query substring
   * @param query string to search in filename/title
   * @param visitor result receiver
   */
  public abstract void findTracks(String query, FoundTracksVisitor visitor) throws IOException;

  public static CachingCatalog create(Context context, MultisourceHttpProvider http, CacheDir cache) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db, cache);
  }
}
