/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import android.support.annotation.NonNull;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;

final class CachingCatalog extends Catalog {

  private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(7);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final RemoteCatalog remote;
  private final Database db;
  private final CacheDir cache;
  private final CommandExecutor executor;

  public CachingCatalog(RemoteCatalog remote, Database db, CacheDir cache) {
    this.remote = remote;
    this.db = db;
    this.cache = cache.createNested("www.zxtunes.com");
    this.executor = new CommandExecutor("zxtunes");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthors(new AuthorsVisitor() {
          @Override
          public void accept(Author obj) {
            db.addAuthor(obj);
          }
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
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthorTracks(author, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
            db.addAuthorTrack(author, obj);
          }
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

  @Override
  @NonNull
  public ByteBuffer getTrackContent(final int id) throws IOException {
    final String filename = Integer.toString(id);
    return executor.executeFetchCommand("file", new FetchCommand<ByteBuffer>() {
      @Override
      public ByteBuffer fetchFromCache() {
        return cache.findFile(filename);
      }

      @Override
      @NonNull
      public ByteBuffer updateCache() throws IOException {
        try {
          fillCache();
          final ByteBuffer data = cache.findFile(filename);
          if (data != null) {
            return data;
          }
        } catch (IOException e) {
          Log.w(TAG, e, "getTrackContent");
        }
        return remote.getTrackContent(id);
      }

      private void fillCache() throws IOException {
        final OutputStream stream = cache.createFile(filename);
        try {
          remote.getTrackContent(id, stream);
          stream.flush();
        } finally {
          stream.close();
        }
      }
    });
  }
}
