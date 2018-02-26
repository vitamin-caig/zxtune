/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.support.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;

final class CachingCatalog extends Catalog {

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp PARTIES_TTL = days(7);
  private final TimeStamp TRACKS_TTL = days(1);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final Catalog remote;
  private final Database db;
  private final CacheDir cache;
  private final CommandExecutor executor;

  public CachingCatalog(Catalog remote, Database db, CacheDir cache) {
    this.remote = remote;
    this.db = db;
    this.cache = cache.createNested("www.zxart.ee");
    this.executor = new CommandExecutor("zxart");
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
  public void queryAuthorTracks(final Author author, final TracksVisitor visitor)
          throws IOException {
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
  public void queryParties(final PartiesVisitor visitor) throws IOException {
    executor.executeQuery("parties", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartiesLifetime(PARTIES_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryParties(new PartiesVisitor() {
          @Override
          public void accept(Party obj) {
            db.addParty(obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryParties(visitor);
      }
    });
  }

  @Override
  public void queryPartyTracks(final Party party, final TracksVisitor visitor)
          throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartyTracksLifetime(party, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryPartyTracks(party, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
            db.addPartyTrack(party, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryPartyTracks(party, visitor);
      }
    });
  }

  @Override
  public void queryTopTracks(final int limit, final TracksVisitor visitor) throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getTopLifetime(TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryTopTracks(limit, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTopTracks(limit, visitor);
      }
    });
  }

  @Override
  public boolean searchSupported() {
    return true;
  }

  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    if (remote.searchSupported()) {
      remote.findTracks(query, visitor);
    } else {
      db.findTracks(query, visitor);
    }
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
      public ByteBuffer fetchFromRemote() throws IOException {
        final ByteBuffer res = remote.getTrackContent(id);
        cache.createFile(filename, res);
        return res;
      }
    });
  }
}
