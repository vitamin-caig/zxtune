/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final TimeStamp GROUPS_TTL = days(30);
  private final TimeStamp AUTHORS_TTL = days(30);
  private final TimeStamp COUNTRIES_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(30);
  
  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }
  
  private final Catalog remote;
  private final Database db;

  public CachingCatalog(Catalog remote, Database db) {
    this.remote = remote;
    this.db = db;
  }

  @Override
  public void queryGroups(final GroupsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGroupsLifetime(GROUPS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Groups cache is empty/expired");
        remote.queryGroups(new CachingGroupsVisitor(visitor));
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryGroups(visitor);
      }
    });
  }
  
  @Override
  public void queryAuthors(final String handleFilter, final AuthorsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(handleFilter, AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for handleFilter=%s", handleFilter);
        remote.queryAuthors(handleFilter, new CachingAuthorsVisitor(visitor));
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(handleFilter, visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final Country country, final AuthorsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getCountryLifetime(country, COUNTRIES_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for country=%d", country.id);
        remote.queryAuthors(country, new CachingAuthorsVisitor(visitor, country));
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(country, visitor);
      }
    });
  }
  
  @Override
  public void queryAuthors(final Group group, final AuthorsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGroupLifetime(group, GROUPS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for group=%d", group.id);
        remote.queryAuthors(group, new CachingAuthorsVisitor(visitor, group));
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(group, visitor);
      }
    });
  }
  
  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for author=%d", author.id);
        remote.queryTracks(author, new CachingTracksVisitor(visitor, author));
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
      Log.d(TAG, "Use remote-side search");
      remote.findTracks(query, visitor);
    } else {
      Log.d(TAG, "Use local search");
      db.findTracks(query, visitor);
    }
  }
  
  @Override
  public ByteBuffer getTrackContent(final int id) throws IOException {
    return Utils.executeFetchCommand(new FetchCommand() {
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
  
  private class CachingGroupsVisitor extends GroupsVisitor {
    
    private final GroupsVisitor delegate;
    
    CachingGroupsVisitor(GroupsVisitor delegate) {
      this.delegate = delegate;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }
    
    @Override
    public void accept(Group obj) {
      delegate.accept(obj);
      db.addGroup(obj);
    }
  }
  
  private class CachingAuthorsVisitor extends AuthorsVisitor {
    
    private final AuthorsVisitor delegate;
    private final Country country;
    private final Group group;
    
    CachingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
      this.country = null;
      this.group = null;
    }
    
    CachingAuthorsVisitor(AuthorsVisitor delegate, Country country) {
      this.delegate = delegate;
      this.country = country;
      this.group = null;
    }
    
    CachingAuthorsVisitor(AuthorsVisitor delegate, Group group) {
      this.delegate = delegate;
      this.country = null;
      this.group = group;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      db.addAuthor(obj);
      if (country != null) {
        db.addCountryAuthor(country, obj);
      }
      if (group != null) {
        db.addGroupAuthor(group, obj);
      }
    }
  }
  
  private class CachingTracksVisitor extends TracksVisitor {
    
    private final TracksVisitor delegate;
    private final Author author;
    
    public CachingTracksVisitor(TracksVisitor delegate, Author author) {
      this.delegate = delegate;
      this.author = author;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }
    
    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      db.addTrack(obj);
      db.addAuthorTrack(author, obj);
    }
  }
}
