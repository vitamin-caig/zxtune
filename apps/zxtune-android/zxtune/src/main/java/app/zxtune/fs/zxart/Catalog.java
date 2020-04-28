/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public abstract static class AuthorsVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Author obj);
  }

  public abstract static class PartiesVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Party obj);
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
   * @param author tracks owner
   * @param visitor result receiver
   */
  public abstract void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException;

  /**
   * Query parties object
   * @param visitor result receiver
   */
  public abstract void queryParties(PartiesVisitor visitor) throws IOException;

  /**
   * Query tracks objects
   * @param party filter by party
   * @param visitor result receiver
   */
  public abstract void queryPartyTracks(Party party, TracksVisitor visitor) throws IOException;

  /**
   * Query top tracks (not cached
   * @param limit count
   * @param visitor result receiver
   */
  public abstract void queryTopTracks(int limit, TracksVisitor visitor) throws IOException;

  /**
   * Find tracks by query substring
   * @param query string to search in filename/title
   * @param visitor result receiver
   */
  public abstract void findTracks(String query, FoundTracksVisitor visitor) throws IOException;

  public static CachingCatalog create(Context context, MultisourceHttpProvider http) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db);
  }
}
