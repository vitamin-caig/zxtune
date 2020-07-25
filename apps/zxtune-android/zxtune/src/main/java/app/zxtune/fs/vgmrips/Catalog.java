package app.zxtune.fs.vgmrips;

import android.content.Context;

import androidx.annotation.Nullable;

import java.io.IOException;

import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public interface Visitor<T> {
    void accept(T obj);
  }

  public interface Grouping {
    void query(Visitor<Group> visitor) throws IOException;
    void queryPacks(String id, Visitor<Pack> visitor, ProgressCallback progress) throws IOException;
  }

  public abstract Grouping companies();
  public abstract Grouping composers();
  public abstract Grouping chips();
  public abstract Grouping systems();

  @Nullable
  public abstract Pack findPack(String id, Visitor<Track> visitor) throws IOException;

  @Nullable
  public abstract Pack findRandomPack(Visitor<Track> visitor) throws IOException;

  public static Catalog create(Context context, MultisourceHttpProvider http) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    final Database db = new Database(context);
    return new CachingCatalog(remote, db);
  }
}
