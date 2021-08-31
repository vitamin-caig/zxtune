/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.utils.ProgressCallback;

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
  public abstract void queryAuthors(AuthorsVisitor visitor, ProgressCallback progress) throws IOException;

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
  public abstract void queryTracks(Author author, TracksVisitor visitor, ProgressCallback progress) throws IOException;

  /**
   * Query genre's tracks
   * @param genre scope
   * @param visitor result receiver
   */
  public abstract void queryTracks(Genre genre, TracksVisitor visitor, ProgressCallback progress) throws IOException;

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
  public abstract void findRandomTracks(TracksVisitor visitor) throws IOException;

  public static CachingCatalog create(Context context, MultisourceHttpProvider http, String key) {
    final RemoteCatalog remote = new RemoteCatalog(http, key);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db);
  }
}
