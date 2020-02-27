/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.HttpProvider;

public abstract class Catalog {

  public abstract static class AuthorsVisitor {

    public void setCountHint(int count) {
    }

    public abstract void accept(Author obj);
  }

  public abstract static class GenresVisitor {

    public void setCountHint(int count) {
    }

    public abstract void accept(Genre obj);
  }

  public abstract static class TracksVisitor {

    public void setCountHint(int count) {
    }

    public abstract void accept(Track obj);
  }

  public abstract static class FoundTracksVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Author author, Track track);
  }

  /**
   * Query authors by handle filter
   * @param visitor result receiver
   */
  public abstract void queryAuthors(AuthorsVisitor visitor) throws IOException;

  /**
   * Query all genres 
   * @param visitor result receiver
   */
  public abstract void queryGenres(GenresVisitor visitor) throws IOException;

  /**
   * Query authors's tracks
   * @param author scope
   * @param visitor result receiver
   */
  public abstract void queryTracks(Author author, TracksVisitor visitor) throws IOException;

  /**
   * Query genre's tracks
   * @param genre scope
   * @param visitor result receiver
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
   */
  public abstract void findTracks(String query, FoundTracksVisitor visitor) throws IOException;

  /**
   * Queries next random track
   * @throws IOException
   */
  @Nullable
  public abstract void findRandomTracks(TracksVisitor visitor) throws IOException;

  /**
   * Get track file content
   * @param id track identifier
   * @return content
   */
  @NonNull
  public abstract ByteBuffer getTrackContent(int id) throws IOException;

  public static Catalog create(Context context, HttpProvider http, CacheDir cache) throws IOException {
    final RemoteCatalog remote = new RemoteCatalog(context, http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db, cache);
  }
}
