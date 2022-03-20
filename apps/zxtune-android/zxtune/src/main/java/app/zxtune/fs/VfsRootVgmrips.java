package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;
import androidx.core.util.Pair;

import java.io.IOException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Random;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.vgmrips.Catalog;
import app.zxtune.fs.vgmrips.Group;
import app.zxtune.fs.vgmrips.Identifier;
import app.zxtune.fs.vgmrips.Pack;
import app.zxtune.fs.vgmrips.RemoteCatalog;
import app.zxtune.fs.vgmrips.Track;
import app.zxtune.utils.ProgressCallback;

@Icon(R.drawable.ic_browser_vfs_vgmrips)
final class VfsRootVgmrips extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootVgmrips.class.getName();

  private final VfsObject parent;
  private final Context context;
  private final Catalog catalog;
  private final RandomDir randomTracks;
  private final GroupingDir[] groupings;

  VfsRootVgmrips(VfsObject parent, Context context, MultisourceHttpProvider http) {
    this.parent = parent;
    this.context = context;
    this.catalog = Catalog.create(context, http);
    this.randomTracks = new RandomDir();
    this.groupings = new GroupingDir[]{
        new CompaniesDir(),
        new ComposersDir(),
        new ChipsDir(),
        new SystemsDir(),
        randomTracks
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
    if (Identifier.CATEGORY_RANDOM.equals(category)) {
      return resolveRandomTrack(uri, path);
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

  private VfsObject resolveRandomTrack(Uri uri, List<String> path) {
    final Pack pack = Identifier.findRandomPack(uri, path);
    final Track track = Identifier.findTrack(uri, path);
    if (path != null && track != null) {
      final PackDir packDir = new PackDir(randomTracks, pack);
      return new TrackFile(packDir, track);
    } else {
      return randomTracks;
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

  private interface ParentDir extends VfsDir {
    Uri.Builder makeUri();
  }

  private abstract class GroupingDir extends StubObject implements ParentDir {

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
      grouping.query(obj -> visitor.onDir(makeChild(obj)));
    }

    final String getCategory() {
      return category;
    }

    final GroupDir makeChild(Group obj) {
      return new GroupDir(this, obj);
    }

    @Override
    public Uri.Builder makeUri() {
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

  @Icon(R.drawable.ic_browser_vfs_radio)
  private class RandomDir extends GroupingDir {
    RandomDir() {
      super(Identifier.CATEGORY_RANDOM, new Catalog.Grouping() {
        @Override
        public void query(Catalog.Visitor<Group> visitor) {}

        @Override
        public void queryPacks(String id, Catalog.Visitor<Pack> visitor, ProgressCallback progress) {}
      });
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_vgmrips_random_name);
    }

    @Override
    public void enumerate(Visitor visitor) {}

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.FEED.equals(id)) {
        return new FeedIterator();
      } else {
        return super.getExtension(id);
      }
    }
  }

  private class FeedIterator implements java.util.Iterator<VfsFile> {

    private final ArrayDeque<Pair<Pack, Track>> tracks = new ArrayDeque<>();
    // hash(id) => hit count
    private final SparseIntArray history = new SparseIntArray();
    private final Random random = new Random();

    @Override
    public boolean hasNext() {
      if (tracks.isEmpty()) {
        loadRandomTracks();
      }
      return !tracks.isEmpty();
    }

    @Override
    public VfsFile next() {
      final Pair<Pack, Track> obj = tracks.removeFirst();
      final PackDir dir = new PackDir(randomTracks, obj.first);
      return new TrackFile(dir, obj.second);
    }

    private void loadRandomTracks() {
      try {
        final Pair<Pack, Track> next = loadNextTrack();
        if (next != null && canPlay(next.first)) {
          tracks.addLast(next);
        }
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to get next track");
      }
    }

    @Nullable
    private Pair<Pack, Track> loadNextTrack() throws IOException {
      final ArrayList<Track> tracks = new ArrayList<>();
      final int[] duration = {0};
      final Pack pack = catalog.findRandomPack(obj -> {
        tracks.add(obj);
        duration[0] += obj.getDuration().toSeconds();
      });
      if (pack == null || 0 == duration[0]) {
        return null;
      }
      final int hit = random.nextInt(duration[0]);
      int bound = 0;
      for (Track trk : tracks) {
        bound += trk.getDuration().toSeconds();
        if (hit < bound) {
          return new Pair<>(pack, trk);
        }
      }
      return null;
    }

    private boolean canPlay(Pack pack) {
      final int key = pack.getId().hashCode();
      int donePlays = history.get(key, 0);
      if (++donePlays > pack.getSongs()) {
        return false;
      }
      history.put(key, donePlays);
      return true;
    }
  }

  private class GroupDir extends StubObject implements ParentDir {

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
      return group.getTitle();
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.packs, group.getPacks(), group.getPacks());
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      parent.grouping.queryPacks(group.getId(), obj -> visitor.onDir(new PackDir(GroupDir.this, obj)),
          visitor);
    }

    @Override
    public Uri.Builder makeUri() {
      return Identifier.forGroup(parent.makeUri(), group);
    }
  }

  private class PackDir extends StubObject implements VfsDir {

    private final ParentDir parent;
    private final Pack pack;

    PackDir(ParentDir parent, Pack pack) {
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
      return pack.getTitle();
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.tracks, pack.getSongs(), pack.getSongs());
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.findPack(pack.getId(), obj -> visitor.onFile(new TrackFile(PackDir.this, obj)));
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
      return track.getTitle();
    }

    @Override
    public String getSize() {
      return track.getDuration().toString();
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return parent.pack.getId() + '/' + track.getTitle();
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
      return lh.track.getNumber() < rh.track.getNumber() ? -1 : 1;
    }
  }
}
