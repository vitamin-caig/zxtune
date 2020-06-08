package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.Comparator;
import java.util.List;

import app.zxtune.R;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.vgmrips.Catalog;
import app.zxtune.fs.vgmrips.Group;
import app.zxtune.fs.vgmrips.Identifier;
import app.zxtune.fs.vgmrips.Pack;
import app.zxtune.fs.vgmrips.RemoteCatalog;
import app.zxtune.fs.vgmrips.Track;

@Icon(R.drawable.ic_browser_vfs_vgmrips)
final class VfsRootVgmrips extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootVgmrips.class.getName();

  private final VfsObject parent;
  private final Context context;
  private final Catalog catalog;
  private final GroupingDir[] groupings;

  VfsRootVgmrips(VfsObject parent, Context context, MultisourceHttpProvider http) {
    this.parent = parent;
    this.context = context;
    this.catalog = Catalog.create(context, http);
    this.groupings = new GroupingDir[]{
        new CompaniesDir(),
        new ComposersDir(),
        new ChipsDir(),
        new SystemsDir()
    };
  }

  @Override
  public Uri getUri() {
    return Identifier.forRoot().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_vgmrips_root_name);
  }


  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_vgmrips_root_description);
  }

  @Override
  public VfsObject getParent() {
    return parent;
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (GroupingDir group : groupings) {
      visitor.onDir(group);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    if (Identifier.isFromRoot(uri)) {
      final List<String> path = uri.getPathSegments();
      return resolve(uri, path);
    } else {
      return null;
    }
  }

  @Nullable
  private VfsObject resolve(Uri uri, List<String> path) {
    final String category = Identifier.findCategory(path);
    if (category == null) {
      return this;
    }
    final GroupingDir grouping = findGrouping(category);
    if (grouping == null) {
      return null;
    }
    final Group group = Identifier.findGroup(uri, path);
    if (group == null) {
      return grouping;
    }
    final GroupDir groupDir = grouping.makeChild(group);
    final Pack pack = Identifier.findPack(uri, path);
    if (pack == null) {
      return groupDir;
    }
    final PackDir packDir = new PackDir(groupDir, pack);
    final Track track = Identifier.findTrack(uri, path);
    if (track == null) {
      return packDir;
    } else {
      return new TrackFile(packDir, track);
    }
  }

  @Nullable
  private GroupingDir findGrouping(String category) {
    for (GroupingDir dir : groupings) {
      if (category.equals(dir.getCategory())) {
        return dir;
      }
    }
    return null;
  }

  private abstract class GroupingDir extends StubObject implements VfsDir {

    private final String category;
    private final Catalog.Grouping grouping;

    GroupingDir(String category, Catalog.Grouping grouping) {
      this.category = category;
      this.grouping = grouping;
    }

    @Override
    public Uri getUri() {
      return makeUri().build();
    }

    @Override
    public VfsObject getParent() {
      return VfsRootVgmrips.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      grouping.query(new Catalog.Visitor<Group>() {
        @Override
        public void accept(Group obj) {
          visitor.onDir(makeChild(obj));
        }
      });
    }

    final String getCategory() {
      return category;
    }

    final GroupDir makeChild(Group obj) {
      return new GroupDir(this, obj);
    }

    final Uri.Builder makeUri() {
      return Identifier.forCategory(category);
    }
  }

  @Icon(R.drawable.ic_browser_vfs_vgmrips_companies)
  private class CompaniesDir extends GroupingDir {

    CompaniesDir() {
      super(Identifier.CATEGORY_COMPANY, catalog.companies());
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_vgmrips_companies_name);
    }
  }

  @Icon(R.drawable.ic_browser_vfs_vgmrips_composers)
  private class ComposersDir extends GroupingDir {

    ComposersDir() {
      super(Identifier.CATEGORY_COMPOSER, catalog.composers());
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_vgmrips_composers_name);
    }
  }

  @Icon(R.drawable.ic_browser_vfs_vgmrips_chips)
  private class ChipsDir extends GroupingDir {

    ChipsDir() {
      super(Identifier.CATEGORY_CHIP, catalog.chips());
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_vgmrips_chips_name);
    }
  }

  @Icon(R.drawable.ic_browser_vfs_vgmrips_systems)
  private class SystemsDir extends GroupingDir {

    SystemsDir() {
      super(Identifier.CATEGORY_SYSTEM, catalog.systems());
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_vgmrips_systems_name);
    }
  }

  private class GroupDir extends StubObject implements VfsDir {

    private final GroupingDir parent;
    private final Group group;

    GroupDir(GroupingDir parent, Group obj) {
      this.parent = parent;
      this.group = obj;
    }

    @Override
    public Uri getUri() {
      return makeUri().build();
    }

    @Override
    public String getName() {
      return group.title;
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.packs, group.packs, group.packs);
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      parent.grouping.queryPacks(group.id, new Catalog.Visitor<Pack>() {

        @Override
        public void accept(Pack obj) {
          visitor.onDir(new PackDir(GroupDir.this, obj));
        }
      }, visitor);
    }

    final Uri.Builder makeUri() {
      return Identifier.forGroup(parent.makeUri(), group);
    }
  }

  private class PackDir extends StubObject implements VfsDir {

    private final GroupDir parent;
    private final Pack pack;

    PackDir(GroupDir parent, Pack pack) {
      this.parent = parent;
      this.pack = pack;
    }

    @Override
    public Uri getUri() {
      return makeUri().build();
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Override
    public String getName() {
      return pack.title;
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.tracks, pack.songs, pack.songs);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.findPack(pack.id, new Catalog.Visitor<Track>() {
        @Override
        public void accept(Track obj) {
          visitor.onFile(new TrackFile(PackDir.this, obj));
        }
      });
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.COMPARATOR.equals(id)) {
        return TracksComparator.INSTANCE;
      } else {
        return super.getExtension(id);
      }
    }

    private Uri.Builder makeUri() {
      return Identifier.forPack(parent.makeUri(), pack);
    }
  }

  private class TrackFile extends StubObject implements VfsFile {

    private PackDir parent;
    private Track track;

    TrackFile(PackDir parent, Track track) {
      this.parent = parent;
      this.track = track;
    }

    @Override
    public Uri getUri() {
      return Identifier.forTrack(parent.makeUri(), track).build();
    }

    @Override
    public String getName() {
      return track.title;
    }

    @Override
    public String getSize() {
      return track.duration.toString();
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return parent.pack.id + '/' + track.title;
      } else if (VfsExtensions.DOWNLOAD_URIS.equals(id)) {
        return RemoteCatalog.getRemoteUris(track);
      } else {
        return super.getExtension(id);
      }
    }
  }

  private static class TracksComparator implements Comparator<VfsObject> {

    private static final TracksComparator INSTANCE = new TracksComparator();

    @Override
    public int compare(VfsObject o1, VfsObject o2) {
      final TrackFile lh = (TrackFile) o1;
      final TrackFile rh = (TrackFile) o2;
      return lh.track.number < rh.track.number ? -1 : 1;
    }
  }
}
