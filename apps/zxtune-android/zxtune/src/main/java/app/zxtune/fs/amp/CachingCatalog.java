/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.support.annotation.NonNull;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.*;
import app.zxtune.fs.http.HttpObject;

final class CachingCatalog extends Catalog {

  private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp GROUPS_TTL = days(30);
  private final TimeStamp AUTHORS_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(30);

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
    this.cache = cache.createNested("amp.dascene.net");
    this.executor = new CommandExecutor("amp");
  }

  @Override
  public void queryGroups(final GroupsVisitor visitor) throws IOException {
    executor.executeQuery("groups", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGroupsLifetime(GROUPS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryGroups(new GroupsVisitor() {
          @Override
          public void accept(Group obj) {
            db.addGroup(obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryGroups(visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final String handleFilter, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(handleFilter, AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthors(handleFilter, new AuthorsVisitor() {
          @Override
          public void accept(Author obj) {
            db.addAuthor(obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(handleFilter, visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final Country country, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getCountryLifetime(country, AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthors(country, new AuthorsVisitor() {
          @Override
          public void accept(Author obj) {
            db.addAuthor(obj);
            db.addCountryAuthor(country, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(country, visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final Group group, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGroupLifetime(group, AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthors(group, new AuthorsVisitor() {
          @Override
          public void accept(Author obj) {
            db.addAuthor(obj);
            db.addGroupAuthor(group, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(group, visitor);
      }
    });
  }

  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
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
        remote.queryTracks(author, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
            db.addAuthorTrack(author, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }
    });
  }

  @Override
  public boolean searchSupported() {
    return true;
  }

  @Override
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
