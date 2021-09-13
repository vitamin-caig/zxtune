package app.zxtune.fs.vgmrips;

import static java.lang.annotation.RetentionPolicy.SOURCE;

import android.content.Context;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.room.ColumnInfo;
import androidx.room.Dao;
import androidx.room.Embedded;
import androidx.room.Entity;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.room.TypeConverter;
import androidx.room.TypeConverters;

import java.io.IOException;
import java.lang.annotation.Retention;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Utils;

class Database {

  @Retention(SOURCE)
  @IntDef({TYPE_COMPANY, TYPE_COMPOSER, TYPE_CHIP, TYPE_SYSTEM})
  @interface Type {}

  static final int TYPE_COMPANY = 0;
  static final int TYPE_COMPOSER = 1;
  static final int TYPE_CHIP = 2;
  static final int TYPE_SYSTEM = 3;

  // Groups are queried by type, so type is the first element of key
  @Entity(primaryKeys = {"type", "id"}, tableName = "groups")
  public static class GroupEntity {
    @Type
    public int type;

    @Embedded
    @NonNull
    public Group group;

    GroupEntity(@Type int type, Group group) {
      this.type = type;
      this.group = group;
    }
  }

  // Cross-reference entity
  @Entity(primaryKeys = {"id", "pack"}, tableName = "group_packs")
  public static class GroupPacksRef {
    @ColumnInfo(name = "id")
    @NonNull
    public String id;

    @ColumnInfo(name = "pack")
    @NonNull
    public String packId;

    GroupPacksRef(String id, String packId) {
      this.id = id;
      this.packId = packId;
    }
  }

  @Entity(primaryKeys = {"id"}, tableName = "packs")
  public static class PackEntity {
    @Embedded
    @NonNull
    public Pack pack;
  }

  @Entity(primaryKeys = {"pack_id", "number"}, tableName = "tracks")
  public static class TrackEntity {
    @ColumnInfo(name = "pack_id")
    @NonNull
    public String packId;

    @Embedded
    @NonNull
    public Track track;

    TrackEntity(String packId, Track track) {
      this.packId = packId;
      this.track = track;
    }
  }

  public static class TimeStampConverter {
    @TypeConverter
    public static Long toTimestamp(TimeStamp in) {
      return in.toSeconds();
    }

    @TypeConverter
    public static TimeStamp fromTimestamp(Long in) {
      return TimeStamp.fromSeconds(in);
    }
  }

  @Dao
  public static abstract class CatalogDao {
    @Query("SELECT id, title, packs FROM groups WHERE type = :type")
    @Nullable
    abstract Group[] queryGroups(@Type int type);

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract void insertGroup(GroupEntity entity);

    @Query("SELECT packs.id, packs.title, packs.songs, packs.score, packs.ratings FROM packs, " +
        "group_packs WHERE group_packs.id = :groupId AND packs.id = group_packs.pack")
    @Nullable
    abstract Pack[] queryGroupPacks(String groupId);

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract void bindGroupToPack(GroupPacksRef ref);

    @Query("SELECT * FROM packs WHERE id = :id")
    @Nullable
    abstract Pack queryPack(String id);

    @Query("SELECT * FROM packs where (SELECT COUNT(*) FROM tracks WHERE id = pack_id) != 0 ORDER BY RANDOM() LIMIT 1")
    @Nullable
    abstract Pack queryRandomPack();

    @Insert(entity = PackEntity.class, onConflict = OnConflictStrategy.REPLACE)
    abstract void insertPack(Pack pack);

    @Query("SELECT number, title, duration, location FROM tracks WHERE pack_id = :id")
    @Nullable
    abstract Track[] queryPackTracks(String id);

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract void insertTrack(TrackEntity track);
  }

  @androidx.room.Database(entities = {GroupEntity.class, GroupPacksRef.class, PackEntity.class,
      TrackEntity.class, Timestamps.DAO.Record.class}, version = 1)
  @TypeConverters({TimeStampConverter.class})
  public static abstract class DatabaseDelegate extends RoomDatabase {
    public abstract CatalogDao catalog();

    public abstract Timestamps.DAO timestamps();
  }

  private final DatabaseDelegate db;

  Database(Context ctx) {
    this.db = Room.databaseBuilder(ctx, DatabaseDelegate.class, "vgmrips").build();
    Utils.sendStatistics(db.getOpenHelper());
  }

  void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(db, cmd);
  }

  Timestamps.Lifetime getLifetime(String id, TimeStamp ttl) {
    return db.timestamps().getLifetime(id, ttl);
  }

  boolean queryGroups(@Type int type, Catalog.Visitor<Group> visitor) {
    final Group[] result = db.catalog().queryGroups(type);
    if (result == null || result.length == 0) {
      return false;
    }
    for (Group obj : result) {
      visitor.accept(obj);
    }
    return true;
  }

  void addGroup(@Type int type, Group obj) {
    db.catalog().insertGroup(new GroupEntity(type, obj));
  }

  boolean queryGroupPacks(String id, Catalog.Visitor<Pack> visitor) {
    final Pack[] result = db.catalog().queryGroupPacks(id);
    if (result == null || result.length == 0) {
      return false;
    }
    for (Pack obj : result) {
      visitor.accept(obj);
    }
    return true;
  }

  void addGroupPack(String id, Pack obj) {
    final CatalogDao dao = db.catalog();
    dao.insertPack(obj);
    dao.bindGroupToPack(new GroupPacksRef(id, obj.getId()));
  }

  void addPack(Pack obj) {
    db.catalog().insertPack(obj);
  }

  @Nullable
  Pack queryPack(String id) {
    return db.catalog().queryPack(id);
  }

  @Nullable
  Pack queryRandomPack() {
    return db.catalog().queryRandomPack();
  }

  boolean queryPackTracks(String id, Catalog.Visitor<Track> visitor) {
    final Track[] result = db.catalog().queryPackTracks(id);
    if (result == null || result.length == 0) {
      return false;
    }
    for (Track obj : result) {
      visitor.accept(obj);
    }
    return true;
  }

  void addPackTrack(String id, Track obj) {
    db.catalog().insertTrack(new TrackEntity(id, obj));
  }
}
