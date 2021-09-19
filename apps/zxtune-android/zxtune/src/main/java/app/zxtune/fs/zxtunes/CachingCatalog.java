/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import java.io.IOException;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;

final public class CachingCatalog implements Catalog {

  private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = TimeStamp.fromDays(30);
  private final TimeStamp TRACKS_TTL = TimeStamp.fromDays(7);

  private final Catalog remote;
  private final Database db;
  private final CommandExecutor executor;

  CachingCatalog(Catalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("zxtunes");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorsLifetime(AUTHORS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthors(new AuthorsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

            @Override
            public void accept(Author obj) {
              db.addAuthor(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(visitor);
      }
    });
  }

  @Override
  public void queryAuthorTracks(final Author author, final TracksVisitor visitor) throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthorTracks(author, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

            @Override
            public void accept(Track obj) {
              db.addTrack(obj);
              db.addAuthorTrack(author, obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthorTracks(author, visitor);
      }
    });
  }

  @Override
  public boolean searchSupported() {
    //TODO: rework logic to more clear
    if (remote.searchSupported()) {
      //network is available, so direct scanning may be more comprehensive
      //TODO: check out if all the cache is not expired
      Log.d(TAG, "Disable fast search due to enabled network");
      return false;
    }
    return true;
  }

  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) {
    //TODO: query also remote catalog when search will be enabled
    db.findTracks(query, visitor);
  }
}
