package app.zxtune.fs.vgmrips;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;

class CachingCatalog extends Catalog {

  private final TimeStamp GROUPS_TTL = days(1);
  private final TimeStamp GROUP_PACKS_TTL = days(1);
  private final TimeStamp PACK_TRACKS_TTL = days(30);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final RemoteCatalog remote;
  private final Database db;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("vgmrips");
  }

  @Override
  public Grouping companies() {
    return new CachedGrouping(Database.TYPE_COMPANY, "company", remote.companies());
  }

  @Override
  public Grouping composers() {
    return new CachedGrouping(Database.TYPE_COMPOSER, "composer", remote.composers());
  }

  @Override
  public Grouping chips() {
    return new CachedGrouping(Database.TYPE_CHIP, "chip", remote.chips());
  }

  @Override
  public Grouping systems() {
    return new CachedGrouping(Database.TYPE_SYSTEM, "system", remote.systems());
  }

  @Nullable
  @Override
  public Pack findPack(final String id, final Visitor<Track> visitor) throws IOException {
    final Pack[] result = {null};
    executor.executeQuery(new QueryCommand() {
      @Override
      public String getScope() {
        return "pack";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getLifetime(id, PACK_TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        final Pack pack = remote.findPack(id, obj -> db.addPackTrack(id, obj));
        if (pack != null) {
          db.addPack(pack);
        }
        result[0] = pack;
      }

      @Override
      public boolean queryFromCache() {
        result[0] = db.queryPack(id);
        return result[0] != null && db.queryPackTracks(id, visitor);
      }
    });
    return result[0];
  }

  @Nullable
  @Override
  public Pack findRandomPack(Visitor<Track> visitor) throws IOException {
    if (remote.isAvailable()) {
      return findRandomPackAndCache(visitor);
    } else {
      return findRandomPackFromCache(visitor);
    }
  }

  @Nullable
  private Pack findRandomPackAndCache(Visitor<Track> visitor) throws IOException {
    final Transaction trans = db.startTransaction();
    try {
      final ArrayList<Track> tracks = new ArrayList<>();
      final Pack result = remote.findRandomPack(tracks::add);
      if (result != null) {
        db.addPack(result);
        for (Track tr : tracks) {
          db.addPackTrack(result.id, tr);
          visitor.accept(tr);
        }
        trans.succeed();
      }
      return result;
    } finally {
      trans.finish();
    }
  }

  @Nullable
  private Pack findRandomPackFromCache(Visitor<Track> visitor) {
    final Pack res = db.queryRandomPack();
    if (res != null) {
      db.queryPackTracks(res.id, visitor);
    }
    return res;
  }

  private class CachedGrouping implements Catalog.Grouping {
    @Database.Type
    private final int type;
    private final String scope;
    private final Catalog.Grouping remote;

    CachedGrouping(@Database.Type int type, String scope, Catalog.Grouping remote) {
      this.type = type;
      this.scope = scope;
      this.remote = remote;
    }

    @Override
    public void query(final Catalog.Visitor<Group> visitor) throws IOException {
      executor.executeQuery(new QueryCommand() {
        @Override
        public String getScope() {
          return scope + "s";
        }

        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getLifetime(scope, GROUPS_TTL);
        }

        @Override
        public Transaction startTransaction() {
          return db.startTransaction();
        }

        @Override
        public void updateCache() throws IOException {
          remote.query(obj -> db.addGroup(type, obj));
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroups(type, visitor);
        }
      });
    }

    @Override
    public void queryPacks(final String id, final Catalog.Visitor<Pack> visitor,
                           final ProgressCallback progress) throws IOException {
      executor.executeQuery(new QueryCommand() {
        @Override
        public String getScope() {
          return scope;
        }

        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getLifetime(id, GROUP_PACKS_TTL);
        }

        @Override
        public Transaction startTransaction() {
          return db.startTransaction();
        }

        @Override
        public void updateCache() throws IOException {
          remote.queryPacks(id, obj -> db.addGroupPack(id, obj), progress);
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroupPacks(id, visitor);
        }
      });
    }
  }
}
