/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import android.content.Context;
import android.support.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.cache.CacheDir;

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

  /**
   * Retrieves track file content
   * @param id object identifier
   * @return raw file content
   */
  @NonNull
  public abstract ByteBuffer getTrackContent(int id) throws IOException;

  public static Catalog create(Context context, HttpProvider http, CacheDir cache) throws IOException {
    final Catalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db, cache);
  }
}
