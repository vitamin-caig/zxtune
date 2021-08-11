package app.zxtune.fs.vgmrips;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.utils.ProgressCallback;

class CachingCatalog implements Catalog {

  private final TimeStamp GROUPS_TTL = TimeStamp.fromDays(1);
  private final TimeStamp GROUP_PACKS_TTL = TimeStamp.fromDays(1);
  private final TimeStamp PACK_TRACKS_TTL = TimeStamp.fromDays(30);

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
    executor.executeQuery("pack", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getLifetime(id, PACK_TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          final Pack pack = remote.findPack(id, obj -> db.addPackTrack(id, obj));
          if (pack != null) {
            db.addPack(pack);
          }
          lifetime.update();
          result[0] = pack;
        });
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
    final Pack[] result = {null};
    db.runInTransaction(() -> {
      final ArrayList<Track> tracks = new ArrayList<>();
      result[0] = remote.findRandomPack(tracks::add);
      if (result[0] != null) {
        db.addPack(result[0]);
        for (Track tr : tracks) {
          db.addPackTrack(result[0].getId(), tr);
          visitor.accept(tr);
        }
      }
    });
    return result[0];
  }

  @Nullable
  private Pack findRandomPackFromCache(Visitor<Track> visitor) {
    final Pack res = db.queryRandomPack();
    if (res != null) {
      db.queryPackTracks(res.getId(), visitor);
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
      executor.executeQuery(scope + "s", new QueryCommand() {

        private final Timestamps.Lifetime lifetime = db.getLifetime(scope, GROUPS_TTL);

        @Override
        public boolean isCacheExpired() {
          return lifetime.isExpired();
        }

        @Override
        public void updateCache() throws IOException {
          db.runInTransaction(() -> {
            remote.query(obj -> db.addGroup(type, obj));
            lifetime.update();
          });
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
      executor.executeQuery(scope, new QueryCommand() {

        private final Timestamps.Lifetime lifetime = db.getLifetime(id, GROUP_PACKS_TTL);

        @Override
        public boolean isCacheExpired() {
          return lifetime.isExpired();
        }

        @Override
        public void updateCache() throws IOException {
          db.runInTransaction(() -> {
            remote.queryPacks(id, obj -> db.addGroupPack(id, obj), progress);
            lifetime.update();
          });
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroupPacks(id, visitor);
        }
      });
    }
  }
}
