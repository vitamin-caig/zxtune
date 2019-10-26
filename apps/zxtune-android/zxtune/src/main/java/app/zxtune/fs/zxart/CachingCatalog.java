/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.support.annotation.NonNull;
import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.*;
import app.zxtune.fs.http.HttpObject;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

final class CachingCatalog extends Catalog {

  private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp PARTIES_TTL = days(7);
  private final TimeStamp TRACKS_TTL = days(1);

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
    this.cache = cache.createNested("www.zxart.ee");
    this.executor = new CommandExecutor("zxart");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "authors";
      }

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
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "tracks";
      }

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
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "parties";
      }

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
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "tracks";
      }

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
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "tracks";
      }

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
    return executor.executeDownloadCommand(new DownloadCommand() {
      @NonNull
      @Override
      public File getCache() throws IOException {
        return cache.findOrCreate(Integer.toString(id));
      }

      @NonNull
      @Override
      public HttpObject getRemote() throws IOException {
        return remote.getTrackObject(id);
      }
    });
  }
}
