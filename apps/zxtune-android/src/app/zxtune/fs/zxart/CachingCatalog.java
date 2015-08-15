/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.zxart;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.zxart.Database.CacheLifetime;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();
  private final static String CACHE_DIR_NAME = "www.zxart.ee";

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp PARTIES_TTL = days(60);
  private final TimeStamp TOP_TTL = days(1);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final VfsCache cacheDir;
  private final Catalog remote;
  private final Database db;

  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = VfsCache.create(context, CACHE_DIR_NAME);
    this.remote = remote;
    this.db = db;
  }

  private interface QueryCommand {
    Database.CacheLifetime getLifetime();

    boolean queryFromCache();

    void queryFromRemote() throws IOException;
  }

  private void executeQuery(QueryCommand cmd) throws IOException {
    final Database.CacheLifetime lifetime = cmd.getLifetime();
    if (lifetime.isExpired() || !cmd.queryFromCache()) {
      IOException remoteError = null;
      final Database.Transaction transaction = db.startTransaction();
      try {
        cmd.queryFromRemote();
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!cmd.queryFromCache() && remoteError != null) {
        throw remoteError;
      }
    }
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor, final Integer id) throws IOException {
    executeQuery(new QueryCommand() {
      @Override
      public Database.CacheLifetime getLifetime() {
        return db.getAuthorsLifetime(null, AUTHORS_TTL);
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(visitor, id);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for id=%s", id);
        remote.queryAuthors(new CachingAuthorsVisitor(), null);
      }
    });
  }

  @Override
  public void queryAuthorTracks(final TracksVisitor visitor, final Author author, final Integer id)
      throws IOException {
    executeQuery(new QueryCommand() {
      @Override
      public CacheLifetime getLifetime() {
        return db.getAuthorsLifetime(author.id, AUTHORS_TTL);
      }

      @Override
      public boolean queryFromCache() {
        return id != null
            ? db.queryTrack(visitor, id)
            : db.queryAuthorTracks(visitor, author);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for id=%s author=%d", id, author.id);
        remote.queryAuthorTracks(new CachingTracksVisitor(author), author, null);
      }
    });
  }

  @Override
  public void queryParties(final PartiesVisitor visitor, final Integer id) throws IOException {
    executeQuery(new QueryCommand() {
      @Override
      public CacheLifetime getLifetime() {
        return db.getPartiesLifetime(null, PARTIES_TTL);
      }

      @Override
      public boolean queryFromCache() {
        return db.queryParties(visitor, id);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Parties cache is empty/expired for id=%s", id);
        remote.queryParties(new CachingPartiesVisitor(), null);
      }
    });
  }

  @Override
  public void queryPartyTracks(final TracksVisitor visitor, final Party party, final Integer id)
      throws IOException {
    executeQuery(new QueryCommand() {
      @Override
      public CacheLifetime getLifetime() {
        return db.getPartiesLifetime(party.id, PARTIES_TTL);
      }

      @Override
      public boolean queryFromCache() {
        return id != null
            ? db.queryTrack(visitor, id)
            : db.queryPartyTracks(visitor, party);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for id=%s party=%d", id, party.id);
        remote.queryPartyTracks(new CachingTracksVisitor(party), party, null);
      }
    });
  }

  @Override
  public void queryTopTracks(final TracksVisitor visitor, final Integer id, final int limit) throws IOException {
    executeQuery(new QueryCommand() {
      @Override
      public CacheLifetime getLifetime() {
        return db.getTopLifetime(TOP_TTL);
      }
      
      @Override
      public boolean queryFromCache() {
        return id != null
          ? db.queryTrack(visitor, id)
          : db.queryTopTracks(visitor, limit);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Top tracks cache is empty/expired");
        remote.queryTopTracks(new CachingTracksVisitor(), id, limit);
      }
    });
  }

  @Override
  public ByteBuffer getTrackContent(int id) throws IOException {
    final String strId = Integer.toString(id);
    final ByteBuffer cachedContent = cacheDir.getCachedFileContent(strId);
    if (cachedContent != null) {
      return cachedContent;
    } else {
      final ByteBuffer content = remote.getTrackContent(id);
      cacheDir.putCachedFileContent(strId, content);
      return content;
    }
  }

  private class CachingAuthorsVisitor extends AuthorsVisitor {

    @Override
    public void accept(Author obj) {
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptAuthor()");
      }
    }
  }

  private class CachingPartiesVisitor extends PartiesVisitor {

    @Override
    public void accept(Party obj) {
      try {
        db.addParty(obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptParty()");
      }
    }
  }

  private class CachingTracksVisitor extends TracksVisitor {

    private final Integer author;
    private final Integer party;

    CachingTracksVisitor(Author author) {
      this.author = author.id;
      this.party = null;
    }

    CachingTracksVisitor(Party party) {
      this.author = null;
      this.party = party.id;
    }

    CachingTracksVisitor() {
      this.author = null;
      this.party = null;
    }

    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj, author, party);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptTrack()");
      }
    }
  }
}
