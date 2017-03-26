/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

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

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp PARTIES_TTL = days(7);
  private final TimeStamp TRACKS_TTL = days(1);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final Catalog remote;
  private final Database db;
  private final CommandExecutor executor;

  public CachingCatalog(Catalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("zxart");
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
  public void queryAuthorTracks(final Author author, final TracksVisitor visitor)
          throws IOException {
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
  public void queryParties(final PartiesVisitor visitor) throws IOException {
    executor.executeQueryCommand("parties", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartiesLifetime(PARTIES_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryParties(visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Parties cache is empty/expired");
        remote.queryParties(new CachingPartiesVisitor(visitor));
      }
    });
  }

  @Override
  public void queryPartyTracks(final Party party, final TracksVisitor visitor)
          throws IOException {
    executor.executeQueryCommand("tracks", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartyTracksLifetime(party, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryPartyTracks(party, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for party=%d", party.id);
        remote.queryPartyTracks(party, new CachingTracksVisitor(visitor, party));
      }
    });
  }

  @Override
  public void queryTopTracks(final int limit, final TracksVisitor visitor) throws IOException {
    executor.executeQueryCommand("tracks", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getTopLifetime(TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTopTracks(limit, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Top tracks cache is empty/expired");
        remote.queryTopTracks(limit, new CachingTracksVisitor(visitor));
      }
    });
  }

  @Override
  public boolean searchSupported() {
    return true;
  }

  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    if (remote.searchSupported()) {
      Log.d(TAG, "Use remote-side search");
      remote.findTracks(query, visitor);
    } else {
      Log.d(TAG, "Use local search");
      db.findTracks(query, visitor);
    }
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

  private class CachingPartiesVisitor extends PartiesVisitor {

    private final PartiesVisitor delegate;

    CachingPartiesVisitor(PartiesVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }

    @Override
    public void accept(Party obj) {
      delegate.accept(obj);
      db.addParty(obj);
    }
  }

  private class CachingTracksVisitor extends TracksVisitor {

    private final TracksVisitor delegate;
    private final Author author;
    private final Party party;

    CachingTracksVisitor(TracksVisitor delegate, Author author) {
      this.delegate = delegate;
      this.author = author;
      this.party = null;
    }

    CachingTracksVisitor(TracksVisitor delegate, Party party) {
      this.delegate = delegate;
      this.author = null;
      this.party = party;
    }

    CachingTracksVisitor(TracksVisitor delegate) {
      this.delegate = delegate;
      this.author = null;
      this.party = null;
    }

    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }

    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      db.addTrack(obj);
      if (author != null) {
        db.addAuthorTrack(author, obj);
      }
      if (party != null) {
        db.addPartyTrack(party, obj);
      }
    }
  }
}
