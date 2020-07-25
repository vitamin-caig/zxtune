/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public abstract static class GroupsVisitor {

    public void setCountHint(int size) {
    }

    public abstract void accept(Group obj);
  }

  public abstract static class TracksVisitor {

    public void setCountHint(int size) {
    }

    //too many tracks possible, so enable breaking
    public abstract boolean accept(Track obj);
  }

  public interface Grouping {

    /**
     * Query set of group objects by filter
     * @param filter letter(s) or '#' for non-letter entries
     * @param visitor result receiver
     */
    void queryGroups(String filter, GroupsVisitor visitor, ProgressCallback progress) throws IOException;

    /**
     * Query single group object
     * @param id object identifier
     */
    Group getGroup(int id) throws IOException;

    /**
     * Query group's tracks
     * @param id object identifier
     * @param visitor result receiver
     */
    void queryTracks(int id, TracksVisitor visitor, ProgressCallback progress) throws IOException;

    /**
     * Query track by name
     * @param id object identifier
     * @param filename track filename
     */
    Track getTrack(int id, String filename) throws IOException;
  }

  public abstract Grouping getAuthors();

  public abstract Grouping getCollections();

  public abstract Grouping getFormats();

  public static CachingCatalog create(Context context, MultisourceHttpProvider http) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db);
  }
}
