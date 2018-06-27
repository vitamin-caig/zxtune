/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.io.IOException;
import java.util.TreeSet;

import app.zxtune.R;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;
import app.zxtune.fs.joshw.Path;

final class VfsRootJoshw extends HttpRootBase implements VfsRoot {

  private final Context context;
  private final AudiobaseDir bases[];

  VfsRootJoshw(Context context, HttpProvider http, CacheDir cache) throws IOException {
    super(Catalog.create(context, http, cache, "joshw"), Path.create());
    this.context = context;
    this.bases = new AudiobaseDir[]{
            new AudiobaseDir("gbs", R.string.vfs_joshw_gbs_name, R.string.vfs_joshw_gbs_description),
            new AudiobaseDir("hes", R.string.vfs_joshw_hes_name, R.string.vfs_joshw_hes_description),
            new AudiobaseDir("nsf", R.string.vfs_joshw_nsf_name, R.string.vfs_joshw_nsf_description),
            new AudiobaseDir("smd", R.string.vfs_joshw_smd_name, R.string.vfs_joshw_smd_description),
            new AudiobaseDir("spc", R.string.vfs_joshw_spc_name, R.string.vfs_joshw_spc_description),
            new AudiobaseDir("kss", R.string.vfs_joshw_kss_name, R.string.vfs_joshw_kss_description),
            new AudiobaseDir("psf", R.string.vfs_joshw_psf_name, R.string.vfs_joshw_psf_description),
            new AudiobaseDir("usf", R.string.vfs_joshw_usf_name, R.string.vfs_joshw_usf_description),
            new AudiobaseDir("gsf", R.string.vfs_joshw_gsf_name, R.string.vfs_joshw_gsf_description),
            new AudiobaseDir("2sf", R.string.vfs_joshw_2sf_name, R.string.vfs_joshw_2sf_description),
            new AudiobaseDir("ssf", R.string.vfs_joshw_ssf_name, R.string.vfs_joshw_ssf_description),
            new AudiobaseDir("dsf", R.string.vfs_joshw_dsf_name, R.string.vfs_joshw_dsf_description),
    };
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_joshw_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_joshw_root_description);
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_joshw;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (AudiobaseDir base : bases) {
      visitor.onDir(base);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) throws IOException {
    return resolve(Path.parse(uri));
  }

  @Override
  @Nullable
  public VfsDir makeDir(app.zxtune.fs.httpdir.Path path) {
    final Path ownPath = (Path) path;
    return ownPath.isCatalogue()
            ? resolveCatalogue(ownPath.getCatalogue())
            : super.makeDir(path);
  }

  @Nullable
  private VfsDir resolveCatalogue(String id) {
    for (AudiobaseDir dir : bases) {
      if (id.equals(dir.getBaseId())) {
        return dir;
      }
    }
    return null;
  }

  @Override
  public VfsFile makeFile(app.zxtune.fs.httpdir.Path path, String size) {
    return new MusicFile(path, size);
  }

  private class AudiobaseDir extends HttpDir {

    private final String id;
    private final int name;
    private final int description;

    AudiobaseDir(String id, int nameRes, int descRes) {
      super(rootPath.getChild(id));
      this.id = id;
      this.name = nameRes;
      this.description = descRes;
    }

    @Override
    public String getName() {
      return context.getString(name);
    }

    @Override
    public String getDescription() {
      return context.getString(description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootJoshw.this;
    }

    final String getBaseId() {
      return id;
    }
  }

  private class MusicFile extends HttpFile {

    //${title} (\[${alt title}\])?(\(${tag}\))*
    private String name;
    private String description;

    MusicFile(app.zxtune.fs.httpdir.Path path, String size) {
      super(path, size);
      loadMetainfo();
    }

    @Override
    public String getName() {
      if (name == null) {
        loadMetainfo();
      }
      return name;
    }

    @Override
    public String getDescription() {
      if (description == null) {
        loadMetainfo();
      }
      return description;
    }

    private void loadMetainfo() {
      final String filename = path.getName();
      int endOfName = filename.indexOf('(');
      if (endOfName == -1) {
        endOfName = filename.lastIndexOf('.');
      }
      name = filename.substring(0, endOfName - 1).trim();
      final TreeSet<String> meta = new TreeSet<>();
      for (int prevMetaPos = endOfName; ; ) {
        final int metaPos = filename.indexOf('(', prevMetaPos);
        if (metaPos == -1) {
          break;
        }
        final int metaPosEnd = filename.indexOf(')', metaPos);
        if (metaPosEnd == -1) {
          break;
        }
        final String field = filename.substring(metaPos + 1, metaPosEnd);
        meta.add(field);
        prevMetaPos = metaPosEnd;
      }
      description = TextUtils.join(" - ", meta);
    }
  }
}
