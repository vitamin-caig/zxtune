/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public abstract static class GroupsVisitor {

    public void setCountHint(int count) {
    }

    public abstract void accept(Group obj);
  }

  public abstract static class AuthorsVisitor {

    public void setCountHint(int count) {
    }

    public abstract void accept(Author obj);
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

  public static final String NON_LETTER_FILTER = "0-9";

  /**
   * Query all groups 
   * @param visitor result receiver
   */
  public abstract void queryGroups(GroupsVisitor visitor) throws IOException;

  /**
   * Query authors by handle filter
   * @param handleFilter letter(s) or '0-9' for non-letter entries
   * @param visitor result receiver
   */
  public abstract void queryAuthors(String handleFilter, AuthorsVisitor visitor) throws IOException;

  /**
   * Query authors by country id
   * @param country scope
   * @param visitor result receiver
   */
  public abstract void queryAuthors(Country country, AuthorsVisitor visitor) throws IOException;

  /**
   * Query authors by group id
   * @param group scope
   * @param visitor result receiver
   */
  public abstract void queryAuthors(Group group, AuthorsVisitor visitor) throws IOException;

  /**
   * Query authors's tracks
   * @param author scope
   * @param visitor result receiver
   */
  public abstract void queryTracks(Author author, TracksVisitor visitor) throws IOException;

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
