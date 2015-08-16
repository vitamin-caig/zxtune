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
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();
  private final static String CACHE_DIR_NAME = "www.zxart.ee";

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp PARTIES_TTL = days(14);
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

  @Override
  public void queryAuthors(final Integer id, final AuthorsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }
      
      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(id, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for id=%s", id);
        //query all
        remote.queryAuthors(null, new CachingAuthorsVisitor());
      }
    });
  }

  @Override
  public void queryAuthorTracks(final Author author, final Integer id, final TracksVisitor visitor)
      throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, AUTHORS_TTL);
      }
      
      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return id != null
            ? db.queryTrack(id, visitor)
            : db.queryAuthorTracks(author, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for id=%s author=%d", id, author.id);
        //query all
        remote.queryAuthorTracks(author, null, new CachingTracksVisitor(author));
      }
    });
  }

  @Override
  public void queryParties(final Integer id, final PartiesVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartiesLifetime(PARTIES_TTL);
      }
      
      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryParties(id, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Parties cache is empty/expired for id=%s", id);
        //query all
        remote.queryParties(null, new CachingPartiesVisitor());
      }
    });
  }

  @Override
  public void queryPartyTracks(final Party party, final Integer id, final TracksVisitor visitor)
      throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getPartyTracksLifetime(party, PARTIES_TTL);
      }
      
      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return id != null
            ? db.queryTrack(id, visitor)
            : db.queryPartyTracks(party, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for id=%s party=%d", id, party.id);
        //query all
        remote.queryPartyTracks(party, null, new CachingTracksVisitor(party));
      }
    });
  }

  @Override
  public void queryTopTracks(final int limit, final Integer id, final TracksVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getTopLifetime(TOP_TTL);
      }
      
      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return id != null
          ? db.queryTrack(id, visitor)
          : db.queryTopTracks(limit, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Top tracks cache is empty/expired");
        //query all
        remote.queryTopTracks(limit, null, new CachingTracksVisitor());
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

    private final Author author;
    private final Party party;

    CachingTracksVisitor(Author author) {
      this.author = author;
      this.party = null;
    }

    CachingTracksVisitor(Party party) {
      this.author = null;
      this.party = party;
    }

    CachingTracksVisitor() {
      this.author = null;
      this.party = null;
    }

    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj);
        if (author != null) {
          db.addAuthorTrack(author, obj);
        }
        if (party != null) {
          db.addPartyTrack(party, obj);
        }
      } catch (Exception e) {
        Log.d(TAG, e, "acceptTrack()");
      }
    }
  }
}
