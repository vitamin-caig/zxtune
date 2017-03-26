/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
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

  private final Catalog remote;
  private final Database db;
  private final CommandExecutor executor;

  public CachingCatalog(Catalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("zxtunes");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQueryCommand("authors", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired");
        remote.queryAuthors(new CachingAuthorsVisitor(visitor));
      }
    });
  }

  @Override
  public void queryAuthorTracks(final Author author, final TracksVisitor visitor) throws IOException {
    executor.executeQueryCommand("tracks", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthorTracks(author, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for author=%d", author.id);
        remote.queryAuthorTracks(author, new CachingTracksVisitor(visitor, author));
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
  public ByteBuffer getTrackContent(final int id) throws IOException {
    return executor.executeFetchCommand("file", new FetchCommand<ByteBuffer>() {
      @Override
      public ByteBuffer fetchFromCache() {
        return db.getTrackContent(id);
      }

      @Override
      public ByteBuffer fetchFromRemote() throws IOException {
        final ByteBuffer res = remote.getTrackContent(id);
        db.addTrackContent(id, res);
        return res;
      }
    });
  }

  private class CachingAuthorsVisitor extends AuthorsVisitor {

    private final AuthorsVisitor delegate;

    CachingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      db.addAuthor(obj);
    }
  }

  private class CachingTracksVisitor extends TracksVisitor {

    private final TracksVisitor delegate;
    private final Author author;

    CachingTracksVisitor(TracksVisitor delegate, Author author) {
      this.delegate = delegate;
      this.author = author;
    }

    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }

    @Override
    public void accept(Track track) {
      delegate.accept(track);
      db.addTrack(track);
      db.addAuthorTrack(author, track);
    }
  }
}
